//
// Created by kh on 4/4/20.
//

#include "Algo.h"

#include <igen/Context.h>
#include <igen/Domain.h>
#include <igen/Config.h>
#include <igen/runner/ProgramRunnerMt.h>
#include <igen/CoverageStore.h>
#include <igen/c50/CTree.h>

#include <klib/print_stl.h>

namespace igen {


class DemoAlgo : public Object {
public:
    explicit DemoAlgo(PMutContext ctx) : Object(move(ctx)) {}

    PMutCTree tree;
    vec<PCNode> leaves;
    str xloc = "L8";

    void build() {
        print_conf_tbl();

        tree = new CTree(ctx()), tree->prepare_data(cov()->loc(xloc)), tree->build_tree();
        leaves.clear(), tree->gather_nodes(leaves);
        std::sort(leaves.begin(), leaves.end(), [](const PCNode &a, const PCNode &b) {
            if (a->n_min_cases() != b->n_min_cases())
                return a->n_min_cases() < b->n_min_cases();
            else
                return a->depth() > b->depth();
        });

        LOG(INFO, "TREE = \n") << *tree;
    }

    void run_cex(int n) {
        for (int i = 0; i < n; ++i) {
            auto cex = leaves.at(i)->gen_one_convering_configs();
            for (const auto &c : cex) config(c);
        }
    }

    void demo_ex_paper() {
        {
            LOG(INFO, "{:=^80}", " Init ");
            config("0,1,1,0,0,0,1,2,1");
            config("2,0,1,1,0,2,0,0,2"); // H
            config("1,1,0,1,1,1,2,1,0");
            build();
        }

        {
            LOG(INFO, "{:=^80}", " Ite 1 ");
            //        s
            config("0,0,1,1,1,1,1,0,1");
            config("1,0,0,0,0,0,2,2,0");
            config("2,0,1,0,0,2,0,1,2");
            config("2,0,1,0,1,0,0,1,2"); // H
            build();
        }


        // x, s, t, u, v
        // x=2 & s=0 & ((u=1 & v=0) | (u=0 & v=1))
        {
            LOG(INFO, "{:=^80}", " Ite 3 ");
            //      x     u v
            config("2,0,0,0,0,0,0,2,2");
            config("2,1,1,0,0,2,1,1,0");
            config("2,1,0,0,0,1,2,0,1");
            //      x     u v
            config("2,0,0,0,1,2,2,1,0"); // H
            config("2,0,0,0,1,1,0,2,1"); // H
            config("2,1,1,0,1,0,1,0,2");
            //      x     u
            config("2,0,1,1,1,1,2,1,0");
            config("2,1,0,1,0,0,0,2,1");
            config("2,1,0,1,0,2,1,0,2");
            build();
        }

        {
            LOG(INFO, "{:=^80}", " Ite 4 ");
            run_cex(2);
            //build();
        }

        LOG(INFO, "{:=^80}", " EXPR ");
        auto expr = tree->build_zexpr(CTree::FreeMix);
        expr = ctx()->zctx_solver_simplify(expr);
        LOG(INFO, "\n{}", expr.to_string());
        LOG(INFO, "{:=^80}", " END ");
        LOG(INFO, "ran_hashes = {}", ran_hashes.size());
    }

