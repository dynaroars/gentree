//
// Created by KH on 3/12/2020.
//

#include "GCovRunner.h"

#include <fstream>
#include <filesystem>

#include <boost/algorithm/string.hpp>
#include <boost/container/flat_map.hpp>
#include <boost/container/flat_set.hpp>
#include <klib/print_stl.h>
#include <klib/vecutils.h>

#ifdef __MINGW32__
typedef unsigned long DWORD;
#   ifndef __kernel_entry
#       define __kernel_entry
#   endif
#endif

#include <boost/process/child.hpp>
#include <boost/process/pipe.hpp>
#include <boost/process/io.hpp>
#include <boost/process/args.hpp>
#include <boost/process/start_dir.hpp>
#include <boost/process/env.hpp>

namespace bp = boost::process;


namespace igen {

void intrusive_ptr_release(GCovRunner *g) {
    boost::sp_adl_block::intrusive_ptr_release(g);
}

GCovRunner::GCovRunner(PMutContext ctx) : Object(move(ctx)) {

}

void GCovRunner::parse(const str &filename) {
    map<str, str> varmap;
    parse(filename, varmap);
}

void GCovRunner::parse(const str &filename, map<str, str> &varmap) {
    std::ifstream ifs(filename);
    if (!ifs) throw std::runtime_error(fmt::format("open file error: {}", filename));
    str line;
    auto procval = [&varmap = std::as_const(varmap)](str &res) {
        for (const auto &p : varmap) {
            boost::replace_all(res, "{" + p.first + "}", p.second);
        }
    };
    auto readval = [&procval](std::istream &s, bool allow_empty = false) -> str {
        str res;
        s >> std::ws;
        std::getline(s, res);
        boost::algorithm::trim(res);
        if (!allow_empty && res.empty())
            throw std::runtime_error("readval(): empty argument");
        procval(res);
        return res;
    };
    auto readkv = [&readval](std::istream &ss, bool allow_empty = false) -> std::pair<str, str> {
        str k;
        ss >> k;
        str v = readval(ss, allow_empty);
        if (!ss || k.empty()) throw std::runtime_error("invalid kv");
        return {k, v};
    };
    auto readargs = [&procval](std::istream &ss) -> vec<str> {
        vec<str> res;
        str entry;
        while (ss >> entry) {
            boost::algorithm::trim(entry);
            if (entry.empty()) continue;
            procval(entry);
            res.emplace_back(move(entry));
        }
        return res;
    };
    while (std::getline(ifs, line)) {
        boost::algorithm::trim(line);
        if (line.empty() || line[0] == '#') continue;
        std::stringstream ss(line);
        str cmd;
        ss >> cmd;
        if (cmd == "include") {
            auto f = std::filesystem::path(filename).parent_path() / readval(ss);
            parse(f.string(), varmap);
        } else if (cmd == "var") {
            auto[k, v] = readkv(ss);
            if (varmap.contains(k))
                throw std::runtime_error(fmt::format("duplicated var: {}", k));
            varmap[k] = v;
        } else if (cmd == "env") {
            auto[k, v] = readkv(ss);
            if (bp_env.count(k))
                throw std::runtime_error(fmt::format("duplicated env: {}", k));
            bp_env[k] = v;
        } else if (cmd == "bin") {
            f_bin = readval(ss);
        } else if (cmd == "wd") {
            f_wd = readval(ss);
        } else if (cmd == "run") {
            cmds.push_back({Cmd::Run, readargs(ss)});
        } else if (cmd == "cleandir") {
            cmds.push_back({Cmd::CleanDir, readargs(ss)});
        } else if (cmd == "touch") {
            cmds.push_back({Cmd::Touch, readargs(ss)});
        } else {
            throw std::runtime_error(fmt::format("invalid command: {}", cmd));
        }
    }
}

void GCovRunner::exec(const vec<str> &config_values) {
    for (const auto &ce : cmds) {
        switch (ce.cmd) {
            case Cmd::Run: {
                vec<str> run_arg;
                run_arg.reserve(config_values.size() + ce.args.size());
                for (const str &s : ce.args) {
                    if (s == "{}") vec_append(run_arg, config_values);
                    else run_arg.emplace_back(s);
                }
                VLOG(60, "Run: {} {}", f_bin, fmt::join(run_arg, " "));

                bp::ipstream out, err;
                bp::child proc_child(f_bin, bp::args(run_arg), bp::start_dir(f_wd), bp::env(bp_env),
                                     bp::std_out > out, bp::std_err > err);
                CHECKF(proc_child, "Error running process {}, args {}", f_bin, run_arg);
                proc_child.wait();
                LOG(INFO, "ec = {}", proc_child.exit_code()) << out.rdbuf();
                //GLOG(INFO) << err.rdbuf();
                break;
            }
            case Cmd::CleanDir: {
                break;
            }
            case Cmd::Touch: {
                for (const str &s : ce.args) {
                    VLOG(60, "Touch file: {}", s);
                    std::ofstream ofs(s);
                }
                break;
            }
        }
    }
}

set<str> GCovRunner::collect_cov() {
    return set<str>();
}

void GCovRunner::clean_cov() {

}


}