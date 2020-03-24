//
// Created by kh on 3/23/20.
//

#include "Algo.h"

#include <fstream>
#include <csignal>
#include <boost/range/adaptor/reversed.hpp>
#include <boost/circular_buffer.hpp>
#include <utility>

#include <igen/Context.h>
#include <igen/Domain.h>
#include <igen/Config.h>
#include <igen/ProgramRunner.h>
#include <igen/CoverageStore.h>
#include <igen/c50/CTree.h>

#include <klib/print_stl.h>
#include <klib/vecutils.h>
#include <klib/random.h>
#include <glog/raw_logging.h>

namespace igen {

namespace {
static volatile std::sig_atomic_t gSignalStatus = 0;
}

class Iter2 : public Object {
public:
    explicit Iter2(PMutContext ctx) : Object(move(ctx)) {}

    ~Iter2() override = default;

public:
    int terminate_counter = 0, max_terminate_counter = 0, n_iterations = 0;
    bool full_configs = false;
    set<hash_t> set_conf_hash, set_ran_conf_hash;

    struct LocData;
    using PLocData = ptr<LocData>;

    struct LocData : public intrusive_ref_base_st<LocData> {
        LocData(PLocation loc) : loc(std::move(loc)) {}

        void link_to(PLocData p) { parent = p, tree = nullptr; }

        void unlink() { link_to(nullptr); }

        bool linked() { return parent != nullptr; }

        int id() const { return loc->id(); }

        PLocation loc;
        PLocData parent;
        PMutCTree tree;

        bool queued_next = false, ignored = false;
        int messed_up = 0;
    };

    vec<PLocData> v_loc_data, v_this_iter, v_next_iter, v_uniq;

public:
    void run_alg() {
        max_terminate_counter = ctx()->get_option_as<int>("term-cnt");
        full_configs = ctx()->has_option("full");
        n_iterations = ctx()->get_option_as<int>("rounds");
        // ====
        vec<PMutConfig> init_configs;
        if (full_configs) {
            init_configs = dom()->gen_all_configs();
            LOG(INFO, "Running full configs (n_configs = {})", init_configs.size());
        } else {
            init_configs = dom()->gen_one_convering_configs();
            int n_one_covering = int(init_configs.size());
            auto seed_configs = get_seed_configs();
            int n_seed_configs = int(seed_configs.size());
            vec_move_append(init_configs, seed_configs);
            LOG(INFO, "Running initial configs (n_one_covering = {}, n_seed_configs = {})", n_one_covering,
                n_seed_configs);
        }
        for (const auto &c : init_configs) set_conf_hash.insert(c->hash()), run_config(c);
        LOG(INFO, "Done running {} init configs", init_configs.size());
        // ====
        for (int iter = 1; iter <= n_iterations; ++iter) {
            CHECK_EQ(set_conf_hash.size(), set_ran_conf_hash.size());
            LOG(INFO, "{:=^80}", fmt::format("  Iteration {}  ", iter));
            if (run_iter(iter)) {
                LOG(WARNING, "terminate_counter = {:>2}", terminate_counter + 1);
                if (++terminate_counter == max_terminate_counter) {
                    LOG(WARNING, "Early break at iteration {}", iter);
                    break;
                }
                enqueue_all();
            } else {
                terminate_counter = 0;
            }
            if (gSignalStatus == SIGINT) {
                LOG(WARNING, "Requested break at iteration {}", iter);
                ctx()->program_runner()->flush_compact_cachedb();
                break;
            } else if (gSignalStatus == SIGUSR1 || gSignalStatus == SIGUSR2) {
                finish_alg("TEMP FINISH", gSignalStatus == SIGUSR2);
                gSignalStatus = 0;
            }
        }
        // ====
        finish_alg();
    }

