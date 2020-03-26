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

#define BOOST_POSIX_HAS_VFORK 1

#include <boost/process/child.hpp>
#include <boost/process/pipe.hpp>
#include <boost/process/io.hpp>
#include <boost/process/args.hpp>
#include <boost/process/start_dir.hpp>
#include <boost/process/env.hpp>
#include <boost/process/posix.hpp>

#include <rapidjson/document.h>
#include <rapidjson/writer.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/istreamwrapper.h>

namespace bp = boost::process;


namespace igen {

void intrusive_ptr_release(GCovRunner *g) {
    boost::sp_adl_block::intrusive_ptr_release(g);
}

GCovRunner::GCovRunner(PMutContext ctx, map<str, str> default_vars) :
        Object(move(ctx)), vars_(move(default_vars)) {
}

void GCovRunner::parse(const str &filename) {
    parse(filename, vars_);
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
        while (ss >> std::ws >> entry) {
            if (!entry.empty() && entry[0] == '\'') {
                entry.erase(entry.begin());
                str cont;
                std::getline(ss, cont, '\'');
                entry += cont;
            }
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
            f_gcov_prog_name = std::filesystem::path(f_bin).filename();
        } else if (cmd == "wd") {
            f_wd = readval(ss);
        } else if (cmd == "gcov_wd") {
            f_gcov_wd = readval(ss);
        } else if (cmd == "gcov_bin") {
            f_gcov_bin = readval(ss);
        } else if (cmd == "loc_trim_prefix") {
            f_loc_trim_prefix = readval(ss);
        } else if (cmd == "run") {
            cmds.push_back({Cmd::Run, readargs(ss)});
        } else if (cmd == "cleandir") {
            cmds.push_back({Cmd::CleanDir, readargs(ss)});
        } else if (cmd == "touch") {
            cmds.push_back({Cmd::Touch, readargs(ss)});
        } else if (cmd == "cp_replace_folder") {
            const auto &a = readargs(ss);
            if (a.size() != 2) throw std::runtime_error(fmt::format("invalid cp_replace_foldercommand: {}", a));
            cp_replace_folder_cmds.emplace_back(a[0], a[1]);
        } else {
            throw std::runtime_error(fmt::format("invalid command: {}", cmd));
        }
    }
}

static std::string read_stream_to_str(std::istream &in) {
    std::string ret;
    char buffer[4096];
    while (in.read(buffer, sizeof(buffer)))
        ret.append(buffer, sizeof(buffer));
    ret.append(buffer, in.gcount());
    return ret;
}

