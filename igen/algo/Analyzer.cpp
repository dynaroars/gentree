//
// Created by kh on 3/14/20.
//

#include "Algo.h"

#include <igen/Context.h>
#include <igen/Domain.h>
#include <igen/Config.h>
#include <igen/ProgramRunnerMt.h>
#include <igen/CoverageStore.h>
#include <igen/c50/CTree.h>

#include <klib/print_stl.h>
#include <klib/vecutils.h>
#include <klib/random.h>

#include <fstream>

#include <boost/algorithm/string.hpp>
#include <boost/container/flat_map.hpp>
#include <boost/container/flat_set.hpp>

#include <tsl/robin_set.h>
#include <tsl/robin_map.h>

namespace igen {


class Analyzer : public Object {
public:
    explicit Analyzer(PMutContext ctx) : Object(move(ctx)) {}

    struct LocData {
        expr e;
        PMutCTree tree;
        bool ignored;
        bool is_first;
        unsigned id_;

        [[nodiscard]] unsigned id() const { return id_; }
    };

    expr to_expr(const z3::model &m, double &ncex) {
        z3::expr_vector vecExpr(m.ctx());
        for (int id = 0; id < dom()->n_vars(); ++id) {
            expr v = m.get_const_interp(dom()->var(id)->zvar().decl());
            if (v) {
                dom()->var(id)->val_id_of(v);
                vecExpr.push_back(dom()->var(id)->zvar() == v);
            } else {
                ncex *= dom()->n_values(id);
            }
        }
        LOG_IF(INFO, ncex > 1, "Has free variable");
        return z3::mk_and(vecExpr);
    }

    tsl::robin_map<unsigned, double> cache_count_models;

    static constexpr double DEF_LIM_MODELS = 1e18;

    double count_models(const expr &ex, double lim = DEF_LIM_MODELS, z3::expr_vector *v_models = nullptr) {
        auto it = cache_count_models.find(ex.id());
        if (it != cache_count_models.end()) return it->second;
        double &ncex = cache_count_models[ex.id()];

        auto solver = ctx()->zscope();
        solver->add(ex);

        while (ncex < lim) {
            z3::check_result checkres = solver->check();
            if (checkres == z3::unsat) {
                break;
            } else if (checkres == z3::unknown) {
                LOG(WARNING, "Z3 solver returns unknown");
                maxi(ncex, 1.0);
                break;
            }
            CHECK_EQ(checkres, z3::sat);
            z3::model m = solver->get_model();
            double this_cex = 1;
            expr mexpr = to_expr(m, this_cex);
            ncex += this_cex;
            solver->add(!mexpr);
            if (v_models != nullptr) v_models->push_back(mexpr);
        }
        return ncex;
    }

    static bool is_all(const str &s, char c) {
        if (s.empty()) return false;
        for (char x : s) if (x != c) return false;
        return true;
    }

    map<str, LocData> read_file(const str &path) {
        map<str, LocData> res;
        std::ifstream f(path);
        CHECKF(!f.fail(), "Error reading file: {}", path);
        str line;
        int read_state = 0;
        vec<str> locs;
        str sexpr, stree;
        bool ignored = false;
        set<unsigned> sexprid;
        unsigned cur_id = 0;
        int cnt_ignored = 0;
        while (getline(f, line)) {
            boost::algorithm::trim(line);
            if (line.empty()) continue;
            if (line[0] == '#') {
                if (line.find("IGNORED") != str::npos) ignored = true;
                continue;
            }
            if (is_all(line, '-')) {
                switch (read_state) {
                    case 0:
                        CHECKF(!locs.empty(), "Empty location set ({})", path);
                        break;
                    case 1:
                        boost::algorithm::trim(sexpr);
                        CHECKF(!sexpr.empty(), "Empty sexpr location set ({})", path);
                        break;
                    default:
                        CHECK(0);
                }
                read_state++;
                CHECK_LE(read_state, 2);
                continue;
            } else if (is_all(line, '=')) {
                if (locs.empty()) continue;
                expr e = dom()->parse_string(move(sexpr));
                PMutCTree tree = new CTree(ctx_mut());
                std::stringstream stream_stree(stree);
                tree->deserialize(stream_stree);
                CHECKF(sexprid.insert(e.id()).second, "Duplicated expression ({}), loc {}", path,
                       locs.empty() ? str() : locs.at(0));
                //LOG(INFO, "EXPR: ") << e;
                expr tree_e = tree->build_zexpr(CTree::FreeMix);
                CHECKF(count_models(e != tree_e, 1) == 0, "Mismatch tree and expr: {}", path);
                bool is_first = true;
                for (const str &s : locs) {
                    CHECKF(!res.contains(s), "Duplicated location ({}): {}", path, s);
                    res.emplace(s, LocData{e, tree, ignored, is_first, cur_id});
                    is_first = false;
                }
                cur_id++, cnt_ignored += ignored;
                ignored = false, read_state = 0, locs.clear(), sexpr.clear(), stree.clear();
                continue;
            }
            switch (read_state) {
                case 0: {
                    for (char &c : line) if (c == ',') c = ' ';
                    std::stringstream ss(line);
                    str tok;
                    while (ss >> tok) locs.emplace_back(move(tok));
                    break;
                }
                case 1: {
                    sexpr += line, sexpr += '\n';
                    break;
                }
                case 2: {
                    CHECKF(stree.empty(), "Invalid tree data ({})", path);
                    stree = move(line);
                    break;
                }
                default:
                    CHECK(0);
            }
        }
        LOG(INFO, "Read {} locs ({} uniq, {} ignored) from {}", res.size(), cur_id, cnt_ignored, path);
        return res;
    }

