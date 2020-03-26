#include <iostream>

#include <klib/common.h>
#include <klib/random.h>
#include <z3++.h>

#include <boost/program_options/options_description.hpp>
#include <boost/program_options/variables_map.hpp>
#include <boost/program_options/parsers.hpp>

#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/timer/timer.hpp>
#include <boost/scope_exit.hpp>
#include <boost/filesystem.hpp>

#include <glog/raw_logging.h>

#include <igen/algo/Algo.h>

namespace po = boost::program_options;

void init_glog(int argc, char **argv) {
    (void) argc;
    FLAGS_colorlogtostderr = true;
    //FLAGS_timestamp_in_logfile_name = false;
    FLAGS_max_log_size = 32;
    FLAGS_logbuflevel = google::GLOG_INFO;
    google::SetStderrLogging(0);
    //google::SetVLOGLevel("*", 20);
    google::InitGoogleLogging(argv[0]);
}

int prog(int argc, char *argv[]) {
    using str = std::string;
    po::options_description desc("Dynamic Interaction Inference for Configurable Software");
    desc.add_options()
            ("filestem,F", po::value<str>(), "Filestem")
            ("target", po::value<str>(), "Target")
            ("dom", po::value<str>(), "Dom file")
            ("runner,r", po::value<str>(), "Select program runner")
            ("simple-runner,S", "Simple runner")
            ("builtin-runner,B", "Builtin runner")
            ("gcov-runner,G", "GCov runner")
            ("seed,s", po::value<uint64_t>()->default_value(123), "Random seed")
            ("output,O", po::value<str>(), "Output result")
            ("disj-conj", "Gen expr strat DisjOfConj")
            ("cache,c", po::value<str>()->implicit_value(""), "Use cachedb")
            ("no-cache-read", "Do not read from cachedb")
            ("no-cache-write", "Do not write to cachedb")
            ("cache-only", "Only use cached result")
            ("runner-threads,j", po::value<int>()->default_value(1), "Number of program runner threads")
            ("config-script", po::value<str>()->default_value(""), "JS configure script")

            ("full", "Run with full configs")
            ("rand", po::value<int>(), "Run with random configs")
            ("analyze,A", po::value<int>()->implicit_value(0), "Run anaylzer")
            ("c50,J", po::value<int>()->implicit_value(0), "Run the ML algorithm")
            ("alg-version,T", po::value<int>()->default_value(0), "Select algo version")
            ("batch-size", po::value<int>()->default_value(0), "Batch size")
            ("input,I", po::value<str>()->default_value(""), "Algorithm input")
            ("term-cnt", po::value<int>()->default_value(0), "Termination counter")

            ("loc,X", po::value<str>(), "Interested location")
            ("rounds,R", po::value<int>(), "Number of iterations")
            ("seed-configs,C", po::value<std::vector<str>>()->default_value(std::vector<str>(), "none"), "Seed configs")

            ("vmodule,V", po::value<str>(), "Verbose logging. eg -V mapreduce=2,file=1,gfs*=3")
            ("verbose,v", po::value<int>(), "Verbose level")
            ("no-buf,N", "Do not buffer log")
            ("log-dir,L", po::value<str>(), "Log to file")
            ("help,h", "Print usage");

    po::variables_map vm;
    auto x = po::parse_command_line(argc, argv, desc);
    po::store(x, vm);
    po::notify(vm);

    //=======================================================================

    if (vm.count("help")) {
        std::cout << desc << std::endl;
        return 1;
    }

    if (vm.count("no-buf")) {
        FLAGS_logbufsecs = 0;
        FLAGS_logbuflevel = -1;
        std::cout.setf(std::ios::unitbuf);
        std::cerr.setf(std::ios::unitbuf);
        setvbuf(stdout, NULL, _IONBF, 0);
        setvbuf(stderr, NULL, _IONBF, 0);
    }
    if (vm.count("verbose")) {
        FLAGS_v = vm["verbose"].as<int>();
        google::SetVLOGLevel("*", FLAGS_v);
    }
    if (vm.count("vmodule")) {
        igen::vec<str> p1, p2;
        str conf = vm["vmodule"].as<str>();
        boost::split(p1, conf, boost::is_any_of(","));
        for (const auto &p : p1) {
            boost::split(p2, p, boost::is_any_of("="));
            google::SetVLOGLevel(p2.at(0).c_str(), boost::lexical_cast<int>(p2.at(1)));
        }
    }
    if (vm.count("log-dir")) {
        str dir = vm["log-dir"].as<str>();
        RAW_VLOG(0, "Log to dir %s", dir.c_str());
        CHECKF(boost::filesystem::exists(dir), "Logdir {} does not exist", dir);
        CHECKF(boost::filesystem::is_directory(dir), "Logdir {} is not a directory", dir);
        FLAGS_log_dir = dir;
    } else {
        for (int i = 0; i < google::NUM_SEVERITIES; ++i) {
            google::SetLogDestination(i, "");
        }
    }

    init_glog(argc, argv);
    // ====
    std::vector<str> all_args{argv, argv + argc};
    igen::map<str, boost::any> opts;
    LOG(WARNING, "Args: {}", fmt::join(all_args, " "));
    for (const auto &kv : vm) opts[kv.first] = kv.second.value();
    opts["_args"] = all_args;

    // ====
    if (vm.count("seed")) {
        uint64_t s = vm["seed"].as<uint64_t>();
        std::seed_seq sseq{uint32_t(s), uint32_t(s >> 32u)};
        igen::Rand.seed(sseq);
        LOG(INFO, "Use random seed: {}", s);
    }

    boost::timer::cpu_timer timer;
    BOOST_SCOPE_EXIT(&timer) {
            LOG(INFO, "{}", timer.format(1));
        }
    BOOST_SCOPE_EXIT_END

    if (vm.count("c50")) {
        switch (vm["c50"].as<int>()) {
            case 0:
                return igen::run_interative_algorithm(opts);
            case 2:
                return igen::run_interative_algorithm_2(opts);
            default:
                CHECK(0) << "Invalid C50 version";
        }
    }

    if (vm.count("analyze")) {
        switch (vm["analyze"].as<int>()) {
            case 0:
                return igen::run_analyzer(opts);
            default:
                CHECK(0) << "Invalid analyzer version";
        }
    }

    return 0;
}

int main(int argc, char *argv[]) {
//    int main_z3_test();
//    return main_z3_test();
    try {
        return prog(argc, argv);
    } catch (z3::exception &ex) {
        LOG(ERROR, "z3 exception: {}", ex);
    } catch (std::exception &ex) {
        LOG(ERROR, "exception: {}", ex.what());
    }
    return 0;
}