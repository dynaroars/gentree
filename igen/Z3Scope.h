//
// Created by KH on 3/5/2020.
//

#ifndef IGEN4_Z3SCOPE_H
#define IGEN4_Z3SCOPE_H

#include <z3++.h>

namespace igen {


class Z3Scope {
public:
    explicit Z3Scope(z3::solver *s) : solver(s) { solver->push(); }

    explicit Z3Scope(z3::solver &s) : solver(&s) { solver->push(); }

    Z3Scope(Z3Scope &&z) noexcept {
        solver = z.solver;
        z.solver = nullptr;
    }

    Z3Scope(const Z3Scope &o) = delete;

    ~Z3Scope() { if (solver) solver->pop(); }

    z3::solver *operator->() { return solver; }

    const z3::solver *operator->() const { return solver; }

    z3::solver &operator*() { return *solver; }

    const z3::solver &operator*() const { return *solver; }

private:
    z3::solver *solver = nullptr;
};


}

#endif //IGEN4_Z3SCOPE_H