void GCovRunner::exec(const vec<str> &config_values) {
#define PRINT_VERBOSE 0

    for (const auto &ce : cmds) {
        switch (ce.cmd) {
            case Cmd::Run: {
                vec<str> run_arg;
                run_arg.reserve(config_values.size() + ce.args.size());
                for (const str &s : ce.args) {
                    if (s == "{}") vec_append(run_arg, config_values);
                    else run_arg.emplace_back(s);
                }
                LOG_IF(INFO, PRINT_VERBOSE, "Run: {} {}", f_bin, fmt::join(run_arg, " "));

                bp::ipstream out, err;
                bp::child proc_child(
                        f_bin, bp::posix::use_vfork, bp::posix::sig.ign(),
                        bp::args(run_arg), bp::start_dir(f_wd), bp::env(bp_env),
#if PRINT_VERBOSE
                        bp::std_out > out, bp::std_err > err
#else
                        bp::std_out > bp::null, bp::std_err > err
#endif
                );
                CHECKF(proc_child, "Error running process: {} {}", f_bin, fmt::join(run_arg, " "));
                proc_child.wait();
                //LOG(INFO, "ec = {}", proc_child.exit_code()) << out.rdbuf();
                //GLOG(INFO) << err.rdbuf();
#if PRINT_VERBOSE
                str str_out = read_stream_to_str(out);
                VLOG(10, "Out: {}", str_out);
#endif
                str str_err = read_stream_to_str(err);
//                VLOG(10, "Err: {}", str_err);
                // TODO: Check stderr
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
    vec<str> run_arg = {"-it", f_gcov_prog_name};

    bp::ipstream out, err;
    bp::child proc_child(f_gcov_bin, bp::posix::use_vfork, bp::posix::sig.ign(),
                         bp::args(run_arg), bp::start_dir(f_gcov_wd),
                         bp::std_out > out, bp::std_err > err);
    CHECKF(proc_child, "Error running gcov: {} {}", f_gcov_bin, fmt::join(run_arg, " "));

    using namespace rapidjson;
    set<str> res;
    str json_str = read_stream_to_str(out);

    std::string serr = read_stream_to_str(err);
    // TODO: Check error
    if (!serr.empty()) {
        CHECKF(0, "Gcov error (ec = {}). Stderr = \n", proc_child.exit_code()) << serr;
    }
    //====

    int cur_n_locs = 0;
    Document document;
    document.ParseInsitu(json_str.data());

    const Value &files = document["files"];
    CHECK(files.IsArray());
    for (Value::ConstValueIterator itf = files.Begin(); itf != files.End(); ++itf) {
        CHECK(itf->IsObject());
        const auto &obj = itf->GetObject();
        const Value &file = obj["file"];
        CHECK(file.IsString());
        str file_str = file.GetString();
        if (boost::algorithm::starts_with(file_str, f_loc_trim_prefix))
            file_str.erase(file_str.begin(), file_str.begin() + f_loc_trim_prefix.size());

        const Value &lines = obj["lines"];
        CHECK(lines.IsArray());
        for (Value::ConstValueIterator itL = lines.Begin(); itL != lines.End(); ++itL) {
            CHECK(itL->IsObject());
            cur_n_locs++;
            const auto &objL = itL->GetObject();
            if (objL["count"].GetInt() == 0) continue;
            int lnum = objL["line_number"].GetInt();

            str loc = file_str;
            loc.push_back(':');
            loc += std::to_string(lnum);
            res.insert(move(loc));
        }
    }

    if (n_locs_ == -1)
        n_locs_ = cur_n_locs;
    else
        CHECK_EQ(n_locs_, cur_n_locs);
    proc_child.wait();
    return res;
}

void GCovRunner::clean_cov() {
    std::filesystem::remove(f_gcov_gcda_file);
}

static void recursive_copy(const boost::filesystem::path &src, const boost::filesystem::path &dst) {
    namespace fs = boost::filesystem;
    CHECK(fs::is_directory(src) && fs::is_directory(dst));

    for (fs::directory_entry &src_item : fs::directory_iterator(src)) {
        fs::path new_ds = dst / src_item.path().filename();
        if (fs::is_directory(src_item.path())) {
            fs::create_directories(new_ds);
            recursive_copy(src_item.path(), new_ds);
        } else if (fs::is_regular_file(src_item)) {
            fs::copy(src_item, new_ds);
        } else if (fs::is_symlink(src_item)) {
            fs::copy_symlink(src_item, new_ds);
        }
    }
}

void GCovRunner::init() {
    namespace fs = boost::filesystem;
    const str allowed_prefix = "/mnt/ramdisk/";

    CHECK(boost::algorithm::starts_with(f_gcov_wd, allowed_prefix));
    f_gcov_gcda_file = f_gcov_wd + "/" + f_gcov_prog_name + ".gcda";

    CHECK(boost::algorithm::starts_with(f_wd, allowed_prefix));
    fs::remove_all(f_wd);
    fs::create_directories(f_wd);

    for (const auto &p : cp_replace_folder_cmds) {
        CHECK(boost::algorithm::starts_with(p.second, allowed_prefix));
        fs::remove_all(p.second);
        fs::create_directories(p.second);
        recursive_copy(p.first, p.second);
    }

    VLOG(1, "GCovRunner inited");
}


}