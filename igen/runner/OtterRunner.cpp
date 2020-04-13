//
// Created by kh on 4/12/20.
//

#include "OtterRunner.h"

#include <boost/container/flat_map.hpp>
#include <boost/container/flat_set.hpp>
#include <tsl/robin_set.h>

#include <fstream>
#include <iostream>
#include <mutex>

#include <boost/iostreams/filtering_stream.hpp>
#include <boost/iostreams/copy.hpp>
#include <boost/iostreams/filter/gzip.hpp>

#include <boost/algorithm/string.hpp>

namespace igen {


ptr<const OtterRunner> OtterRunner::create(const str &otter_file) {
    static map<str, POtterRunner> _m_cached;
    static std::mutex _mtx;
    std::unique_lock<std::mutex> _lock(_mtx);

    POtterRunner &res = _m_cached[otter_file];
    if (res == nullptr)
        res = new OtterRunner(otter_file);
    return res;
}

OtterRunner::OtterRunner(const str &otter_file) {
    _parse(otter_file);
}

OtterRunner::~OtterRunner() {}


static bool is_all(const str &s, char c) {
    if (s.empty()) return false;
    for (char x : s) if (x != c) return false;
    return true;
}

void OtterRunner::_parse(const str &otter_file) {
    std::ifstream file(otter_file, std::ios_base::in | std::ios_base::binary);
    CHECKF(!file.fail(), "Cannot read file stream: {}", otter_file);
    boost::iostreams::filtering_istream in;
    in.push(boost::iostreams::gzip_decompressor());
    in.push(file);

    int n_vars = 0, n_locs = 0;
    in >> n_vars >> n_locs;
    n_vars_ = n_vars;
    CHECKF(!in.fail(), "Cannot read gzip stream: {}", otter_file);
    CHECK(n_vars > 0 && n_locs > 0);

    {
        str str_locs;
        in >> std::ws, getline(in, str_locs);
        std::stringstream ss(str_locs);
        str_locs.reserve(n_locs);
        str loc;
        while (ss >> loc) loc_names_.emplace_back(move(loc));
        CHECK_EQ(sz(loc_names_), n_locs);

        str str_sep;
        in >> std::ws, getline(in, str_sep);
        CHECK(is_all(str_sep, '#'));
    }

    int n_all_configs = 0, n_dup_configs = 0;
    str line;

    int read_state = 0;
    Bitset bs(n_locs);
    vec<vec<short>> samples;
    tsl::robin_set<hash_t> s_conf_hashes;
    str sexpr;
    while (getline(in, line)) {
        if (is_all(line, '-')) {
            switch (read_state) {
                case 0:
                    CHECKF(bs.any(), "Empty location set ({})", otter_file);
                    break;
                case 1:
                    boost::algorithm::trim(sexpr);
                    CHECKF(!sexpr.empty(), "Empty sexpr ({})", otter_file);
                    break;
                default:
                    CHECK(0);
            }
            read_state++;
            CHECK_LE(read_state, 2);
            continue;
        } else if (is_all(line, '=')) {
            CHECK(!samples.empty());
            CHECK_EQ(read_state, 2);
            entries_.emplace_back(move(samples), std::move(bs));
            read_state = 0, bs.clear(), bs.resize(n_locs), samples.clear(), s_conf_hashes.clear(), sexpr.clear();
            continue;
        }
        switch (read_state) {
            case 0: {
                std::stringstream ss(line);
                int loc, prev_loc = -1;
                while (ss >> loc) {
                    CHECK(0 <= loc && loc < n_locs);
                    CHECK(prev_loc = -1 || prev_loc < loc);
                    bs.set(loc);
                    prev_loc = loc;
                }
                break;
            }
            case 1: {
                sexpr += line;
                break;
            }
            case 2: {
                if (line.empty()) continue;
                for (char &c : line) if (c == ',') c = ' ';
                std::stringstream ss(line);
                vec<short> dat;
                dat.reserve(n_vars);
                short val;
                while (ss >> val) {
                    CHECK(-1 <= val);
                    dat.push_back(val);
                }
                CHECK_EQ(sz(dat), n_vars);
                n_all_configs++;
                if (s_conf_hashes.insert(calc_hash_128(dat)).second) {
                    samples.emplace_back(move(dat));
                } else {
                    n_dup_configs++;
                }
                break;
            }
            default:
                CHECK(0);
        }
    }
    LOG(INFO, "Loaded otter from {} (locs={}, all_confs={}, dup_confs={})",
        otter_file, n_locs, n_all_configs, n_dup_configs);
}

bool OtterRunner::_match(const vec<short> &a, const PConfig &_b) const {
    const vec<short> &b = _b->values();
    for (int i = 0; i < n_vars_; ++i) {
        short x = a[i], y = b[i];
        if (x != -1 && y != -1 && x != y) return false;
    }
    return true;
}

set<str> OtterRunner::run(const PConfig &config) const {
    Bitset bs(n_locs());
    CHECK_EQ(n_vars(), config->ctx()->dom()->n_vars());
    for (const auto &e : entries_) {
        for (const auto &c : e.confs) {
            if (_match(c, config)) {
                bs |= e.covs;
                break;
            }
        }
    }

    set<str> ret;
    std::size_t pos = bs.find_first();
    while (pos != Bitset::npos) {
        ret.emplace_hint(ret.end(), loc_names_[pos]);
        pos = bs.find_next(pos);
    }
    return ret;
}


}