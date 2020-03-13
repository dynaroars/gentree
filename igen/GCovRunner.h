//
// Created by KH on 3/12/2020.
//

#ifndef IGEN4_GCOVRUNNER_H
#define IGEN4_GCOVRUNNER_H

#include <klib/common.h>
#include <igen/Context.h>

#include <boost/process/environment.hpp>

namespace igen {

class GCovRunner : public Object {
public:
    GCovRunner(PMutContext ctx);

    void parse(const str &filename);

    void parse(const str &filename, map<str, str> &varmap);

    void exec(const vec<str> &config_values);

    set<str> collect_cov();

    void clean_cov();

private:
    str f_bin, f_wd, f_gcov_wd, f_gcov_bin, f_gcov_prog_name, f_gcov_gcda_file, f_loc_trim_prefix;

    boost::process::environment bp_env;

    enum class Cmd {
        Run, CleanDir, Touch
    };
    struct CmdEntry {
        Cmd cmd;
        vec<str> args;
    };
    vec<CmdEntry> cmds;
};

}

#endif //IGEN4_GCOVRUNNER_H