    // compare two output
    void run_analyzer_0() {
        auto finp = get_inp();
        CHECK_EQ(finp.size(), 2) << "Need two input files to compare";
        auto ma = read_file(finp.at(0));
        auto mb = read_file(finp.at(1));
        set<unsigned> sdiff, smissing, slocsa, slocsb, sprintdiff;
        int cntdiff = 0, cntmissing = 0;
        double totalcex = 0;
        z3::expr_vector ve_cex(zctx());
        for (const auto &p : ma) {
            const auto &dat = p.second;
            slocsa.insert(dat.id());
            auto it = mb.find(p.first);
            if (it == mb.end()) {
                VLOG(dat.is_first ? 5 : 7, "{} not found in B", p.first);
                smissing.insert(dat.id()), cntmissing++;
                continue;
            }
            const double LIM = DEF_LIM_MODELS;
            expr expr_cex = dat.e != it->second.e;
            ve_cex.push_back(expr_cex);
            double num_cex = count_models(expr_cex, LIM, &ve_cex);
            if (num_cex == LIM) {
                if (dat.is_first) {
                    LOG(WARNING, "Loc {} has more than {:G} cex", p.first, LIM);
                } else {
                    VLOG(7, "Loc {} has more than {:G} cex", p.first, LIM);
                }
            }
            if (num_cex == 0) {
                //VLOG(0, "{} ok", p.first);
            } else {
                VLOG(dat.is_first ? 5 : 7, "{} diff (cex = {:G}){} (id={})",
                     p.first, num_cex,
                     dat.ignored || it->second.ignored ? " (IGNORED)" : "",
                     dat.id());
                if (sprintdiff.insert(dat.id()).second) {
                    totalcex += num_cex;
                    GVLOG(10) << "\nA: " << dat.e << "\nB: " << it->second.e;
                }
                sdiff.insert(dat.id()), cntdiff++;
            }
        }
        for (const auto &p : mb) {
            slocsb.insert(p.second.id());
            if (!ma.contains(p.first)) {
                VLOG(p.second.is_first ? 5 : 7, "{} not found in A", p.first);
                smissing.insert(p.second.id()), cntmissing++;
            }
        }
        LOG(INFO, "Counting total cex");
        double total_cex = count_models(z3::mk_or(ve_cex));

        int repeat_id = ctx()->get_option_as<int>("_repeat_id");
        LOG(INFO, "R[{:>2}] {:=^80}",
            repeat_id, "  FINAL RESULT  ");
        LOG(INFO, "R[{:>2}] Total: diff {:^4}, miss {:^4}, locs A {:^4}, B {:^4}",
            repeat_id, cntdiff, cntmissing, ma.size(), mb.size());
        LOG(INFO, "R[{:>2}] Uniq : diff {:^4}, miss {:^4}, locs A {:^4}, B {:^4}, cex {:G}",
            repeat_id,
            sdiff.size(), smissing.size(),
            slocsa.size(), slocsb.size(), totalcex);
        LOG(INFO, "R[{:>2}] Total CEX: {:G}",
            repeat_id, total_cex);
    }

