//
// Created by KH on 3/5/2020.
//

#ifndef IGEN4_CONTEXT_H
#define IGEN4_CONTEXT_H

#include <klib/common.h>
#include <boost/container/flat_map.hpp>
#include <boost/any.hpp>
#include <z3++.h>

#include "Z3Scope.h"

namespace igen {

class Domain;

void intrusive_ptr_release(const Domain *);

class ProgramRunnerMt;

void intrusive_ptr_add_ref(const ProgramRunnerMt *);

void intrusive_ptr_release(const ProgramRunnerMt *);

class CoverageStore;

void intrusive_ptr_add_ref(const CoverageStore *);

void intrusive_ptr_release(const CoverageStore *);

class Context : public intrusive_ref_base_mt<Context> {
public:
    Context();

    virtual ~Context();

    void set_option(const str &key, boost::any val);

    void set_options(map<str, boost::any> opts);

    bool has_option(const str &key) const;

    boost::any get_option(const str &key) const;

    template<class T>
    T get_option_as(const str &key) const { return boost::any_cast<T>(get_option(key)); }

    void init();

    void cleanup();

    const ptr<const Domain> &dom() const { return c_dom_; };

    //const ptr<Domain> &dom() { return dom_; };

    // ptr<const ProgramRunnerMt> runner() const { return program_runner_; };

    const ptr<ProgramRunnerMt> &runner() { return program_runner_; };

    const ptr<const CoverageStore> &cov() const { return c_coverage_store_; };

    const ptr<CoverageStore> &cov_mut() { return coverage_store_; };

public:
    z3::context &zctx() { return z3ctx_; }

    z3::solver &zsolver() { return z3solver_; }

    Z3Scope zscope() { return Z3Scope(&z3solver_); }

    const z3::expr &ztrue() const { return z3true_; }

    const z3::expr &zfalse() const { return z3false_; }

    const z3::expr &zbool(bool b) const { return b ? z3true_ : z3false_; }

    expr zctx_solver_simplify(const expr &e) const;

private:
    friend class Object;

    map<str, boost::any> options;

    z3::context z3ctx_;
    z3::solver z3solver_;
    z3::expr z3true_;
    z3::expr z3false_;

    ptr<Domain> dom_;
    ptr<const Domain> c_dom_;
    ptr<ProgramRunnerMt> program_runner_;
    ptr<CoverageStore> coverage_store_;
    ptr<const CoverageStore> c_coverage_store_;
};

using PContext = ptr<const Context>;
using PMutContext = ptr<Context>;

class Object : public intrusive_ref_base_mt<Object> {
public:
    PContext ctx() const { return ctx_; }

    const PMutContext &ctx() { return ctx_; }

    const PMutContext &ctx_mut() const { return ctx_; }

    z3::context &zctx() const { return ctx_mut()->z3ctx_; }

    z3::solver &zsolver() { return ctx()->z3solver_; }

    Z3Scope zscope() { return Z3Scope(&ctx()->z3solver_); }

    const z3::expr &ztrue() const { return ctx()->z3true_; }

    const z3::expr &zfalse() const { return ctx()->z3false_; }

    const ptr<const Domain> &dom() const { return ctx_->c_dom_; };

    const ptr<const CoverageStore> &cov() const { return ctx_->cov(); };

    const ptr<CoverageStore> &cov_mut() { return ctx_->cov_mut(); };

    virtual ~Object() = default;

    virtual void cleanup() {};

protected:
    explicit Object(PMutContext ctx) : ctx_(move(ctx)) {};

private:
    PMutContext ctx_;
};

using PObject = ptr<const Object>;
using PMutObject = ptr<Object>;

}

#endif //IGEN4_CONTEXT_H