    int gen_cex(vec<PMutConfig> &cex, const vec<PCNode> &leaves, int n_small, int n_rand = 0, int n_large = 0) {
        int gen_cnt = 0, skipped = 0, max_min_cases = -1;
        const auto gen_for = [this, &max_min_cases, &skipped, &cex, &gen_cnt](const PCNode &node, int lim = kMAX<int>) {
            int skipped_me = 0;
            maxi(max_min_cases, node->n_min_cases());
            for (auto &c : node->gen_one_convering_configs(lim)) {
                if (set_conf_hash.insert(c->hash()).second) { cex.emplace_back(move(c)), gen_cnt++; }
                else { skipped++, skipped_me++; }
            }
            return skipped_me;
        };
        // ====
        for (const PCNode &node : leaves) {
            if (sz(cex) >= n_small) break;
            gen_for(node);
        }
        VLOG(10, "Small: {:>2} cex, skipped {}, max_min_cases = {}", cex.size(), skipped, max_min_cases);
        // ====
        n_rand += n_small, max_min_cases = -1;
        for (int i = 0; i < 10; ++i) {
            if (sz(cex) >= n_rand) break;
            gen_for(*Rand.get(leaves));
        }
        VLOG(10, "Rand : {:>2} cex, skipped {}, max_min_cases = {}", cex.size(), skipped, max_min_cases);
        // ====
        n_large += n_rand, max_min_cases = -1;
        for (const PCNode &node : boost::adaptors::reverse(leaves)) {
            if (sz(cex) >= n_large) break;
            gen_for(node);
        }
        VLOG(10, "Large: {:>2} cex, skipped {}, max_min_cases = {}", cex.size(), skipped, max_min_cases);
        // ====
        return gen_cnt;
    }

    bool run_one_loc(int iter, int meidx, int t, const PLocData &dat, bool rebuild, bool heavy) {
        CHECK(!dat->linked());
        auto &tree = dat->tree;
        if (rebuild) build_tree(dat);

        vec<PMutConfig> cex;
        vec<PCNode> leaves;
        if (heavy) {
            tree->gather_nodes(leaves), sort_leaves(leaves);
            gen_cex(cex, leaves, 5, 5, 5);
        } else {
            tree->gather_nodes(leaves, 0, std::max(16, tree->n_min_cases() + 1)), sort_leaves(leaves);
            gen_cex(cex, leaves, 5);
        }
        const auto &loc = dat->loc;
        if (cex.empty()) {
            LOG(INFO, "Can't gen cex for loc ({}) {}", loc->id(), loc->name());
            return true;
        }

        for (const auto &c : cex) run_config(c);

        bool ok = true;
        for (const PMutConfig &c : cex) {
            const vec<int> cov_ids = c->cov_loc_ids();
            bool new_truth = std::binary_search(cov_ids.begin(), cov_ids.end(), loc->id());
            bool tree_eval = tree->test_add_config(c, new_truth).first;
            if (tree_eval != new_truth) {
                ok = false;
                break;
            }
        }

        LOG(INFO, "{:>4} {:>3} | {:>4} {:>2} {:>5} {:>3} {} | "
                  "{:>3} {:>3} {:>3} {:>2} | {:>5} {:>4} {:>3} | {:>5}",
            iter, t,
            dat->loc->id(), dat->messed_up, leaves.size(), cex.size(), ok ? ' ' : '*',
            meidx, v_this_iter.size(), v_next_iter.size(), terminate_counter,
            cov()->n_configs(), cov()->n_locs(), v_uniq.size(),
            ctx()->program_runner()->n_cache_hit()
        );
        return ok;
    }

