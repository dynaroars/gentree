//
// Created by KH on 3/5/2020.
//

#ifndef IGEN4_DOMAIN_H
#define IGEN4_DOMAIN_H

#include "Context.h"
#include "Config.h"
#include <z3++.h>
#include <boost/iterator/iterator_adaptor.hpp>

namespace igen {

class VarEntry : public Object {
private:
    friend class Domain;

    int id_;
    str name_;
    vec<std::string> labels_;

private:
    expr zvar;
    vec<expr> zvar_eq_val;

public:
    explicit VarEntry(PMutContext ctx);

    int n_vars() const { return (int) labels_.size(); }

    int id() const { return id_; }

    const str &name() const { return name_; };

    const vec<str> &labels() const { return labels_; };

    expr eq(int val) const;
};

using PVarEntry = ptr<const VarEntry>;
using PMutVarEntry = ptr<VarEntry>;

//===================================================================================

class Domain : public Object {
public:
    explicit Domain(PMutContext ctx);

    std::istream &parse(std::istream &input);

    int n_all_values() const { return n_all_values_; }

    int n_vars() const { return int(vars.size()); }

private:
    vec<PMutVarEntry> vars;
    int n_all_values_ = 0;

    friend std::ostream &operator<<(std::ostream &output, const Domain &d);

public:
    class iterator :
            public boost::iterator_adaptor<
                    iterator,                         // This class, for CRTP
                    vec<PMutVarEntry>::const_iterator, // Base type
                    PMutVarEntry>  // value_type
    {
    public:
        iterator() = delete;

        iterator(const iterator &) = default;

    private:
        friend class Domain;                      // allow private constructor
        friend class boost::iterator_core_access; // allow dereference()
        explicit iterator(base_type iter) : iterator_adaptor(iter) {}

        [[nodiscard]] const PMutVarEntry &dereference() const { return *base_reference(); }
    };

    class const_iterator :
            public boost::iterator_adaptor<
                    const_iterator,                        // This class, for CRTP
                    vec<PMutVarEntry>::const_iterator,     // Base type
                    PVarEntry,                             // value_type
                    boost::use_default,                    // difference_type
                    PVarEntry>       // reference_type
    {
    public:
        const_iterator() = delete;

        const_iterator(const const_iterator &) = default;

        // Implicit conversion from iterator to const_iterator:
        const_iterator(const iterator &iter) : iterator_adaptor(iter.base()) {}

    private:
        friend class Domain;                 // allow private constructor
        friend class boost::iterator_core_access; // allow dereference()
        explicit const_iterator(base_type iter) : iterator_adaptor(iter) {}

        PVarEntry dereference() const { return *base_reference(); }
    };

    iterator begin() { return iterator(vars.begin()); }

    iterator end() { return iterator(vars.end()); }

    const_iterator begin() const { return cbegin(); }

    const_iterator end() const { return cend(); }

    const_iterator cbegin() const { return const_iterator(vars.begin()); }

    const_iterator cend() const { return const_iterator(vars.end()); }
};

std::istream &operator>>(std::istream &input, Domain &d);

std::ostream &operator<<(std::ostream &output, const Domain &d);

using PDomain = ptr<const Domain>;
using PMutDomain = ptr<Domain>;

}

#endif //IGEN4_DOMAIN_H
