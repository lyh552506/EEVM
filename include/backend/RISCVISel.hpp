#pragma once
#include "../../include/backend/BackendPass.hpp"
#include "../../include/lib/CFG.hpp"
#include "../../include/backend/RISCVContext.hpp"
#include "../../include/backend/RISCVRegister.hpp"
#include "../../include/backend/RISCVMIR.hpp"
#include "../../include/lib/BaseCFG.hpp"
#include <algorithm>

void LowerFormalArguments(Function* func, RISCVLoweringContext& ctx);

class RISCVISel:public BackEndPass<Function>{
    RISCVLoweringContext& ctx;
    RISCVMIR* Builder(RISCVMIR::RISCVISA,User*);
    RISCVMIR* Builder_withoutDef(RISCVMIR::RISCVISA,User*);
    RISCVMIR* Builder(RISCVMIR::RISCVISA,std::initializer_list<RISCVMOperand*>);
    RISCVMIR* Builder_withoutDef(RISCVMIR::RISCVISA _isa,std::initializer_list<RISCVMOperand*> list);

    // return the virreg which handles the condition
    // will register to the mapping automatically
    void condition_helper(BinaryInst*);

    void InstLowering(User*);
    void InstLowering(AllocaInst*);
    void InstLowering(StoreInst*);
    void InstLowering(LoadInst*);
    void InstLowering(FPTSI*);
    void InstLowering(SITFP*);
    void InstLowering(UnCondInst*);
    void InstLowering(CondInst*);
    void InstLowering(BinaryInst*);
    //
    void InstLowering(GetElementPtrInst*);
    //
    void InstLowering(PhiInst*);
    //
    void InstLowering(CallInst*);
    void InstLowering(RetInst*);

    void InstLowering(ZextInst*);
 
    RISCVMOperand* Li_Intimm(ConstIRInt* Intconst);
    public:
    RISCVISel(RISCVLoweringContext&);
    bool run(Function*);
};

