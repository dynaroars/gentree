//
// Created by kh on 3/23/20.
//

#include "Algo.h"

#include <fstream>
#include <csignal>
#include <boost/range/adaptor/reversed.hpp>
#include <boost/circular_buffer.hpp>

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
        LocData(const PLocation &loc) : loc(loc) {}

        void link_to(PLocData p) { parent = p, tree = nullptr; }

        bool linked() { return parent != nullptr; }

        PLocation loc;
        PLocData parent;
        PMutCTree tree;
    };

    vec<PLocData> v_loc_data, v_this_iter, v_next_iter;

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
            LOG(INFO, "{:=^80}", fmt::format("  Iteration {}  ", iter));
            if (run_iter(iter)) {
                LOG(WARNING, "Early break at iteration {}", iter);
                break;
            }
            if (gSignalStatus == SIGINT) {
                LOG(WARNING, "Requested break at iteration {}", iter);
                ctx()->program_runner()->flush_compact_cachedb();
                break;
            }
        }
        // ====
    }

    bool gen_cex(vec<PMutConfig> &cex, const vec<PCNode> &leaves, int n_small, int n_rand = 0, int n_large = 0) {
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
        LOG(INFO, "Small: {:>2} cex, skipped {}, max_min_cases = {}", cex.size(), skipped, max_min_cases);
        // ====
        int rand_skip = 0;
        n_rand += n_small, max_min_cases = -1;
        while (sz(cex) < n_rand) {
            if (gen_for(*Rand.get(leaves)) > 0 && ++rand_skip == 10) break;
        }
        LOG(INFO, "Rand : {:>2} cex, skipped {}, max_min_cases = {}, rand_skip = {}",
            cex.size(), skipped, max_min_cases, rand_skip);
        // ====
        n_large += n_rand, max_min_cases = -1;
        for (const PCNode &node : boost::adaptors::reverse(leaves)) {
            if (sz(cex) >= n_large) break;
            gen_for(node);
        }
        LOG(INFO, "Large: {:>2} cex, skipped {}, max_min_cases = {}", cex.size(), skipped, max_min_cases);
        // ====
        return gen_cnt;
    }

    bool run_one_loc([[maybe_unused]] int t, const PLocData &dat) {
        CHECK(!dat->linked());
        auto &tree = dat->tree;
        tree = new CTree(ctx()), tree->prepare_data(dat->loc), tree->build_tree();

        vec<PMutConfig> cex;
        vec<PCNode> leaves;
        tree->gather_nodes(leaves, 0, tree->n_min_cases() + 1);
        if(gen_cex(cex, leaves, 10) == 0) {
            tree->gather_nodes(leaves, 0, tree->n_min_cases() + 1);
        }
        return false;
    }

    bool run_iter([[maybe_unused]] int iter) {
        v_this_iter = std::move(v_next_iter), v_next_iter.clear();
        auto v_new_locdat = prepare_vec_loc_data();
        vec_move_append(v_this_iter, v_new_locdat);
        for (const auto &dat : v_this_iter) {
            CHECK(!dat->linked());
            bool finished = false;
            for (int t = 1; t <= 100; ++t) {
                if (run_one_loc(t, dat)) {
                    finished = true;
                    break;
                }
            }
            if (!finished) v_next_iter.emplace_back(dat);
        }
        return false;
    }

private:
    map<hash_t, PLocData> map_hash_locs;

    vec<PLocData> prepare_vec_loc_data() {
        vec<PLocData> v_new_locdat;
        map_hash_locs.clear(), map_hash_locs.reserve(cov()->n_locs());
        v_loc_data.resize(cov()->n_locs());
        for (const auto &loc : cov()->locs()) {
            auto &dat = v_loc_data[loc->id()];
            bool is_new = false;
            if (dat == nullptr) dat = new LocData(loc), is_new = true;
            if (auto res = map_hash_locs.try_emplace(loc->hash(), dat); !res.second) {
                dat->link_to(res.first->second);
                if (is_new) v_new_locdat.emplace_back(dat);
            }
        }
        return v_new_locdat;
    }

    void run_config(const PMutConfig &c) {
        auto loc_names = ctx()->program_runner()->run(c);
        cov()->register_cov(c, loc_names);
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
            vec<str> vs = ctx()->get_option_as<vec<str>>("seed-configs");
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
};

int run_interative_algorithm_2(const boost::program_options::variables_map &vm) {
    std::signal(SIGINT, [](int signal) {
        if (gSignalStatus) {
            RAW_LOG(ERROR, "Force terminate");
            exit(1);
        }
        gSignalStatus = signal;
    });

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