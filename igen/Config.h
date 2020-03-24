//
// Created by KH on 3/5/2020.
//

#ifndef IGEN4_CONFIG_H
#define IGEN4_CONFIG_H

#include "Context.h"
#include "Domain.h"
#include "Location.h"
#include <klib/hash.h>

#include <boost/iterator/iterator_adaptor.hpp>

namespace igen {

class Location;

class Config : public Object {
public:
    explicit Config(PMutContext ctx, int id = -1);

    explicit Config(PMutContext ctx, const vec<short> &values, int id = -1);

    explicit Config(PMutContext ctx, const str &str_values, int id = -1);

    explicit Config(const ptr<const Config> &c);

    explicit Config(const ptr<Config> &c);

    int id() const { return id_; }

    void set_id(int id) {
        CHECK_EQ(id_, -1) << "Config id is already set";
        id_ = id;
    }

    const vec<short> &values() const { return values_; }

    vec<short> &values() { return values_; }

    vec<str> value_labels() const;

    short operator[](int var_id) const { return values_.at((size_t) var_id); }

    short get(int var_id) const { return values_.at((size_t) var_id); }

    short value(int var_id) const { return values_.at((size_t) var_id); }

    void set(int var_id, int value) {
        CHECK(-1 <= value && value < dom()->n_values(var_id));
        values_.at((size_t) var_id) = (short) value;
    }

    void set_all(int value);

    void set_all(const vec<short> &values);

    vec<int> &cov_loc_mut_ids() { return cov_locs_; }

    const vec<int> &cov_loc_ids() const { return cov_locs_; }

    void add_cov_loc_id(int loc_id);

    bool eval(z3::expr e) const;

    expr to_expr() const;

    bool cov_loc(const str &loc_name) const;

    hash_t hash(bool bypass_cache = false) const;

private:
    int id_;
    vec<short> values_;
    vec<int> cov_locs_; // Location id

    mutable hash_t cached_hash_ = hash128_empty;

public:
    class Entry {
    public:
        [[nodiscard]] int var_id() const { return var_id_; }

        [[nodiscard]] int value() const { return value_; }

        [[nodiscard]] const str &name() const { return dom_.name(var_id_); }

        [[nodiscard]] const str &label() const { return dom_.label(var_id_, value_); }

    private:
        friend class Config;

        Entry(const Domain &dom, const int var_id, const int value) : dom_(dom), var_id_(var_id), value_(value) {
            DCHECK(0 <= var_id && var_id < dom.n_vars() && -1 <= value && value < dom.n_values(var_id));
        }

        const Domain &dom_;
        const int var_id_;
        const int value_;
    };

    class const_iterator :
            public boost::iterator_adaptor<const_iterator, vec<short>::const_iterator, // Base type
                    const Entry,         // value_type
                    boost::use_default,     // difference_type
                    const Entry>       // reference_type
    {
    public:
        const_iterator() = delete;

        const_iterator(const const_iterator &) = default;

    private:
        friend class Config;                 // allow private constructor
        friend class boost::iterator_core_access; // allow dereference()
        const_iterator(base_type iter, base_type iter_begin, PDomain dom) :
                iterator_adaptor(iter), iter_begin(iter_begin), dom(move(dom)) {}

        [[nodiscard]] Entry dereference() const {
            return Entry(*dom, int(base() - iter_begin), *base_reference());
        }

        base_type iter_begin;
        PDomain dom;
    };

    class const_location_iterator :
            public boost::iterator_adaptor<const_location_iterator, vec<int>::const_iterator, // Base type
                    const ptr<const Location>,         // value_type
                    boost::use_default,     // difference_type
                    const ptr<const Location>>       // reference_type
    {
    public:
        const_location_iterator() = delete;

        const_location_iterator(const const_location_iterator &) = default;

    private:
        friend class Config;                 // allow private constructor
        friend class boost::iterator_core_access; // allow dereference()
        const_location_iterator(base_type iter, PContext ctx) : iterator_adaptor(iter), ctx(move(ctx)) {}

        [[nodiscard]] const ptr<const Location> dereference() const;

        base_type iter_begin;
        PContext ctx;
    };

    const_iterator begin() const { return const_iterator(values_.begin(), values_.begin(), dom()); }

    const_iterator end() const { return const_iterator(values_.end(), values_.begin(), dom()); }

public:
    klib::custom_iterable_container<const_location_iterator> cov_locs() const {
        return {const_location_iterator(cov_locs_.begin(), ctx()), const_location_iterator(cov_locs_.end(), ctx())};
    }
};

using PConfig = ptr<const Config>;
using PMutConfig = ptr<Config>;

std::ostream &operator<<(std::ostream &output, const Config &d);

}

#endif //IGEN4_CONFIG_H
