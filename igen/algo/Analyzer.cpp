//
// Created by kh on 3/14/20.
//

#include "Algo.h"

#include <igen/Context.h>
#include <igen/Domain.h>
#include <igen/Config.h>
#include <igen/ProgramRunner.h>
#include <igen/CoverageStore.h>

#include <klib/print_stl.h>
#include <klib/vecutils.h>
#include <igen/c50/CTree.h>
#include <fstream>

#include <boost/algorithm/string.hpp>
#include <boost/container/flat_map.hpp>

namespace igen {


class Analyzer : public Object {
public:
    explicit Analyzer(PMutContext ctx) : Object(move(ctx)) {}

    map<std::pair<int, int>, bool> map_is_equiv;

    bool is_equiv(const expr &a, const expr &b, bool def = false) {
        auto it = map_is_equiv.find({a.id(), b.id()});
        if (it != map_is_equiv.end()) return it->second;
        expr bi = (a == b);
        z3::check_result res = ctx()->zsolver().check(1, &bi);
        switch (res) {
            case z3::check_result::sat:
                return map_is_equiv[{a.id(), b.id()}] = map_is_equiv[{b.id(), a.id()}] = true;
            case z3::check_result::unsat:
                return map_is_equiv[{a.id(), b.id()}] = map_is_equiv[{b.id(), a.id()}] = false;
            case z3::check_result::unknown:
                LOG(WARNING, "Z3 solver returns unknown:\n") << ctx()->zsolver() << '\n' << a << '\n' << b;
                return map_is_equiv[{a.id(), b.id()}] = map_is_equiv[{b.id(), a.id()}] = def;
            default:
                abort();
        }
    }

    map<str, expr> read_file(const str &path) {
        map<str, expr> res;
        std::ifstream f(path);
        CHECKF(!f.fail(), "Error reading file: {}", path);
        str line;
        bool read_loc = true;
        vec<str> locs;
        str sexpr;
        while (getline(f, line)) {
            boost::algorithm::trim(line);
            if (line.empty() || line[0] == '#') continue;
            if (line[0] == '-') {
                read_loc = false;
                continue;
            } else if (line[0] == '=') {
                z3::expr_vector evec = ctx()->zctx().parse_string(sexpr.c_str());
                expr e = z3::mk_and(evec);
                for (const str &s : locs) {
                    CHECK(!res.contains(s));
                    res.emplace(s, e);
                }
                read_loc = true, locs.clear(), sexpr.clear();
                continue;
            }
            if (read_loc) {
                for (char &c : line) if (c == ',') c = ' ';
                std::stringstream ss(line);
                str tok;
                while (ss >> tok) locs.emplace_back(move(tok));
            } else {
                sexpr += line, sexpr += '\n';
            }
        }
        return res;
    }

    // compare two output
    void run_analyzer_0() {
        auto finp = get_inp();
        CHECK_EQ(finp.size(), 2) << "Need two input files to compare";
        auto ma = read_file(finp.at(0));
        auto mb = read_file(finp.at(1));
        set<unsigned> sdiff, smissing;
        for (const auto &p : ma) {
            auto it = mb.find(p.first);
            if (it == mb.end()) {
                LOG(INFO, "{} not found in B", p.first);
                smissing.insert(p.second.id());
                continue;
            }
            bool eq = is_equiv(p.second, it->second);
            if (eq) {
                VLOG(0, "{} ok", p.first);
            } else {
                LOG(INFO, "{} diff", p.first);
                GVLOG(0) << "\nA: " << p.second << "\nB:" << it->second;
                sdiff.insert(p.second.id());
            }
        }
        for (const auto &p : mb) {
            if (!ma.contains(p.first)) {
                LOG(INFO, "{} not found in A", p.first);
                smissing.insert(p.second.id());
            }
        }
        LOG(INFO, "{:=^80}", "  FINAL RESULT  ");
        LOG(INFO, "Uniq: diff {}, miss {}", sdiff.size(), smissing.size());
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
            default:
                CHECK(0);
        }
    }
};


int run_analyzer(const boost::program_options::variables_map &vm) {
    PMutContext ctx = new Context();
    for (const auto &kv : vm) {
        ctx->set_option(kv.first, kv.second.value());
    }
    ctx->init();
    //ctx->program_runner()->init();
    {
        ptr<Analyzer> ite_alg = new Analyzer(ctx);
        ite_alg->run_alg();
    }
    ctx->cleanup();
    return 0;
}


}