    bool run_iter(int iter) {
        v_this_iter = std::move(v_next_iter), v_next_iter.clear();
        auto v_new_locdat = prepare_vec_loc_data();
        int n_new_loc = sz(v_new_locdat);
        vec_move_append(v_this_iter, v_new_locdat);
        LOG_BLOCK(INFO, {
            fmt::print(log, "v_this_iter (tot {:3>}, new {}) = {{", v_this_iter.size(), n_new_loc);
            for (const auto &d : v_this_iter) fmt::print(log, "{}/{}, ", d->loc->id(), d->loc->name());
            log << "}";
        });
        CHECK(is_no_dup(v_this_iter));
        int meidx = 0, prev_n_conf = cov()->n_configs();
        for (const auto &dat : v_loc_data) dat->queued_next = false;
        for (const auto &dat : v_this_iter) {
            CHECK(!dat->linked());
            bool finished = false, need_rebuild = true;
            int c_success = 0;
            meidx++;
            for (int t = 1; t <= 10; ++t) {
                if (gSignalStatus == SIGINT) return false;
                if (run_one_loc(iter, meidx, t, dat, need_rebuild, c_success > 0)) {
                    need_rebuild = false;
                    if (++c_success == 3) {
                        finished = true;
                        break;
                    }
                } else {
                    need_rebuild = true;
                    c_success = 0;
                }
            }
            if (!finished) enqueue_next(dat);
        }
        int add_loc = 0;
        for (int i = 0; i < prev_n_conf; ++i) {
            const auto &c = cov()->config(i);
            const vec<int> cov_ids = c->cov_loc_ids();
            auto it = cov_ids.begin();
            for (const PLocation &loc : cov()->locs()) {
                if (loc->id() >= sz(v_loc_data)) continue;
                bool new_truth = false;
                if (it != cov_ids.end() && *it == loc->id()) new_truth = true, ++it;
                const auto &dat = v_loc_data[loc->id()];
                if (dat->linked() || dat->queued_next || dat->ignored) continue;
                CHECK_NE(dat->tree, nullptr) << fmt::format("({}) {}", loc->id(), loc->name());

                bool tree_eval = dat->tree->test_config(c).first;
                if (new_truth != tree_eval) {
                    add_loc++;
                    enqueue_next(dat);
                }
            }
        }
        LOG(INFO, "Added {} locs not in v_this_iter", add_loc);
        return v_next_iter.empty();
    }

    void enqueue_next(const PLocData &dat) {
        CHECK(!dat->ignored);
        if (terminate_counter > 0 && ++dat->messed_up == 10) {
            LOG(WARNING, "Ignore loc ({}) {}", dat->id(), dat->loc->name());
            dat->ignored = true;
            return;
        }
        v_next_iter.emplace_back(dat), dat->queued_next = true;
    }

    void enqueue_all() {
        v_next_iter.reserve(v_uniq.size());
        for (const auto &dat : v_uniq)
            if (!dat->ignored) v_next_iter.emplace_back(dat);
    }

    void finish_alg(str header = "FINAL RESULT", bool expensive_simplify = true) {
        bool out_to_file = ctx()->has_option("output");
        auto expr_strat = CTree::FreeMix;
        if (ctx()->has_option("disj-conj")) expr_strat = CTree::DisjOfConj;
        // ====

        vec<vec<PLocData>> vvp(v_loc_data.size());
        for (const auto &d : v_loc_data)
            if (d->linked()) vvp[d->parent->id()].push_back(d);
            else vvp[d->id()].push_back(d);

        LOG(INFO, "{:=^80}", "  " + header + "  ");
        std::stringstream out;
        int simpl_cnt = 0;
        for (const PLocData &dat : v_loc_data) {
            const auto &loc = dat->loc;
            const auto &tree = dat->tree;
            build_tree(dat);
            if (dat->linked()) continue;

            bool do_simpl = expensive_simplify;
            LOG_IF(INFO, do_simpl, "Simplifying expr: {:>3} ({}) {}", ++simpl_cnt, loc->id(), loc->name());
            CHECK_NE(tree, nullptr);

            z3::expr e = tree->build_zexpr(expr_strat);
            e = e.simplify();
            if (do_simpl) e = ctx()->zctx_solver_simplify(e);

            if (dat->ignored) out << "# IGNORED\n";
            for (const auto &d : vvp[dat->id()])
                out << d->loc->name() << ", ";
            out << "\n-\n" << e << "\n======\n";
        }

        if (out_to_file) {
            str fout = ctx()->get_option_as<str>("output");
            std::ofstream ofs(fout);
            if (ofs.fail()) {
                LOG(INFO, "OUTPUT: \n") << out.rdbuf();
                CHECKF(0, "Can't open output file {}", fout);
            }
            ofs << out.rdbuf();
        } else {
            LOG(INFO, "OUTPUT: \n") << out.rdbuf();
        }
    }

private:
    map<hash_t, PLocData> map_hash_locs;

