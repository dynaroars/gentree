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
#include <pugixml.hpp>

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
#define PRINT_VERBOSE 0


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
        } else if (cmd == "cov_env") {
            auto[k, v] = readkv(ss);
            cov_env[k] = v;
        } else if (cmd == "env") {
            auto[k, v] = readkv(ss);
            prog_env[k] = v;
        } else if (cmd == "bin") {
            f_bin = readval(ss);
            f_gcov_prog_name = std::filesystem::path(f_bin).filename();
        } else if (cmd == "wd") {
            f_wd = readval(ss);
        } else if (cmd == "cov_wd") {
            f_cov_wd = readval(ss);
        } else if (cmd == "cov_bin") {
            f_cov_bin = readval(ss);
        } else if (cmd == "loc_trim_prefix") {
            f_loc_trim_prefix.emplace_back(readval(ss));
        } else if (cmd == "run") {
            cmds.push_back({Cmd::Run, readargs(ss)});
        } else if (cmd == "cleandir") {
            cmds.push_back({Cmd::CleanDir, readargs(ss)});
        } else if (cmd == "clean_wd") {
            cmds.push_back({Cmd::CleanWd, readargs(ss)});
        } else if (cmd == "touch") {
            cmds.push_back({Cmd::Touch, readargs(ss)});
        } else if (cmd == "cp_replace_folder") {
            const auto &a = readargs(ss);
            if (a.size() != 2) throw std::runtime_error(fmt::format("invalid cp_replace_foldercommand: {}", a));
            cp_replace_folder_cmds.emplace_back(a[0], a[1]);
        } else if (cmd == "lang") {
            lang = Language::_from_string_nocase(readval(ss).c_str());
        } else if (cmd == "python_bin") {
            f_python_bin = readval(ss);
        } else if (cmd == "perl_bin") {
            f_perl_bin = readval(ss);
        } else if (cmd == "cov_arg") {
            auto args = readargs(ss);
            vec_move_append(f_cov_args, args);
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
                switch (lang) {
                    case Language::Cpp:
                        _run_cpp(std::move(run_arg));
                        break;
                    case Language::Python:
                        _run_py(std::move(run_arg));
                        break;
                    case Language::Ocaml:
                        _run_ocaml(std::move(run_arg));
                        break;
                    case Language::Perl:
                        _run_perl(std::move(run_arg));
                        break;
                    default:
                        CHECK(0);
                }
                break;
            }
            case Cmd::CleanWd: {
                std::filesystem::remove_all(f_wd);
                std::filesystem::create_directory(f_wd);
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
    switch (lang) {
        case Language::Cpp:
            return _collect_cov_cpp();
        case Language::Python:
            return _collect_cov_py();
        case Language::Ocaml:
            return _collect_cov_ocaml();
        case Language::Perl:
            return _collect_cov_perl();
        default:
            CHECK(0);
    }
}


