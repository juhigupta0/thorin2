#include "thorin/pass/optimize.h"

#include "thorin/pass/fp/eta_red.h"
#include "thorin/pass/fp/tail_rec_elim.h"
#include "thorin/pass/pipelinebuilder.h"
#include "thorin/pass/rw/lam_spec.h"
#include "thorin/pass/rw/scalarize.h"
#include "thorin/phase/phase.h"

namespace thorin {

void optimize(World& world, PipelineBuilder& builder) {
    Pipeline pipe(world);
    pipe.add<PassManPhase>(builder.opt_prep_phase1(world));
    pipe.add<Scalerize>();
    pipe.add<EtaRed>();
    pipe.add<TailRecElim>();
    pipe.add<PassManPhase>(builder.opt_prep_phase2(world));
    pipe.add<PassManPhase>(builder.opt_phase(world));
    pipe.add<LamSpec>();
    pipe.add<PassManPhase>(builder.codegen_prep_phase(world));
    // pipe.add<PassManPhase>(builder.opt_phase2(world));
    pipe.run();
}

} // namespace thorin