    vec<PLocData> prepare_vec_loc_data() {
        vec<PLocData> v_new_locdat;
        map_hash_locs.clear(), map_hash_locs.reserve(size_t(cov()->n_locs()));
        v_loc_data.resize(size_t(cov()->n_locs()));
        v_uniq.clear();
        for (const auto &loc : cov()->locs()) {
            auto &dat = v_loc_data[size_t(loc->id())];
            bool is_new = false;
            if (dat == nullptr) dat = new LocData(loc), is_new = true;
            if (auto res = map_hash_locs.try_emplace(loc->hash(), dat); !res.second) {
                dat->link_to(res.first->second);
            } else {
                if (is_new || dat->linked()) v_new_locdat.emplace_back(dat);
                if (dat->linked()) dat->unlink();
                v_uniq.emplace_back(dat);
            }
        }
        return v_new_locdat;
    }

    void run_config(const PMutConfig &c) {
        auto loc_names = ctx()->program_runner()->run(c);
        cov_mut()->register_cov(c, loc_names);
        CHECK(set_ran_conf_hash.insert(c->hash()).second);
        if (dom()->n_vars() <= 16 && loc_names.size() <= 16) {
            VLOG(50, "{}  ==>  ", *c) << loc_names;
        } else {
            VLOG(50, "Config {}  ==>  {} locs", c->id(), loc_names.size());
        }
    }

    vec<PMutConfig> get_seed_configs() const {
        vec<PMutConfig> ret;
        if (ctx()->has_option("seed-configs")) {
            auto vs = ctx()->get_option_as<vec<str>>("seed-configs");
            for (const auto &s : vs)
                ret.emplace_back(new Config(ctx_mut(), s));
        }
        return ret;
    }

    int get_cur_ignore_thr(int x) {
        static constexpr double L = 110, x0 = 15000, k = -0.000346;
        double res = L / (1.0 + std::exp(-k * (x - x0)));
        return (int) std::max(res, 1.0);
    }

    void sort_leaves(vec<PCNode> &leaves) const {
        std::sort(leaves.begin(), leaves.end(), [](const PCNode &a, const PCNode &b) {
            if (a->n_min_cases() != b->n_min_cases())
                return a->n_min_cases() < b->n_min_cases();
            else
                return a->depth() > b->depth();
        });
    }

    bool is_no_dup(const vec<PLocData> &v) const {
        set<hash_t> s;
        for (const auto &d : v)
            if (!s.insert(d->loc->hash()).second) return false;
        return true;
    }

    void build_tree(const PLocData &d) {
        auto &tree = d->tree;
        tree = new CTree(ctx()), tree->prepare_data(d->loc), tree->build_tree();
    }
};

int run_interative_algorithm_2(const boost::program_options::variables_map &vm) {
    const auto &sighandler = [](int signal) {
        if (signal == SIGINT && gSignalStatus) {
            RAW_LOG(ERROR, "Force terminate");
            exit(1);
        }
        gSignalStatus = signal;
    };
    std::signal(SIGINT, sighandler);
    std::signal(SIGUSR1, sighandler);
    std::signal(SIGUSR2, sighandler);

    PMutContext ctx = new Context();
    for (const auto &kv : vm) {
        ctx->set_option(kv.first, kv.second.value());
    }
    ctx->init();
    ctx->program_runner()->init();
    {
        ptr<Iter2> ite_alg = new Iter2(ctx);
        ite_alg->run_alg();
    }
    ctx->cleanup();
    return 0;
}

}