    void demo_diff_c50() {
        tbl_order = {"s", "t", "z"};
        LOG(INFO, "{:=^80}", " Init ");
        config("0,0,0");
        config("0,0,1");
        config("0,0,2");
        config("0,0,3");
        config("0,0,4");
        config("0,1,0");
        config("0,1,1");
        config("0,1,2");
        config("0,1,3");
        config("0,1,4");
        config("1,0,0");
        config("1,0,1");
        config("1,0,2");
        config("1,0,3");
        config("1,0,4");
        config("1,1,0");
        config("1,1,1");
        config("1,1,2");
        config("1,1,3");
        config("1,1,4");
        build();
    }
    void demo_diff_c50_2() {
        tbl_order = {"s", "t", "z"};
        LOG(INFO, "{:=^80}", " Init ");
        config("0,0,0");
        config("0,0,1");
        config("0,0,2");
        config("0,1,0");
        config("0,1,3");
        config("0,1,4");
        config("1,0,1");
        config("1,0,2");
        config("1,0,3");
        config("1,1,0");
        config("1,1,1");
        config("1,1,2");
        config("1,1,3");
        config("1,1,4");
        build();
    }
    void demo_bdd() {
        tbl_order = {};
        xloc = "L1";
        LOG(INFO, "{:=^80}", " Init ");
        config("0,0,0,0,0,0,0,0,0,0,0");
        config("1,0,0,0,0,0,0,0,0,0,0");
        config("1,1,0,0,0,0,0,0,0,0,0");
        config("1,0,1,0,0,0,0,0,0,0,0");
        config("1,0,0,1,0,0,0,0,0,0,0");
        config("1,0,0,0,1,0,0,0,0,0,0");
        config("1,0,0,0,0,1,0,0,0,0,0");
        config("1,0,0,0,0,0,1,0,0,0,0");
        config("1,0,0,0,0,0,0,1,0,0,0");
        config("1,0,0,0,0,0,0,0,1,0,0");
        config("1,0,0,0,0,0,0,0,0,1,0");
        config("1,0,0,0,0,0,0,0,0,0,1");
        build();

        LOG(INFO, "{:=^80}", " EXPR ");
        auto expr = tree->build_zexpr(CTree::FreeMix);
        expr = ctx()->zctx_solver_simplify(expr);
        LOG(INFO, "\n{}", expr.to_string());
        LOG(INFO, "{:=^80}", " END ");
        LOG(INFO, "ran_hashes = {}", ran_hashes.size());
    }
    map<str, boost::any> run_alg() {
        switch (ctx()->get_option_as<int>("alg-version")) {
            case 0:
                demo_ex_paper();
                break;
            case 1:
                demo_diff_c50();
                break;
            case 2:
                demo_diff_c50_2();
                break;
            case 20:
                demo_bdd();
                break;
            default:
                CHECK(0);
        }
        return {};
    }


    set<hash_t> ran_hashes;

    std::stringstream conf_ss;
    int cid = 0;
    vec<str> tbl_order = {"s", "t", "u", "v", "a", "b", "c", "d", "e"};

    PMutConfig config(PMutConfig c) {
        CHECK(ran_hashes.insert(c->hash()).second) << " # " << c->to_str_raw();
        auto loc_names = ctx()->runner()->run({c}).at(0);
        cov_mut()->register_cov(c, loc_names);
        VLOG(50, "{}  ==>  ", *c) << loc_names << "   # " << c->to_str_raw();

        {
            std::stringstream q;
            for (const str &s : tbl_order) {
                for (int i = 0, nv = dom()->n_vars(); i < nv; ++i) {
                    if (dom()->var(i)->name() == s) {
                        q << c->get(i) << " & ";
                        break;
                    }
                }
            }
            fmt::print(conf_ss, "\nc_{{{}}} & {} {}", ++cid, q.str(), fmt::join(loc_names, ","));
            if (tree) {
                bool tval = tree->test_config(c).first;
                conf_ss << " & " << (tval != loc_names.count(xloc) ? "Y" : "N");
            }
            conf_ss << " \\\\";
        }

        return c;
    }

    PMutConfig config(const str &s) {
        return config(new Config(ctx_mut(), s));
    }

    void print_conf_tbl() {
        if (tbl_order.empty()) return;
        std::stringstream q;
        q << "\\text{config} & ";
        for (const str &s : tbl_order) fmt::print(q, "{} & ", s);
        q << "\\text{coverage}";
        if (tree) q << " & \\text{cex?}";
        LOG(INFO, "\n{} \\\\\n\\midrule{}", q.str(), conf_ss.str());
        conf_ss = {};
    }
};

map<str, boost::any> run_demo_algo(const map<str, boost::any> &opts) {
    map<str, boost::any> ret;
    PMutContext ctx = new Context();
    ctx->set_options(opts);
    ctx->set_option("runner", str("builtin"));
    switch (ctx->get_option_as<int>("alg-version")) {
        case 0:
            ctx->set_option("filestem", str("@ex_paper"));
            break;
        case 1:
        case 2:
            ctx->set_option("filestem", str("@ex_paper_diff"));
            break;
        case 20:
            ctx->set_option("filestem", str("@ex_bdd"));
            break;
        default:
            CHECK(0);
    }
    ctx->set_option("cache", str("x"));
    ctx->init();
    ctx->init_runner();
    ctx->runner()->reset_local_timer();
    {
        ptr<DemoAlgo> ite_alg = new DemoAlgo(ctx);
        ret = ite_alg->run_alg();
    }
    ctx->cleanup();
    return ret;
}

}