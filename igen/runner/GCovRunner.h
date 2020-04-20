//
// Created by KH on 3/12/2020.
//

#ifndef IGEN4_GCOVRUNNER_H
#define IGEN4_GCOVRUNNER_H

#include <klib/common.h>
#include <igen/Context.h>

#include <boost/process/environment.hpp>

#include <klib/enum.h>

namespace igen {

namespace details {
BETTER_ENUM(GCovRunnerLanguage, int, Cpp, Python, Ocaml)
}

class GCovRunner : public Object {
public:
    explicit GCovRunner(PMutContext ctx, map<str, str> default_vars = {});

    void init();

    void parse(const str &filename);

    void parse(const str &filename, map<str, str> &varmap);

    void exec(const vec<str> &config_values);

    set<str> collect_cov();

    void clean_cov();

    int n_locs() const { return n_locs_; }

    using Language = details::GCovRunnerLanguage;
private:
    void _run_cpp(vec<str> args);

    set<str> _collect_cov_cpp();

    void _clean_cov_cpp();

    void _run_py(vec<str> args);

    set<str> _collect_cov_py();

    void _clean_cov_py();

    void _trim_file_prefix(str &file_str) const;

private:
    str f_bin, f_wd, f_cov_wd, f_cov_bin, f_gcov_prog_name, f_gcov_gcda_file;
    str f_python_bin, f_python_cov_file;
    vec<str> f_loc_trim_prefix;
    map<str, str> vars_;

    boost::process::environment prog_env, cov_env;

    enum class Cmd {
        Run, CleanDir, Touch
    };
    struct CmdEntry {
        Cmd cmd;
        vec<str> args;
    };
    vec<CmdEntry> cmds;

    Language lang = Language::Cpp;

    vec<std::pair<str, str>> cp_replace_folder_cmds;

    int n_locs_ = -1;
};

}

#endif //IGEN4_GCOVRUNNER_H
