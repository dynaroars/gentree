//
// Created by kh on 3/14/20.
//

#include "Algo.h"

#include <igen/Context.h>
#include <igen/Domain.h>
#include <igen/Config.h>
#include <igen/runner/ProgramRunnerMt.h>
#include <igen/CoverageStore.h>
#include <igen/c50/CTree.h>

#include <klib/print_stl.h>
#include <klib/vecutils.h>
#include <klib/random.h>

#include <fstream>

#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string_regex.hpp>
#include <boost/container/flat_map.hpp>
#include <boost/container/flat_set.hpp>
#include <boost/lexical_cast.hpp>

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
        vec<str> comments;

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
        VLOG_IF(150, ncex > 1, "Has free variable");
        return z3::mk_and(vecExpr);
    }

    map<std::pair<unsigned, int>, double> cache_count_models;

    static constexpr int DEF_LIM_MODELS = kMAX<int>;

    double count_models(const expr &ex, int lim_n_runs = DEF_LIM_MODELS, z3::expr_vector *v_models = nullptr) {
        std::pair<unsigned, int> cache_key = {ex.hash(), lim_n_runs};
        if (v_models == nullptr) {
            auto it = cache_count_models.find(cache_key);
            if (it != cache_count_models.end()) return it->second;
        }
        double &ncex = (cache_count_models[cache_key] = 0);

        auto solver = ctx()->zscope();
        solver->add(ex);

        for (int n_run = 0; n_run < lim_n_runs; ++n_run) {
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
            //VLOG(100, "ncex {:10G}", ncex);
        }
        return ncex;
    }

    static bool is_all(const str &s, char c) {
        if (s.empty()) return false;
        for (char x : s) if (x != c) return false;
        return true;
    }

    map<str, LocData> read_file(const str &path, vec<str> *global_comments = nullptr) {
        map<str, LocData> res;
        std::ifstream f(path);
        CHECKF(!f.fail(), "Error reading file: {}", path);
        str line;
        int read_state = 0;
        vec<str> locs, comments;
        str sexpr, stree;
        bool ignored = false;
        set<unsigned> sexprid;
        unsigned cur_id = 0;
        int cnt_ignored = 0;
        while (getline(f, line)) {
            boost::algorithm::trim(line);
            if (line.empty()) continue;
            if (line[0] == '#') {
                comments.push_back(line);
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
                if (locs.empty()) {
                    if (global_comments != nullptr)
                        vec_move_append(*global_comments, comments);
                    comments.clear();
                    continue;
                }
                expr e = dom()->parse_string(move(sexpr));
                PMutCTree tree = new CTree(ctx_mut());
                std::stringstream stream_stree(stree);
                tree->deserialize(stream_stree);
                CHECKF(sexprid.insert(e.hash()).second, "Duplicated expression ({}), loc {}", path,
                       locs.empty() ? str() : locs.at(0));
                //LOG(INFO, "EXPR: ") << e;
                expr tree_e = tree->build_zexpr(CTree::FreeMix);
                CHECKF(count_models(e != tree_e, 1) == 0, "Mismatch tree and expr: {}", path);
                bool is_first = true;
                for (const str &s : locs) {
                    CHECKF(!res.contains(s), "Duplicated location ({}): {}", path, s);
                    res.emplace(s, LocData{e, tree, ignored, is_first, cur_id, move(comments)});
                    is_first = false;
                }
                cur_id++, cnt_ignored += ignored;
                ignored = false, read_state = 0, locs.clear(), comments.clear(), sexpr.clear(), stree.clear();
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
            expr expr_cex = dat.e != it->second.e;
            ve_cex.push_back(expr_cex);
            double num_cex = count_models(expr_cex, kMAX<int>, &ve_cex);
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
                LOG(INFO, "{:>7} {:>4} {:>7} | {:>4} {:>4} {:>4} | {:>4} {:>4} {:>4} | ",
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

    bool is_pure(const expr &e, Z3_decl_kind outer, Z3_decl_kind inner) {
        CHECK_EQ(e.decl().decl_kind(), outer);
        int n_args = (int) e.num_args();
        CHECK_GT(n_args, 0);
        for (int i = 0; i < n_args; ++i) {
            expr g = e.arg(i);
            if (g.is_eq()) continue;
            if (g.decl().decl_kind() == inner) {
                long last_id = -1;
                int n_args_g = (int) g.num_args();
                for (int j = 0; j < n_args_g; ++j) {
                    expr x = g.arg(j);
                    if (!x.is_eq()) return false;
                    CHECK_EQ(x.num_args(), 2);
                    expr lhs = x.arg(0);

                    bool is_var = false;
                    for (const auto &t : *dom()) {
                        if (lhs.id() == t->zvar().id()) {
                            is_var = true;
                            break;
                        }
                    }
                    if (!is_var) return false;

                    if (last_id == -1) last_id = lhs.id();
                    else if (last_id != lhs.id()) return false;
                }
            } else {
                return false;
            }
        }
        return true;
    }

    bool is_igen_compat(const expr &e) {
        if (!e.is_and()) return false;
        int n_args = (int) e.num_args();
        CHECK_GT(n_args, 0);
        bool has_or_clause = false;
        for (int i = 0; i < n_args; ++i) {
            expr g = e.arg(i);
            if (g.is_eq()) continue;
            if (has_or_clause || !g.is_or()) return false;
            has_or_clause = true;
            if (!is_pure(g, Z3_OP_OR, Z3_OP_UNINTERPRETED)) return false;
        }
        return true;
    }

    // read exp result
    map<str, boost::any> run_analyze_2() {
        map<str, boost::any> ret;
        vec<str> gcmt;
        auto finp = get_inp();
        CHECK_EQ(finp.size(), 1) << "Need 1 input file";
        auto ma = read_file(finp.at(0), &gcmt);

        vec<str> params;
        boost::algorithm::split_regex(params, gcmt.at(2), boost::regex("[ |]+"));
//        LOG(INFO, "params {}", fmt::join(params, ","));

//        fmt::print(out, "# {:>8} {:>4} {:>4} | {:>5} {:>5} | {:>2} {:>4} | {:>5} {:>5} {:>6} | {}\n======\n",
//                   cov()->n_configs(), cov()->n_locs(), v_uniq.size(),
//                   ctx()->runner()->n_cache_hit(), ctx()->runner()->n_locs(),
//                   repeat_id_, iter,
//
//                   timer.elapsed().wall / NS, ctx()->runner()->local_timer().wall / NS,
//                   ctx()->runner()->total_elapsed().wall / NS,
//
//                   state_hash().str()
//        );

        int cnt_singular = 0, cnt_and = 0, cnt_or = 0, cnt_mixed = 0;
        int cnt_pure = 0, cnt_mix_ok = 0, cnt_mix_fail = 0;
        for (const auto &p : ma) {
            const auto &dat = p.second;
            if (!dat.is_first) continue;
            const expr &e = dat.e;
            VLOG(10, "{}\n {}", p.first, e.to_string());
            CHECK(e.is_app());
            if (e.is_const() || e.is_eq()) {
                VLOG(10, "=> Singular");
                cnt_singular++, cnt_pure++;
                continue;
            }
            if (e.is_and() && is_pure(e, Z3_OP_AND, Z3_OP_OR)) {
                VLOG(10, "=> And");
                cnt_and++, cnt_pure++;
                continue;
            }
            if (e.is_or() && is_pure(e, Z3_OP_OR, Z3_OP_UNINTERPRETED)) {
                VLOG(10, "=> Or");
                cnt_or++, cnt_pure++;
                continue;
            }
            bool ig_compat = is_igen_compat(e);
            VLOG(10, "=> Mixed{}", ig_compat ? ", igen compat" : "");
            cnt_mixed++;
            if (ig_compat) cnt_mix_ok++; else cnt_mix_fail++;
        }
#define RET_PARAM(name) ret[#name] = name
        RET_PARAM(cnt_singular);
        RET_PARAM(cnt_and);
        RET_PARAM(cnt_or);
        RET_PARAM(cnt_mixed);
        RET_PARAM(cnt_pure);
        RET_PARAM(cnt_mix_ok);
        RET_PARAM(cnt_mix_fail);
#undef RET_PARAM
        ret["cnt_total"] = cnt_singular + cnt_and + cnt_or + cnt_mixed;

#define RET_PARAM(type, p, pos) ret[p] = boost::lexical_cast<type>(params.at(pos))
        RET_PARAM(int, "n_configs", 1);
        RET_PARAM(int, "n_locs", 2);
        RET_PARAM(int, "n_locs_uniq", 3);

        RET_PARAM(int, "n_cache_hit", 4);
        RET_PARAM(int, "n_locs_total", 5);

        RET_PARAM(int, "repeat_id", 6);
        RET_PARAM(int, "iter", 7);

        RET_PARAM(int, "t_total", 8);
        RET_PARAM(int, "t_runner", 9);
        ret["t_search"] = boost::any_cast<int>(ret["t_total"]) - boost::any_cast<int>(ret["t_runner"]);
        RET_PARAM(int, "t_runner_total", 10);

        RET_PARAM(std::string, "hash", 11);
#undef RET_PARAM
        return ret;
    }

    double calc_mcc(double tp, double tn, double fp, double fn) {
        double nom = tp * tn - fp * fn;
        double denom = sqrt((tp + fp) * (tp + fn) * (tn + fp) * (tn + fn));
        if (denom == 0) denom = 1;

        return nom / denom;
    }

    double calc_fscore(double tp, double fp, double fn) {
        double p = (tp + fp == 0) ? 0 : double(tp) / (tp + fp);
        double r = (tp + fn == 0) ? 0 : double(tp) / (tp + fn);
        double f = (r + p == 0) ? 0 : double(2 * r * p) / (r + p);
        return f;
    }

    // calculate MCC
    // https://en.wikipedia.org/wiki/Matthews_correlation_coefficient
    map<str, boost::any> run_analyze_3() {
        auto finp = get_inp();
        CHECK_EQ(finp.size(), 2) << "Need two input files to compare";
        vec<str> gcmt, params;
        auto m_truth = read_file(finp.at(0));
        auto m_predicate = read_file(finp.at(1), &gcmt);
        boost::algorithm::split_regex(params, gcmt.at(2), boost::regex("[ |]+"));

        double cspace = dom()->config_space();
        vec<PMutConfig> all_confs = dom()->gen_all_configs();
        CHECK_EQ(cspace, all_confs.size());

        double total_mcc = 0, total_fscore = 0;
        int cnt_interactions = 0, cnt_exact = 0;
        std::stringstream wrong_locs;
        for (const auto &p : m_truth) {
            if (!p.second.is_first) continue;
            cnt_interactions++;
            double mcc = 0, fscore = 0;
            auto _pred = m_predicate.find(p.first);
            if (_pred == m_predicate.end()) {
                // Loc not found
                wrong_locs << '!' << p.first << ' ';
                mcc = fscore = 0;
            } else {
                const auto &truth = p.second, &pred = _pred->second;
                if (count_models(truth.e != pred.e, 1) == 0) {
                    mcc = fscore = 1;
                    cnt_exact++;
                } else {
                    wrong_locs << p.first << ' ';
                    double tp = 0, tn = 0, fp = 0, fn = 0;
                    truth.tree->build_interpreter(), pred.tree->build_interpreter();
                    for (const auto &c : all_confs) {
                        bool v_truth = truth.tree->interpret(*c), v_pred = pred.tree->interpret(*c);
                        if (v_truth && v_pred) tp++;
                        else if (!v_truth && !v_pred) tn++;
                        else if (!v_truth && v_pred) fp++;
                        else if (v_truth && !v_pred) fn++;
                    }
                    VLOG(1, "tp, tn, fp, fn = {:10G}, {:10G}, {:10G}, {:10G}", tp, tn, fp, fn);
                    CHECK_EQ(tp + tn + fp + fn, cspace);
                    mcc = calc_mcc(tp, tn, fp, fn);
                    fscore = calc_fscore(tp, fp, fn);
                }
            }
            total_mcc += mcc, total_fscore += fscore;
            if (mcc != 1 || fscore != 1) LOG(INFO, "Loc {}: mcc {}, fscore {}", p.first, mcc, fscore);
        }

        map<str, boost::any> ret;
        ret["avg_mcc"] = (total_mcc / cnt_interactions);
        ret["avg_f"] = (total_fscore / cnt_interactions);
        ret["cnt_interactions"] = cnt_interactions;
        ret["cnt_exact"] = cnt_exact;
        ret["cnt_wrong"] = cnt_interactions - cnt_exact;
        ret["delta_locs"] = sz(m_predicate) - sz(m_truth);
        ret["n_configs"] = boost::lexical_cast<int>(params.at(1));
        ret["wrong_locs"] = wrong_locs.str();
        return ret;
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
            case 2:
                return run_analyze_2();
            case 3:
                return run_analyze_3();
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