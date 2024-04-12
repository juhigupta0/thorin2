#include "thorin/plug/matrix/pass/lower_matrix_merge_operation.h"

#include <iostream>

#include <thorin/lam.h>

#include "thorin/plug/affine/affine.h"
#include "thorin/plug/core/core.h"
#include "thorin/plug/direct/direct.h"
#include "thorin/plug/matrix/matrix.h"
#include "thorin/plug/mem/mem.h"

namespace thorin::plug::matrix {

Ref LowerMatrixMergeOperation::rewrite(Ref def) {

    if (auto mat_ax = match<matrix::addition_gpu>(def)) {
       
        // Insert the current def in a container
        addition_defs[def] = def;
    
    } else if (auto mat_ax = match<matrix::multiplication_gpu>(def)) {
        
        // Extract the arguements of multiplication_gpu operation
        auto [mem, M, N, O]  = mat_ax->args<4>();
        
        // Check if during the pass, do we have any previous addition_gpu operation
        // If yes, then compare the output of the addition_gpu operation 
        
        if (!addition_defs.empty()){

            for (const auto& pair : addition_defs) {

                auto addition_def = pair.first;
                // Confirm if the def in the accumulated addition_defs container is an addition_gpu operation

                if (auto mat_ax_add = match<matrix::addition_gpu>(addition_def)){
                    // Extract the arguements of addition_gpu operation
                    auto [mem, A, B, R]  = mat_ax_add->args<4>();
                    auto [m, k, w] = mat_ax_add->decurry()->args<3>();
                    auto& world = addition_def->world();

                    // Check if output of the addition_gpu operation is the one of the inputs of the multiplication_gpu operation
                    // If any of the inputs matches, fuse both the operations and return the fused operation
                    if(R == M){
                        auto new_app = world.app(world.app(world.annex<add_multiply_gpu>(), {m, w}), {mem, A, B, N, O});
                        return new_app;
                    }
                        
                    else if (R == N){
                        auto new_app = world.app(world.app(world.annex<add_multiply_gpu>(), {m, w}), {mem, A, B, M, O});
                        return new_app;
                    }

                    return def;
                }
            }
        }
    }
    return def;
}

} // namespace thorin::plug::matrix
