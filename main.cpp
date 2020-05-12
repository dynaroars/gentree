#include <iostream>

#include <klib/common.h>
#include <klib/random.h>
#include <klib/WorkQueue.h>
#include <klib/vecutils.h>
#include <klib/anyutils.h>
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
#include <igen/Context.h>
#include <igen/runner/ProgramRunnerMt.h>

namespace po = boost::program_options;
using namespace igen;

void init_glog(int argc, char **argv) {
    (void) argc;
    FLAGS_colorlogtostderr = true;
    //FLAGS_timestamp_in_logfile_name = false;
    //FLAGS_max_log_size = 32;
    FLAGS_logbuflevel = google::GLOG_INFO;
    google::SetStderrLogging(0);
    //google::SetVLOGLevel("*", 20);
    google::InitGoogleLogging(argv[0]);
    google::InstallFailureSignalHandler();
}

str analyze_res_to_csv(vec<map<str, boost::any>> v_results) {
    constexpr double kNAN = kMIN<double>;
    std::stringstream ss;
    vec<str> cols = get_keys_as_vec(v_results.at(0));
    vec<vec<double>> col_vals(cols.size());
    fmt::print(ss, "{}\n", fmt::join(cols, ","));
    for (const auto &mp : v_results) {
        CHECK(get_keys_as_vec(mp) == cols);
        int col_id = 0;
        for (const auto &kv : mp) {
            ss << any2string(kv.second) << ',';
            col_vals[col_id].push_back(any2double(kv.second));
            col_id++;
        }
        ss << '\n';
    }
    if (!v_results.empty()) {
        int col_id = 0;
        for (auto &v : col_vals) {
            sort(v.begin(), v.end());
            if (v.at(0) == kNAN) {
                ss << "nan,";
                continue;
            } else if (cols.at(col_id++) == "_repeat_id") {
                ss << "MED,";
                continue;
            }
            ss << vec_median(v) << ",";
        }
    }
    return ss.str();
}

