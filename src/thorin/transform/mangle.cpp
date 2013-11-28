#include "thorin/literal.h"
#include "thorin/primop.h"
#include "thorin/type.h"
#include "thorin/world.h"
#include "thorin/analyses/scope.h"

namespace thorin {

class Mangler {
public:
    Mangler(const Scope& scope,
            Def2Def& mapped,
            ArrayRef<size_t> to_drop,
            ArrayRef<Def> drop_with,
            ArrayRef<Def> to_lift,
            const GenericMap& generic_map)
        : scope(scope)
        , to_drop(to_drop)
        , drop_with(drop_with)
        , to_lift(to_lift)
        , generic_map(generic_map)
        , world(scope.world())
        , pass1(scope.mark())
        , pass2(mapped)
    {
        std::queue<Def> queue;
        for (auto def : to_lift)
            queue.push(def);
        mark_down(pass1, queue);
    }

    Lambda* mangle();
    void mangle_body(Lambda* olambda, Lambda* nlambda);
    Lambda* mangle_head(Lambda* olambda);
    Def mangle(Def odef);
    Def lookup(Def def) {
        assert(pass2.contains(def));
        return pass2[def];
    }

    const Scope& scope;
    ArrayRef<size_t> to_drop;
    ArrayRef<Def> drop_with;
    ArrayRef<Def> to_lift;
    GenericMap generic_map;
    World& world;
    DefSet pass1;
    Def2Def& pass2;
    Lambda* nentry;
    Lambda* oentry;
};

Lambda* Mangler::mangle() {
    assert(scope.num_entries() == 1 && "TODO");
    oentry = scope.entries()[0];
    const Pi* o_pi = oentry->pi();
    auto nelems = o_pi->elems().cut(to_drop, to_lift.size());
    size_t offset = o_pi->elems().size() - to_drop.size();

    Call call(oentry);
    for (auto def : drop_with)
        call.args.push_back(def);

    for (auto x : to_drop)
        call.idx.push_back(x);

#if 0
    auto iter = world.cache_.find(call);
    if (iter != world.cache_.end()) {
        assert(!iter->second->empty());
        return iter->second;
    }
#endif

    for (size_t i = offset, e = nelems.size(), x = 0; i != e; ++i, ++x)
        nelems[i] = to_lift[x]->type();

    const Pi* n_pi = world.pi(nelems)->specialize(generic_map)->as<Pi>();
    nentry = world.lambda(n_pi, oentry->name);

    // put in params for entry (oentry)
    // op -> iterates over old params
    // np -> iterates over new params
    //  i -> iterates over to_drop
    for (size_t op = 0, np = 0, i = 0, e = o_pi->size(); op != e; ++op) {
        const Param* oparam = oentry->param(op);
        if (i < to_drop.size() && to_drop[i] == op)
            pass2[oparam] = drop_with[i++];
        else {
            const Param* nparam = nentry->param(np++);
            nparam->name = oparam->name;
            pass2[oparam] = nparam;
        }
    }

    for (size_t i = offset, e = nelems.size(), x = 0; i != e; ++i, ++x) {
        pass2[to_lift[x]] = nentry->param(i);
        nentry->param(i)->name = to_lift[x]->name;
    }

    pass2[oentry] = oentry;
    mangle_body(oentry, nentry);

    for (auto cur : scope.rpo().slice_from_begin(1)) {
        if (pass2.contains(cur))
            mangle_body(cur, lookup(cur)->as_lambda());
        else
            pass2[cur] = cur;
    }

    //world.cache_[call] = nentry;
    return nentry;
}

Lambda* Mangler::mangle_head(Lambda* olambda) {
    assert(!pass2.contains(olambda));
    assert(!olambda->empty());
    Lambda* nlambda = olambda->stub(generic_map, olambda->name);
    pass2[olambda] = nlambda;

    for (size_t i = 0, e = olambda->num_params(); i != e; ++i)
        pass2[olambda->param(i)] = nlambda->param(i);

    return nlambda;
}

void Mangler::mangle_body(Lambda* olambda, Lambda* nlambda) {
    assert(!olambda->empty());
    Array<Def> ops(olambda->ops().size());
    for (size_t i = 1, e = ops.size(); i != e; ++i)
        ops[i] = mangle(olambda->op(i));

    // fold branch if possible
    if (auto select = olambda->to()->isa<Select>()) {
        Def cond = mangle(select->cond());
        if (auto lit = cond->isa<PrimLit>())
            ops[0] = mangle(lit->value().get_u1().get() ? select->tval() : select->fval());
        else
            ops[0] = mangle(select); //world.select(cond, mangle(select->tval()), mangle(select->fval()));
    } else
        ops[0] = mangle(olambda->to());

    ArrayRef<Def> nargs(ops.slice_from_begin(1));// new args of nlambda
    Def ntarget = ops.front();                   // new target of nlambda

    // check whether we can optimize tail recursion
    if (ntarget == oentry) {
        bool substitute = true;
        for (size_t i = 0, e = to_drop.size(); i != e && substitute; ++i)
            substitute &= nargs[to_drop[i]] == drop_with[i];

        if (substitute)
            return nlambda->jump(nentry, nargs.cut(to_drop));
    }

    nlambda->jump(ntarget, nargs);
}

enum class Eval { Run, Infer, Halt };

Def Mangler::mangle(Def odef) {
    if (!pass1.contains(odef) && !pass2.contains(odef))
        return odef;
    if (pass2.contains(odef))
        return lookup(odef);

    if (auto olambda = odef->isa_lambda()) {
        assert(scope.contains(olambda));
        return mangle_head(olambda);
    } else if (auto param = odef->isa<Param>()) {
        assert(scope.contains(param->lambda()));
        return pass2[odef] = odef;
    }

    auto oprimop = odef->as<PrimOp>();
    Array<Def> nops(oprimop->size());
    Eval eval = Eval::Infer;
    for (size_t i = 0, e = oprimop->size(); i != e; ++i) {
        auto op = mangle(oprimop->op(i));

        if (auto evalop = op->isa<EvalOp>()) {
            if (evalop->isa<Run>() && eval == Eval::Infer)
                eval = Eval::Run;
            else {
                assert(evalop->isa<Halt>());
                eval = Eval::Halt;
            }
            op = evalop->def();
        }

        nops[i] = op;
    }

    auto nprimop = world.rebuild(oprimop, nops);
    if (eval == Eval::Run) 
        nprimop = world.run(nprimop);
    else if (eval == Eval::Halt)
        nprimop = world.halt(nprimop);

    return pass2[oprimop] = nprimop;
}

//------------------------------------------------------------------------------

Lambda* mangle(const Scope& scope,
               Def2Def& mapping,
               ArrayRef<size_t> to_drop,
               ArrayRef<Def> drop_with,
               ArrayRef<Def> to_lift,
               const GenericMap& generic_map) {
    return Mangler(scope, mapping, to_drop, drop_with, to_lift, generic_map).mangle();
}

Lambda* drop(const Scope& scope, Def2Def& mapping, ArrayRef<Def> with) {
    size_t size = with.size();
    Array<size_t> to_drop(size);
    for (size_t i = 0; i != size; ++i)
        to_drop[i] = i;

    return mangle(scope, mapping, to_drop, with, Array<Def>(), GenericMap());
}

//------------------------------------------------------------------------------

}