    // try random conf
    void run_analyze_1() {
        boost::timer::cpu_timer timer;
        ctx()->init_runner();

        auto finp = get_inp();
        CHECK_EQ(finp.size(), 1) << "Need 1 input file";
        auto ma = read_file(finp.at(0));

        int n_iter = 0;
        bool run_full = false;
        if (ctx()->has_option("rand")) n_iter = ctx()->get_option_as<int>("rand");
        else if (ctx()->has_option("full")) n_iter = (int) dom()->config_space(), run_full = true;
        CHECK_GT(n_iter, 0) << "Invalid arguments";

        int n_batch = ctx()->get_option_as<int>("batch-size");
        if (n_batch == 0) n_batch = 100;
        CHECK_GT(n_batch, 0);

        tsl::robin_set<hash_t> all_configs;
        all_configs.reserve(n_iter);
        tsl::robin_set<hash_t> wrong_configs, wrong_configs_nig;
        set<str> wrong_locs, wrong_locs_uniq, missing_locs, empty_set;
        set<str> wrong_locs_nig, wrong_locs_uniq_nig, missing_locs_nig;

        vec<PMutConfig> batch_confs(n_batch), generated_full_configs;
        vec<set<str>> batch_locs;
        for (auto &c : batch_confs) c = new Config(ctx_mut());
        int it = n_batch;

        int nvars = dom()->n_vars();
        vec<int> dom_nvals(nvars);
        for (int var = 0; var < nvars; ++var)
            dom_nvals[var] = dom()->n_values(var);

        if (run_full) {
            LOG(INFO, "Generating full {:G} configs", dom()->config_space());
            generated_full_configs = dom()->gen_all_configs();
            CHECK_EQ(n_iter, generated_full_configs.size());
        }
        int it_full_configs = 0;

        for (int iter = 1; iter <= n_iter; ++iter) {
            if (it == n_batch) {
                if (run_full) {
                    for (int i = 0; i < n_batch; ++i) {
                        if (it_full_configs < sz(generated_full_configs)) {
                            batch_confs[i] = move(generated_full_configs[it_full_configs++]);
                            all_configs.insert(batch_confs[i]->hash());
                        } else {
                            batch_confs.erase(batch_confs.begin() + i, batch_confs.end());
                            break;
                        }
                    }
                } else {
                    for (int i = 0; i < n_batch; ++i) {
                        const PMutConfig &c = batch_confs[i];
                        do {
                            for (int var = 0; var < nvars; ++var)
                                c->set(var, Rand.get(dom_nvals[var]));
                        } while (!all_configs.insert(c->hash(true)).second);
                    }
                }
                batch_locs = ctx_mut()->runner()->run(batch_confs);
                CHECK_EQ(batch_confs.size(), batch_locs.size());
                it = 0;
            }

            const PMutConfig &c = batch_confs.at(it);
            CHECK_NE(c, nullptr);
            const set<str> &e = batch_locs.at(it);
            ++it;

            vec<int8_t> eval_cache(ma.size(), -1);
            eval_cache.reserve(ma.size());
            for (const auto &p : ma) {
                const auto &dat = p.second;
                int8_t &cache_val = eval_cache[dat.id()];
                bool eval_res, uniq_loc;
                if (cache_val == -1) {
                    dat.tree->build_interpreter();
                    cache_val = (eval_res = dat.tree->interpret(*c));
                    //CHECK_EQ(eval_res, c->eval(dat.e));
                    uniq_loc = true;
                } else {
                    eval_res = (bool) cache_val;
                    uniq_loc = false;
                }

                bool truth = e.contains(p.first);
                if (eval_res != truth) {
                    wrong_configs.insert(c->hash());
                    wrong_locs.insert(p.first);
                    if (uniq_loc) wrong_locs_uniq.insert(p.first);

                    if (!dat.ignored) {
                        wrong_configs_nig.insert(c->hash());
                        wrong_locs_nig.insert(p.first);
                        if (uniq_loc) {
                            wrong_locs_uniq_nig.insert(p.first);
                            LOG_FIRST_N(WARNING, 1000)
                                << fmt::format("Wrong: loc {}, ", p.first)
                                << *c << "  #    " << c->to_str_raw();
                        }
                    }
                }
            }
            for (const auto &s : e) {
                if (!ma.contains(s)) {
                    LOG_FIRST_N(ERROR, 1000)
                        << fmt::format("Missing: loc {}, ", s) << *c
                        << "  #    " << c->to_str_raw();
                    missing_locs.insert(s);
                }
            }
            if (it == 1) {
                LOG(INFO, "{:>4} {:>4} {:>4} | {:>4} {:>4} {:>4} | {:>4} {:>4} {:>4} | ",
                    all_configs.size(), wrong_configs.size(), ctx()->runner()->n_cache_hit(),
                    wrong_locs.size(), wrong_locs_uniq.size(), missing_locs.size(),
                    wrong_configs_nig.size(), wrong_locs_nig.size(), wrong_locs_uniq_nig.size()
                )
                        << (sz(wrong_locs_uniq) <= 10 ? wrong_locs_uniq : empty_set)
                        << (sz(missing_locs) <= 10 ? missing_locs : empty_set);
            }
        }
        LOG(INFO, "Total        time: {}", timer.format(0));
        LOG(INFO, "Runner       time: {}", boost::timer::format(ctx()->runner()->timer(), 0));
        LOG(INFO, "Multi-runner time: {}", boost::timer::format(ctx()->runner()->total_elapsed(), 0));
    }

    vec<str> get_inp() {
        str inp = ctx()->get_option_as<str>("input");
        boost::algorithm::replace_all(inp, ",", " ");
        std::stringstream ss(inp);
        str tok;
        vec<str> res;
        while (ss >> tok) res.push_back(tok);
        return res;
    }

    map<str, boost::any> run_alg() {
        switch (ctx()->get_option_as<int>("alg-version")) {
            case 0:
                run_analyzer_0();
                return {};
            case 1:
                run_analyze_1();
                return {};
            default:
                CHECK(0);
        }
    }
};


map<str, boost::any> run_analyzer(const map<str, boost::any> &opts) {
    map<str, boost::any> ret;
    PMutContext ctx = new Context();
    ctx->set_options(opts);
    ctx->init();
    //ctx->init_runner();
    {
        ptr<Analyzer> ite_alg = new Analyzer(ctx);
        ret = ite_alg->run_alg();
    }
    ctx->cleanup();
    return ret;
}


}