int prog(int argc, char *argv[]) {
    po::options_description desc("Dynamic Interaction Inference for Configurable Software");
    desc.add_options()
            ("filestem,F", po::value<str>(), "Filestem")
            ("target", po::value<str>(), "Target")
            ("dom", po::value<str>(), "Dom file")
            ("runner,r", po::value<str>(), "Select program runner")
            ("simple-runner,S", "Simple runner")
            ("builtin-runner,B", "Builtin runner")
            ("gcov-runner,G", "GCov runner")
            ("otter-runner,Y", "Otter runner")
            ("seed,s", po::value<uint64_t>()->default_value(123), "Random seed")
            ("output,O", po::value<str>(), "Output result")
            ("params-output,P", po::value<str>(), "Parameter Output result")
            ("disj-conj", "Gen expr strat DisjOfConj")
            ("cache,c", po::value<str>()->default_value(""), "Cache control: read/write/execute")
            ("cache-path,p", po::value<str>()->default_value(""), "Custom cachedb path")
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

            ("locs,X", po::value<str>()->default_value(""), "Interested location")
            ("rounds,R", po::value<int>()->default_value(int(1e9)), "Number of iterations")
            ("seed-configs,C", po::value<std::vector<str>>()->default_value(std::vector<str>(), "none"), "Seed configs")
            ("rep", po::value<int>()->default_value(1), "Repeat the experiment")
            ("rep-parallel", po::value<int>()->default_value(1), "Repeat multithreaded")
            ("fixed-seed", "Keep seed fixed")

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
        setvbuf(stdout, nullptr, _IONBF, 0);
        setvbuf(stderr, nullptr, _IONBF, 0);
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

    std::vector<str> all_args{argv, argv + argc};
    LOG(WARNING, "Args: {}", fmt::join(all_args, " "));

    boost::timer::cpu_timer timer;
    BOOST_SCOPE_EXIT(&timer) {
            LOG(INFO, "{}", timer.format(1));
        }
    BOOST_SCOPE_EXIT_END

    // == BEGIN REAL PROGRAM ==

    const int n_repeats = vm["rep"].as<int>();
    const int n_threads = vm["rep-parallel"].as<int>();
    const bool fixed_seed = vm.count("fixed-seed");

    WorkQueue work_queue;
    if (n_threads > 1) work_queue.init(n_threads);
    vec<map<str, boost::any>> v_results(n_repeats);
    const auto get_opts = [&vm = std::as_const(vm), &all_args = std::as_const(all_args)]
            (int thread_id, int repeat_id) {
        igen::map<str, boost::any> opts;
        for (const auto &kv : vm) opts[kv.first] = kv.second.value();
        opts["_args"] = all_args;
        opts["_thread_id"] = thread_id;
        opts["_repeat_id"] = repeat_id;

        // ====
        str str_item_id = std::to_string(repeat_id);
        for (auto &kv : opts) {
            if (kv.second.type() == typeid(str)) {
                str s = boost::any_cast<str>(kv.second);
                boost::algorithm::replace_all(s, "{i}", str_item_id);
                kv.second = s;
            }
        }
        return opts;
    };

    str cache_ctl = vm["cache"].as<str>();
    PMutContext shared_ctx;
    if (!cache_ctl.empty() && n_repeats > 1 && n_threads > 1) {
        shared_ctx = new Context();
        shared_ctx->set_options(get_opts(-1, -1));
        shared_ctx->init();
        shared_ctx->init_runner();
        LOG(WARNING, "Inited shared program runner");
    }
    BOOST_SCOPE_EXIT(&shared_ctx) {
            if (shared_ctx != nullptr) shared_ctx->cleanup();
        }
    BOOST_SCOPE_EXIT_END
    const auto fn_each = [&vm = std::as_const(vm), &all_args = std::as_const(all_args),
            fixed_seed, &v_results, &get_opts, &shared_ctx = std::as_const(shared_ctx)]
            (int thread_id, int repeat_id) {
        LOG(WARNING, "@@@ Running repeat, repeat_id = {}, thread_id = {}", repeat_id, thread_id);
        auto opts = get_opts(thread_id, repeat_id);

        {
            uint64_t seed64 = vm["seed"].as<uint64_t>();
            if (!fixed_seed) seed64 += uint64_t(repeat_id);
            std::seed_seq sseq{uint32_t(seed64), uint32_t(seed64 >> 32u)};
            igen::Rand.seed(sseq);
            LOG(INFO, "Use random seed: {} (repeat_id = {}, thread_id = {})", seed64, repeat_id, thread_id);
            opts["seed"] = seed64;
        }

        if (shared_ctx != nullptr) {
            opts["_shared_program_runner"] = shared_ctx->runner();
        }

        auto &mp_res = v_results.at(repeat_id);
        // ====
        if (vm.count("c50")) {
            switch (vm["c50"].as<int>()) {
                case 2:
                    mp_res = igen::run_interative_algorithm_2(opts);
                    break;
                case 3:
                    mp_res = igen::run_demo_algo(opts);
                    break;
                default:
                    CHECK(0) << "Invalid C50 version";
            }
        }

        if (vm.count("analyze")) {
            switch (vm["analyze"].as<int>()) {
                case 0:
                    mp_res = igen::run_analyzer(opts);
                    break;
                default:
                    CHECK(0) << "Invalid analyzer version";
            }
        }

        mp_res["_thread_id"] = thread_id;
        mp_res["_repeat_id"] = repeat_id;
    };
    if (n_threads > 1) {
        work_queue.start();
        work_queue.run_batch_job(fn_each, n_repeats);
    } else {
        for (int i = 0; i < n_repeats; ++i) fn_each(0, i);
    }

    str csv_params = analyze_res_to_csv(v_results);
    if (vm.count("params-output")) {
        str pout = vm["params-output"].as<str>();
        std::ofstream file(pout);
        file << csv_params;
        LOG(INFO, "Wrote params output to {}", pout);
    } else {
        LOG(INFO, "Params output:\n") << csv_params;
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