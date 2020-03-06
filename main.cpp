#include <iostream>

#include <klib/common.h>
#include <z3++.h>

#include <boost/program_options/options_description.hpp>
#include <boost/program_options/variables_map.hpp>
#include <boost/program_options/parsers.hpp>

#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/timer/timer.hpp>
#include <boost/scope_exit.hpp>

#include <glog/raw_logging.h>

namespace po = boost::program_options;

void init_glog(int argc, char **argv) {
    (void) argc;
    FLAGS_colorlogtostderr = true;
    FLAGS_timestamp_in_logfile_name = false;
    FLAGS_max_log_size = 32;
    google::SetStderrLogging(0);
    //google::SetVLOGLevel("*", 20);
    google::InitGoogleLogging(argv[0]);
}

int prog(int argc, char *argv[]) {
    using str = std::string;
    po::options_description desc("Dynamic Interaction Inference for Configurable Software");
    desc.add_options()
            ("seed,s", po::value<uint64_t>(), "Random seed")
            ("runner,r", po::value<str>(), "Executor: s/simple")
            ("filestem,F", po::value<str>(), "Filestem")
            
            ("conjdisj,A", "Run the conj/disj algorithm")
            ("c50,C", "Run the ML algorithm")

            ("vmodule,V", po::value<str>(), "Verbose logging. eg -V mapreduce=2,file=1,gfs*=3")
            ("verbose,v", po::value<int>(), "Verbose level")
            ("no-buf,N", "Do not buffer log")
            ("log-file,L", po::value<str>(), "Log to file")
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
    if (vm.count("log-file")) {
        str logFile = vm["log-file"].as<str>();
        RAW_VLOG(0, "Log to file %s", logFile.c_str());
        for (int i = 0; i < google::NUM_SEVERITIES; ++i) {
            google::SetLogDestination(i, logFile.c_str());
        }
    }

    init_glog(argc, argv);

    boost::timer::cpu_timer timer;
    BOOST_SCOPE_EXIT(&timer) {
            LOG(INFO) << timer.format(1);
        }
    BOOST_SCOPE_EXIT_END

    if (vm.count("iterative")) {
        return 0;
    }

    if (vm.count("C50")) {
        return 0;
    }

    return 0;
}

int main(int argc, char *argv[]) {
//    int main_z3_test();
//    return main_z3_test();
    try {
        return prog(argc, argv);
    } catch (z3::exception &ex) {
        LOG(ERROR) << "z3 exception: " << ex;
    } catch (std::exception &ex) {
        LOG(ERROR) << ex.what();
    }
    return 0;
}