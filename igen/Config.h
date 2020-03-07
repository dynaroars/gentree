//
// Created by KH on 3/5/2020.
//

#ifndef IGEN4_CONFIG_H
#define IGEN4_CONFIG_H

#include "Context.h"
#include "Domain.h"

#include <boost/iterator/iterator_adaptor.hpp>

namespace igen {

class Config : public Object {
public:
    explicit Config(PMutContext ctx, int id = -1);

    int id() const { return id_; }

    const vec<int> &values() const { return values_; }

    vec<int> &values() { return values_; }

private:
    int id_;
    vec<int> values_;

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
            DCHECK(0 <= var_id && var_id < dom.n_vars() && 0 <= value && value < dom.n_values(var_id));
        }

        const Domain &dom_;
        const int var_id_;
        const int value_;
    };

    class const_iterator :
            public boost::iterator_adaptor<const_iterator, vec<int>::const_iterator, // Base type
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

    const_iterator begin() const { return const_iterator(values_.begin(), values_.begin(), dom()); }

    const_iterator end() const { return const_iterator(values_.end(), values_.begin(), dom()); }
};

using PConfig = ptr<const Config>;
using PMutConfig = ptr<Config>;

std::ostream &operator<<(std::ostream &output, const Config &d);

}

#endif //IGEN4_CONFIG_H
