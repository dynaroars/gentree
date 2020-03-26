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

namespace igen {


class Analyzer : public Object {
public:
    explicit Analyzer(PMutContext ctx) : Object(move(ctx)) {}

    map<std::pair<int, int>, int> map_count_cex;

    struct LocData {
        expr e;
        PMutCTree tree;

        [[nodiscard]] unsigned id() const { return e.id(); }
    };

    static expr to_expr(const z3::model &m) {
        z3::expr_vector vecExpr(m.ctx());
        int nvars = m.num_consts();
        vecExpr.resize(nvars);
        for (int i = 0; i < nvars; ++i) {
            const auto &de = m.get_const_decl(i);
            z3::expr eqExpr = (de() == m.get_const_interp(de));
            vecExpr.set(i, eqExpr);
        }
        return z3::mk_and(vecExpr);
    }

    int count_cex(const expr &a, const expr &b, int lim = 1000) {
        auto it = map_count_cex.find({a.id(), b.id()});
        if (it != map_count_cex.end()) return it->second;
        int &ncex = map_count_cex[{a.id(), b.id()}];

        expr bi = (a != b);
        auto solver = ctx()->zscope();
        solver->add(a != b);

        while (ncex < lim) {
            z3::check_result checkres = solver->check();
            if (checkres == z3::unsat) {
                break;
            } else if (checkres == z3::unknown) {
                LOG(WARNING, "Z3 solver returns unknown:\n") << *solver;
                ncex = lim;
                break;
            }
            CHECK_EQ(checkres, z3::sat);
            ncex++;
            z3::model m = solver->get_model();
            expr mexpr = to_expr(m);
            solver->add(!mexpr);
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
        set<unsigned> sexprid;
        while (getline(f, line)) {
            boost::algorithm::trim(line);
            if (line.empty() || line[0] == '#') continue;
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
                CHECKF(sexprid.insert(e.id()).second, "Duplicated expression ({})", path);
                //LOG(INFO, "EXPR: ") << e;
                expr tree_e = tree->build_zexpr(CTree::FreeMix);
                CHECKF(count_cex(e, tree_e, 1) == 0, "Mismatch tree and expr: {}", path);
                for (const str &s : locs) {
                    CHECKF(!res.contains(s), "Duplicated location ({}): {}", path, s);
                    res.emplace(s, LocData{e, tree});
                }
                read_state = 0, locs.clear(), sexpr.clear(), stree.clear();
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
        LOG(INFO, "Read {} locs from {}", res.size(), path);
        return res;
    }

    // compare two output
    void run_analyzer_0() {
        auto finp = get_inp();
        CHECK_EQ(finp.size(), 2) << "Need two input files to compare";
        auto ma = read_file(finp.at(0));
        auto mb = read_file(finp.at(1));
        set<unsigned> sdiff, smissing, slocsa, slocsb, sprintdiff;
        int cntdiff = 0, cntmissing = 0, totalcex = 0;
        for (const auto &p : ma) {
            slocsa.insert(p.second.id());
            auto it = mb.find(p.first);
            if (it == mb.end()) {
                VLOG(5, "{} not found in B", p.first);
                smissing.insert(p.second.id()), cntmissing++;
                continue;
            }
            int num_cex = count_cex(p.second.e, it->second.e);
            if (num_cex == 0) {
                //VLOG(0, "{} ok", p.first);
            } else {
                VLOG(5, "{} diff (cex = {}) ({})", p.first, num_cex, p.second.id());
                if (sprintdiff.insert(p.second.id()).second) {
                    totalcex += num_cex;
                    GVLOG(10) << "\nA: " << p.second.e << "\nB: " << it->second.e;
                }
                sdiff.insert(p.second.id()), cntdiff++;
            }
        }
        for (const auto &p : mb) {
            slocsb.insert(p.second.id());
            if (!ma.contains(p.first)) {
                VLOG(5, "{} not found in A", p.first);
                smissing.insert(p.second.id()), cntmissing++;
            }
        }
        LOG(INFO, "{:=^80}", "  FINAL RESULT  ");
        LOG(INFO, "Total: diff {:>4}, miss {:>4}, locs A {:>4}, B {:>4}", cntdiff, cntmissing, ma.size(), mb.size());
        LOG(INFO, "Uniq : diff {:>4}, miss {:>4}, locs A {:>4}, B {:>4}, cex {:>4}", sdiff.size(), smissing.size(),
            slocsa.size(), slocsb.size(), totalcex);
    }

    // try random conf
    void run_analyze_1() {
        ctx()->runner()->init();

        auto finp = get_inp();
        CHECK_EQ(finp.size(), 1) << "Need 1 input file";
        auto ma = read_file(finp.at(0));
        int n_iter = ctx()->get_option_as<int>("rounds");
        int n_batch = ctx()->get_option_as<int>("batch-size");
        if (n_batch == 0) n_batch = 100;
        CHECK_GT(n_batch, 0);

        set<hash_t> all_configs, wrong_configs;
        set<str> wrong_locs, wrong_locs_uniq, empty_set, missing_locs;

        vec<PMutConfig> batch_confs(n_batch);
        vec<set<str>> batch_locs;
        for (auto &c : batch_confs) c = new Config(ctx_mut());
        int it = n_batch;

        for (int iter = 1; iter <= n_iter; ++iter) {
            if (it == n_batch) {
                for (int i = 0; i < n_batch; ++i) {
                    const PMutConfig &c = batch_confs[i];
                    do {
                        for (int var = 0; var < dom()->n_vars(); ++var)
                            c->set(var, Rand.get(dom()->n_values(var)));
                    } while (!all_configs.insert(c->hash(true)).second);
                }
                batch_locs = ctx_mut()->runner()->run(batch_confs);
                CHECK_EQ(batch_confs.size(), batch_locs.size());
                it = 0;
            }

            const PMutConfig &c = batch_confs[it];
            const set<str> &e = batch_locs[it];
            ++it;

            map<unsigned, bool> eval_cache;
            for (const auto &p : ma) {
                auto eid = p.second.id();
                auto it = eval_cache.find(eid);
                bool eval_res, uniq_loc;
                if (it == eval_cache.end()) {
                    eval_res = eval_cache[eid] = p.second.tree->test_config(c).first;
                    //CHECK_EQ(eval_res, c->eval(p.second.e));
                    uniq_loc = true;
                } else {
                    eval_res = it->second;
                    uniq_loc = false;
                }

                bool truth = e.contains(p.first);
                if (eval_res != truth) {
                    wrong_configs.insert(c->hash());
                    wrong_locs.insert(p.first);
                    if (uniq_loc) wrong_locs_uniq.insert(p.first);
                }
            }
            for (const auto &s : e) if (!ma.contains(s)) missing_locs.insert(s);
            if (it == 1) {
                LOG(INFO, "{:>4} {:>4} {:>4} | {:>4} {:>4} {:>4} | ",
                    all_configs.size(), wrong_configs.size(), ctx()->runner()->n_cache_hit(),
                    wrong_locs_uniq.size(), wrong_locs.size(), missing_locs.size())
                        << (sz(wrong_locs_uniq) <= 10 ? wrong_locs_uniq : empty_set)
                        << (sz(missing_locs) <= 10 ? missing_locs : empty_set);
            }
        }
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

    void run_alg() {
        switch (ctx()->get_option_as<int>("alg-version")) {
            case 0:
                return run_analyzer_0();
            case 1:
                return run_analyze_1();
            default:
                CHECK(0);
        }
    }
};


int run_analyzer(const map<str, boost::any> &opts) {
    PMutContext ctx = new Context();
    ctx->set_options(opts);
    ctx->init();
    //ctx->runner()->init();
    {
        ptr<Analyzer> ite_alg = new Analyzer(ctx);
        ite_alg->run_alg();
    }
    ctx->cleanup();
    return 0;
}


}