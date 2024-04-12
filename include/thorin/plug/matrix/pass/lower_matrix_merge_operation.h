#pragma once

#include <thorin/def.h>

#include <thorin/pass/pass.h>

namespace thorin::plug::matrix {

class LowerMatrixMergeOperation : public RWPass<LowerMatrixMergeOperation, Lam> {
public:
    LowerMatrixMergeOperation(PassMan& man)
        : RWPass(man, "lower_matrix_merge_operation") {}

    /// custom rewrite function
    Ref rewrite(Ref) override;
    Ref rewrite_(Ref);

private:
    Def2Def rewritten;
    Def2Def addition_defs;
};

} // namespace thorin::plug::matrix
