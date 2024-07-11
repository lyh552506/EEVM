#pragma once
#include "../../include/backend/BackendPass.hpp"
#include "../../include/backend/RISCVContext.hpp"
#include "../../include/backend/RISCVISel.hpp"
#include "../../include/backend/RISCVAsmPrinter.hpp"
#include "../../include/backend/RISCVISel.hpp"
#include "../../include/backend/RISCVRegister.hpp"
#include "../../include/backend/RegAlloc.hpp"
#include "../../include/lib/BaseCFG.hpp"
#include "../../include/backend/PhiElimination.hpp"
#include "../../include/backend/LegalizePass.hpp"
class RISCVModuleLowering:BackEndPass<Module>{
    // bool LoweringGlobalValue(Module*);
    RISCVLoweringContext ctx;
    void LowerGlobalArgument(Module*); 
    public:
    bool run(Module*);
};

class RISCVFunctionLowering:BackEndPass<Function>{
    RISCVLoweringContext& ctx;
    public:
    bool run(Function*);
    RISCVFunctionLowering(RISCVLoweringContext& ctx):ctx(ctx){};
};