void GCovRunner::clean_cov() {
    switch (lang) {
        case Language::Cpp:
            return _clean_cov_cpp();
        case Language::Python:
            return _clean_cov_py();
        case Language::Ocaml:
            return _clean_cov_ocaml();
        case Language::Perl:
            return _clean_cov_perl();
        default:
            CHECK(0);
    }
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

    CHECK(boost::algorithm::starts_with(f_cov_wd, allowed_prefix));
    switch (lang) {
        case Language::Cpp:
            f_gcov_gcda_file = f_cov_wd + "/" + f_gcov_prog_name + ".gcda";
            break;
        case Language::Python:
            f_python_cov_file = f_cov_wd + "/.coverage";
            break;
        case Language::Ocaml:
            f_ocaml_cov_file = f_cov_wd + "/bisect0001.out";
            break;
        case Language::Perl:
            f_dir_perl_cover_db = f_cov_wd + "/cover_db";
            break;
        default:
            break;
    }

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

void GCovRunner::_trim_file_prefix(str &file_str) const {
    for (const auto &prefix : f_loc_trim_prefix) {
        if (boost::algorithm::starts_with(file_str, prefix)) {
            file_str.erase(file_str.begin(), file_str.begin() + sz(prefix));
            break;
        }
    }
}

//======================================================================================================================

void GCovRunner::_run_cpp(vec<str> args) {
#if PRINT_VERBOSE
    bp::ipstream out, err;
#endif
    bp::child proc_child(
            f_bin, bp::posix::use_vfork, bp::posix::sig.ign(),
            bp::args(args), bp::start_dir(f_wd), bp::env(prog_env),
#if PRINT_VERBOSE
            bp::std_out > out, bp::std_err > err
#else
            bp::std_out > bp::null, bp::std_err > bp::null
#endif
    );
    CHECKF(proc_child, "Error running process: {} {}", f_bin, fmt::join(args, " "));
    proc_child.wait();
    //LOG(INFO, "ec = {}", proc_child.exit_code()) << out.rdbuf();
    //GLOG(INFO) << err.rdbuf();
#if PRINT_VERBOSE
    str str_out = read_stream_to_str(out);
    LOG(INFO, "Out (ec={}): {}", proc_child.exit_code(), str_out);
    str str_err = read_stream_to_str(err);
    LOG(INFO, "Err: {}", str_err);
#endif
    // TODO: Check stderr
}

set<str> GCovRunner::_collect_cov_cpp() {
    vec<str> run_arg = {"-it", f_gcov_prog_name};

    bp::ipstream out, err;
    bp::child proc_child(f_cov_bin, bp::posix::use_vfork, bp::posix::sig.ign(),
                         bp::args(run_arg), bp::start_dir(f_cov_wd),
                         bp::std_out > out, bp::std_err > err);
    CHECKF(proc_child, "Error running gcov: {} {}", f_cov_bin, fmt::join(run_arg, " "));

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
        _trim_file_prefix(file_str);

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

void GCovRunner::_clean_cov_cpp() {
    std::filesystem::remove(f_gcov_gcda_file);
}

//======================================================================================================================

void GCovRunner::_run_py(vec<str> args) {
    vec<str> pre_args{f_cov_bin, "run"};
    pre_args.insert(pre_args.end(), f_cov_args.begin(), f_cov_args.end());
    pre_args.emplace_back(f_bin);
    args.insert(args.begin(), pre_args.begin(), pre_args.end());

#if PRINT_VERBOSE
    bp::ipstream out, err;
#endif
    bp::child proc_child(
            f_python_bin, bp::posix::use_vfork, // bp::posix::sig.ign(),
            bp::args(args), bp::start_dir(f_wd), bp::env(prog_env),
#if PRINT_VERBOSE
            bp::std_out > out, bp::std_err > err
#else
            bp::std_out > bp::null, bp::std_err > bp::null
#endif
    );
    CHECKF(proc_child, "Error running process: {} {}", f_bin, fmt::join(args, " "));
    proc_child.wait();
    //LOG(INFO, "ec = {}", proc_child.exit_code()) << out.rdbuf();
    //GLOG(INFO) << err.rdbuf();
#if PRINT_VERBOSE
    str str_out = read_stream_to_str(out);
    LOG(INFO, "Out (ec={}): {}", proc_child.exit_code(), str_out);
    str str_err = read_stream_to_str(err);
    LOG(INFO, "Err: {}", str_err);
#endif
    // TODO: Check stderr
}

set<str> GCovRunner::_collect_cov_py() {
    vec<str> run_arg = {f_cov_bin, "json", "-o-"};

    bp::ipstream out, err;
    bp::child proc_child(f_python_bin, bp::posix::use_vfork, bp::posix::sig.ign(),
                         bp::args(run_arg), bp::start_dir(f_cov_wd), bp::env(cov_env),
                         bp::std_out > out, bp::std_err > err);
    CHECKF(proc_child, "Error running gcov: {} {}", f_cov_bin, fmt::join(run_arg, " "));

    using namespace rapidjson;
    set<str> res;
    str json_str = read_stream_to_str(out);

    std::string serr = read_stream_to_str(err);
    // TODO: Check error
    if (!serr.empty()) {
        CHECKF(0, "Py-cov error (ec = {}). Stderr = \n", proc_child.exit_code()) << serr;
    }
    //====

    int cur_n_locs = 0;
    Document document;
    document.ParseInsitu(json_str.data());

    const Value &files = document["files"];
    CHECK(files.IsObject());
    for (auto &file_entry : files.GetObject()) {
        str file_str = file_entry.name.GetString();
        _trim_file_prefix(file_str);

        const Value &lines = file_entry.value["executed_lines"];
        CHECK(lines.IsArray());
        for (auto &v : lines.GetArray()) {
            CHECK(v.IsInt());
            int lnum = v.GetInt();

            str loc = file_str;
            loc.push_back(':');
            loc += std::to_string(lnum);
            res.insert(move(loc));
        }

        const auto &summary = file_entry.value["summary"];
        CHECK(summary.IsObject());
        cur_n_locs += summary.GetObject()["num_statements"].GetInt();
    }

    if (n_locs_ == -1)
        n_locs_ = cur_n_locs;
    else
        CHECK_EQ(n_locs_, cur_n_locs);
    proc_child.wait();
    return res;
}

void GCovRunner::_clean_cov_py() {
    std::filesystem::remove(f_python_cov_file);
}


//======================================================================================================================

void GCovRunner::_run_ocaml(vec<str> args) {
#if PRINT_VERBOSE
    bp::ipstream out, err;
#endif
    bp::child proc_child(
            f_bin, bp::posix::use_vfork, // bp::posix::sig.ign(),
            bp::args(args), bp::start_dir(f_wd), bp::env(prog_env),
#if PRINT_VERBOSE
            bp::std_out > out, bp::std_err > err
#else
            bp::std_out > bp::null, bp::std_err > bp::null
#endif
    );
    CHECKF(proc_child, "Error running process: {} {}", f_bin, fmt::join(args, " "));
    proc_child.wait();
    //LOG(INFO, "ec = {}", proc_child.exit_code()) << out.rdbuf();
    //GLOG(INFO) << err.rdbuf();
#if PRINT_VERBOSE
    str str_out = read_stream_to_str(out);
    LOG(INFO, "Out (ec={}): {}", proc_child.exit_code(), str_out);
    str str_err = read_stream_to_str(err);
    LOG(INFO, "Err: {}", str_err);
#endif
    // TODO: Check stderr
}

set<str> GCovRunner::_collect_cov_ocaml() {
    vec<str> run_arg = {"-xml", "-", f_ocaml_cov_file};
    run_arg.insert(run_arg.end(), f_cov_args.begin(), f_cov_args.end());

    bp::ipstream out, err;
    bp::child proc_child(f_cov_bin, bp::posix::use_vfork, bp::posix::sig.ign(),
                         bp::args(run_arg), bp::start_dir(f_cov_wd),
                         bp::std_out > out, bp::std_err > err);
    CHECKF(proc_child, "Error running Ocaml bisect: {} {}", f_cov_bin, fmt::join(run_arg, " "));

    set<str> res;
    str xml_str = read_stream_to_str(out);

    std::string serr = read_stream_to_str(err);
    // TODO: Check error
    if (!serr.empty()) {
        CHECKF(0, "Ocaml bisect error (ec = {}). Stderr = \n", proc_child.exit_code()) << serr;
    }
    //====

    using namespace pugi;
    xml_document doc;
    xml_parse_result result = doc.load_buffer_inplace(xml_str.data(), xml_str.size());
    CHECKF(result, "Parse XML error: description: {}, offset: {}", result.description(), result.offset);


    int cur_n_locs = 0;
    xml_node bisect_report = doc.child("bisect-report");
    for (xml_node file : bisect_report.children("file")) {
        str path = file.attribute("path").value();
        CHECK(!path.empty());
        _trim_file_prefix(path);
        for (xml_node point : file.children("point")) {
            int offset = point.attribute("offset").as_int(-1);
            int count = point.attribute("count").as_int(-1);
            CHECK(offset != -1 && count != -1);
            if (count > 0) {
                str loc = path;
                loc.push_back(':');
                loc += std::to_string(offset);
                res.insert(move(loc));
            }
            cur_n_locs++;
        }
    }

    n_locs_ = std::max(n_locs_, cur_n_locs);
//    if (n_locs_ == -1)
//        n_locs_ = cur_n_locs;
//    else
//        CHECK_EQ(n_locs_, cur_n_locs);
    proc_child.wait();
    return res;
}

void GCovRunner::_clean_cov_ocaml() {
    std::filesystem::remove(f_ocaml_cov_file);
}

//======================================================================================================================

void GCovRunner::_run_perl(vec<str> _args) {
    vec<str> args{"-MDevel::Cover=-coverage,statement", f_bin};
    vec_move_append(args, _args);

#if PRINT_VERBOSE
    bp::ipstream out, err;
#endif
    bp::child proc_child(
            f_perl_bin, bp::posix::use_vfork, bp::posix::sig.ign(),
            bp::args(args), bp::start_dir(f_wd), bp::env(prog_env),
#if PRINT_VERBOSE
            bp::std_out > out, bp::std_err > err
#else
            bp::std_out > bp::null, bp::std_err > bp::null
#endif
    );
    CHECKF(proc_child, "Error running process: {} {}", f_perl_bin, fmt::join(args, " "));
    proc_child.wait();
    //LOG(INFO, "ec = {}", proc_child.exit_code()) << out.rdbuf();
    //GLOG(INFO) << err.rdbuf();
#if PRINT_VERBOSE
    str str_out = read_stream_to_str(out);
    LOG(INFO, "Out (ec={}): {}", proc_child.exit_code(), str_out);
    str str_err = read_stream_to_str(err);
    LOG(INFO, "Err: {}", str_err);
#endif
    // TODO: Check stderr
}

set<str> GCovRunner::_collect_cov_perl() {
    static const vec<str> run_arg = {"-report", "sort"};

    bp::ipstream out, err;
    bp::child proc_child(f_cov_bin, bp::posix::use_vfork, bp::posix::sig.ign(),
                         bp::args(run_arg), bp::start_dir(f_cov_wd),
                         bp::std_out > out, bp::std_err > err);
    CHECKF(proc_child, "Error running Perl cover: {} {}", f_cov_bin, fmt::join(run_arg, " "));

    set<str> res;

    str line;
    str find_pat = f_bin + ":";
    int cur_n_locs = 0;

    str file_str = f_bin;
    _trim_file_prefix(file_str);
    while (std::getline(out, line)) {
        if(sz(line) > sz(find_pat) && line.compare(0, find_pat.size(), find_pat) == 0) {
            str b;
            int verify_nlocs;
            std::stringstream ss(line);
            ss >> b >> b >> verify_nlocs >> b >> b;

            cur_n_locs = sz(b);
            CHECK_EQ(verify_nlocs, cur_n_locs);
            int curLine = 1;
            for(const char &c : b) {
                if(c == '1') {
                    str loc = file_str;
                    loc.push_back(':');
                    loc += std::to_string(curLine);
                    res.insert(move(loc));
                }
                curLine++;
            }
            break;
        }
    }
    while (std::getline(out, line));

    n_locs_ = std::max(n_locs_, cur_n_locs);
    proc_child.wait();
    return res;
}

void GCovRunner::_clean_cov_perl() {
    std::filesystem::remove_all(f_dir_perl_cover_db);
}


}