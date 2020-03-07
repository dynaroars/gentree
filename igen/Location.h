//
// Created by KH on 3/7/2020.
//

#ifndef IGEN4_LOCATION_H
#define IGEN4_LOCATION_H

#include "Context.h"

#include <boost/iterator/iterator_adaptor.hpp>
#include <klib/custom_container.h>

namespace igen {

class Config;

class Location : public Object {
public:
    explicit Location(PMutContext ctx, str name, int id = -1);

    int id() const { return id_; }

    void set_id(int id) {
        CHECK_EQ(id_, -1) << "Location id is already set";
        id_ = id;
    }

    const str &name() const { return name_; }

    vec<int> &cov_by_mut_ids() { return cov_by_; }

private:
    str name_;
    int id_;

    vec<int> cov_by_; // Config Id

public:
    class const_config_iterator :
            public boost::iterator_adaptor<const_config_iterator, vec<int>::const_iterator, // Base type
                    const ptr<const Config>,         // value_type
                    boost::use_default,     // difference_type
                    const ptr<const Config>>       // reference_type
    {
    public:
        const_config_iterator() = delete;

        const_config_iterator(const const_config_iterator &) = default;

    private:
        friend class Location;                 // allow private constructor
        friend class boost::iterator_core_access; // allow dereference()
        const_config_iterator(base_type iter, PContext ctx) : iterator_adaptor(iter), ctx(move(ctx)) {}

        [[nodiscard]] const ptr<const Config> dereference() const;

        base_type iter_begin;
        PContext ctx;
    };

public:
    klib::custom_iterable_container<const_config_iterator> cov_by() const {
        return {const_config_iterator(cov_by_.begin(), ctx()), const_config_iterator(cov_by_.end(), ctx())};
    }
};

using PLocation = ptr<const Location>;
using PMutLocation = ptr<Location>;

}

#endif //IGEN4_LOCATION_H
