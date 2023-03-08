#pragma once

#include <vector>

#include <thorin/def.h>
#include <thorin/pass/pass.h>

namespace thorin::direct {

/// This is the second part of ds2cps.
/// We replace all ds call sites of cps (or ds converted) functions with the cps calls.
/// `b = f args` becomes `f (args,cont)` with a newly introduced continuation `cont : cn b`.
class CPS2DS : public RWPass<CPS2DS, Lam> {
public:
    CPS2DS(PassMan& man)
        : RWPass(man, "cps2ds")
        , sym_{.cps_call = world().sym("csp_call")} {}

    void enter() override;

private:
    // Def2Def rewritten_lams;
    struct {
        Sym cps_call;
    } sym_;
    Def2Def rewritten_;
    DefSet rewritten_lams;
    std::vector<Lam*> lam_stack;
    Lam* curr_lam_ = nullptr;

    void rewrite_lam(Lam* lam);
    const Def* rewrite_body(const Def*);
    const Def* rewrite_body_(const Def*);
};

} // namespace thorin::direct
