#include "../include/backend/RISCVTrival.hpp"

RISCVMIR* RISCVTrival::CopyFrom(VirRegister* dst,RISCVMOperand* src){
    RISCVMIR* copyinst = nullptr;
    if(dst->GetType()==RISCVType::riscv_i32 && src->GetType()==RISCVType::riscv_i32) {
        copyinst=new RISCVMIR(RISCVMIR::mv);
    }
    else if(dst->GetType()==RISCVType::riscv_float32 && src->GetType()==RISCVType::riscv_float32) {
        copyinst=new RISCVMIR(RISCVMIR::_fmv_s);
    }
    else assert(0&&"Error Type!");
        copyinst->SetDef(dst);
        copyinst->AddOperand(src);
    return copyinst;
}