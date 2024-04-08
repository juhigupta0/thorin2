#include "thorin/plug/matrix/pass/lower_matrix_highlevel.h"

#include <iostream>

#include <thorin/lam.h>

#include "thorin/plug/affine/affine.h"
#include "thorin/plug/core/core.h"
#include "thorin/plug/direct/direct.h"
#include "thorin/plug/matrix/matrix.h"
#include "thorin/plug/mem/mem.h"

namespace thorin::plug::matrix {

namespace {

std::optional<Ref> internal_function_of_axiom(const Axiom* axiom, Ref meta_args, Ref args) {
    auto& world = axiom->world();
    auto name   = axiom->sym().str();
    find_and_replace(name, ".", "_");
    find_and_replace(name, "%", "");
    name = INTERNAL_PREFIX + name;

    auto replacement = world.external(world.sym(name));
    if (replacement) {
        auto spec_fun = world.app(replacement, meta_args);
        auto ds_fun   = direct::op_cps2ds_dep(spec_fun);
        return world.app(ds_fun, args);
    }
    return std::nullopt;
}

} // namespace

Ref LowerMatrixHighLevelMapRed::rewrite(Ref def) {
    if (auto i = rewritten.find(def); i != rewritten.end()) return i->second;
    auto new_def   = rewrite_(def);
    rewritten[def] = new_def;
    return rewritten[def];
}

Ref LowerMatrixHighLevelMapRed::rewrite_(Ref def) {
    if (auto mat_ax = match<matrix::prod>(def)) {
        auto [mem, M, N]  = mat_ax->args<3>();
        auto [m, k, l, w] = mat_ax->decurry()->args<4>();
        auto w_lit        = Lit::isa(w);

        auto ext_fun = world().external(world().sym("extern_matrix_prod"));
        if (ext_fun && (w_lit && *w_lit == 64)) {
            auto ds_fun  = direct::op_cps2ds_dep(ext_fun);
            auto fun_app = world().app(ds_fun, {mem, m, k, l, M, N});
            return fun_app;
        }
    } else if (auto mat_ax = match<matrix::multiplication_gpu>(def)) {
        auto [mem, M, N, O]  = mat_ax->args<4>();
        auto [m, k, l, w] = mat_ax->decurry()->args<4>();
        auto w_lit        = Lit::isa(w);

        auto ext_fun = world().external(world().sym("extern_matrix_multiplication_gpu"));
        if (ext_fun && (w_lit && *w_lit == 64)) {
            auto ds_fun  = direct::op_cps2ds_dep(ext_fun);
            auto fun_app = world().app(ds_fun, {mem, m, k, l, M, N, O});
            return fun_app;
        }
    } else if (auto mat_ax = match<matrix::addition>(def)) {
        auto [mem, M, N]  = mat_ax->args<3>();
        auto [m, k, w] = mat_ax->decurry()->args<3>();
        auto w_lit        = Lit::isa(w);

        auto ext_fun = world().external(world().sym("extern_matrix_addition"));
        if (ext_fun && (w_lit && *w_lit == 64)) {
            auto ds_fun  = direct::op_cps2ds_dep(ext_fun);
            auto fun_app = world().app(ds_fun, {mem, m, k, M, N});
            return fun_app;
        }
    } else if (auto mat_ax = match<matrix::addition_gpu>(def)) {
        auto [mem, M, N, O]  = mat_ax->args<4>();
        auto [m, k, w] = mat_ax->decurry()->args<3>();
        auto w_lit        = Lit::isa(w);

        auto ext_fun = world().external(world().sym("extern_matrix_addition_gpu"));
        if (ext_fun && (w_lit && *w_lit == 64)) {
            auto ds_fun  = direct::op_cps2ds_dep(ext_fun);
            auto fun_app = world().app(ds_fun, {mem, m, k, M, N, O});
            return fun_app;
        }
    } else if (auto mat_ax = match<matrix::add_multiply_gpu>(def)) {
        auto [mem, A, B, C, O]  = mat_ax->args<5>();
        auto [m, w] = mat_ax->decurry()->args<2>();
        auto w_lit        = Lit::isa(w);

        auto ext_fun = world().external(world().sym("extern_matrix_add_multiply_gpu"));
        if (ext_fun && (w_lit && *w_lit == 64)) {
            auto ds_fun  = direct::op_cps2ds_dep(ext_fun);
            auto fun_app = world().app(ds_fun, {mem, m, A, B, C, O});
            return fun_app;
        }
    }
    

    if (auto outer_app = def->isa<App>()) {
        if (auto inner_app = outer_app->callee()->isa<App>()) {
            if (auto axiom = inner_app->callee()->isa<Axiom>()) {
                if (auto internal_function = internal_function_of_axiom(axiom, inner_app->arg(), outer_app->arg())) {
                    world().DLOG("lower matrix axiom {} in {} : {}", *axiom->sym(), def, def->type());
                    world().DLOG("lower matrix axiom using: {} : {}", *internal_function, (*internal_function)->type());
                    return *internal_function;
                }
            }
        }
    }

    return def;
}

} // namespace thorin::plug::matrix
