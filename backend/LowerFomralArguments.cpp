#include "../include/backend/RISCVISel.hpp"
/// @todo 参数带有浮点数的情况

void LowerFormalArguments(Function* func, RISCVLoweringContext& ctx) {
    #define M(x) ctx.mapping(x)
    int IntMaxNum=8, FloatMaxNum=8;
    std::unique_ptr<RISCVFrame>& frame = ctx.GetCurFunction()->GetFrame();
    std::vector<std::unique_ptr<Value>>& params = func->GetParams();
    RISCVFunction* RISCVfunc = dynamic_cast<RISCVFunction*>(M(func));
    std::vector<int>& spillnodes = RISCVfunc->GetParamNeedSpill();
    if(!params.empty()) {
        RISCVType type;
        int offset = 0;
        for(auto it=spillnodes.rbegin(); it!=spillnodes.rend(); it++) {
            // load param from stack to reg
            type = RISCVTyper(params[*it]->GetType());
            VirRegister* vreg = ctx.createVReg(type);
            PhyRegister* preg = PhyRegister::GetPhyReg(PhyRegister::PhyReg::s0);
            StackRegister* stackreg = new StackRegister(PhyRegister::PhyReg::s0, offset);
            ctx.insert_val2mop(params[*it].get(), vreg);
            if(type == riscv_i32) {
                RISCVMIR* loadinst = new RISCVMIR(RISCVMIR::_lw);
                loadinst->SetDef(vreg);
                loadinst->AddOperand(stackreg);
                ctx(loadinst); 
                offset+=4;
            }
            else if(type == riscv_float32) {
                RISCVMIR* loadinst = new RISCVMIR(RISCVMIR::_flw);
                loadinst->SetDef(vreg);
                loadinst->AddOperand(stackreg);
                ctx(loadinst); 
                offset+=4;
            }
            else if(type == riscv_ptr) {
                RISCVMIR* loadinst = new RISCVMIR(RISCVMIR::_ld);
                loadinst->SetDef(vreg);
                loadinst->AddOperand(stackreg);
                ctx(loadinst); 
                offset+=8;
            }
            else {
                // riscv_none
                assert(0&&"LowerFormalArguments: Error type");
            }
        }
        int regint=PhyRegister::PhyReg::a0;
        int regfloat=PhyRegister::PhyReg::fa0;
        for(int index=0;index<params.size();index++) {
            if(std::find(spillnodes.begin(), spillnodes.end(), index)!=spillnodes.end()) {
                continue;
            }
            else {
                RISCVType type = RISCVTyper(params[index]->GetType());
                VirRegister* vreg = ctx.createVReg(type);
                if(type==riscv_i32 || type==riscv_ptr) {
                    RISCVMIR* inst = new RISCVMIR(RISCVMIR::mv);
                    inst->SetDef(vreg);
                    inst->AddOperand(PhyRegister::GetPhyReg(static_cast<PhyRegister::PhyReg>(regint)));
                    ctx.insert_val2mop(params[index].get(), vreg);
                    ctx(inst);
                    regint++;
                }
                else if(type==riscv_float32) {
                    RISCVMIR* inst = new RISCVMIR(RISCVMIR::_fmv_s);
                    inst->SetDef(vreg);
                    inst->AddOperand(PhyRegister::GetPhyReg(static_cast<PhyRegister::PhyReg>(regfloat)));
                    ctx.insert_val2mop(params[index].get(), vreg);
                    ctx(inst);
                    regfloat++;
                }
                else {
                    assert(0&&"LowerFormalArguments: Error type");
                }
            }
        }
    }
    #undef M

    // std::vector<std::unique_ptr<Value>>& params = func->GetParams();
    // if (params.size()==0) {return;}
    // else {
    //     int paramnum=params.size();
    //     int min=paramnum>8?8:paramnum;
    //     if(paramnum>8) {
    //         int offset =0;
    //         for(int i=paramnum-1; i>min-1; --i) {
    //             if(params[i]->GetType()->GetTypeEnum()==InnerDataType::IR_Value_INT) {
    //                 auto loadinst = new RISCVMIR(RISCVMIR::_lw);
    //                 VirRegister* vreg = ctx.createVReg(RISCVTyper(params[i]->GetType()));
    //                 loadinst->SetDef(vreg);
    //                 PhyRegister* preg = PhyRegister::GetPhyReg(PhyRegister::PhyReg::s0);
    //                 StackRegister* stackreg = new StackRegister(PhyRegister::PhyReg::s0, offset);
    //                 loadinst->AddOperand(stackreg);
    //                 ctx.insert_val2mop(params[i].get(), vreg);
    //                 ctx(loadinst); 
    //                 offset+=4;
    //             }
    //             else if(params[i]->GetType()->GetTypeEnum()==InnerDataType::IR_Value_Float) {
    //                 auto loadinst = new RISCVMIR(RISCVMIR::_flw);
    //                 VirRegister* vreg = ctx.createVReg(RISCVTyper(params[i]->GetType()));
    //                 loadinst->SetDef(vreg);
    //                 PhyRegister* preg = PhyRegister::GetPhyReg(PhyRegister::PhyReg::s0);
    //                 StackRegister* stackreg = new StackRegister(PhyRegister::PhyReg::s0, offset);
    //                 loadinst->AddOperand(stackreg);
    //                 ctx.insert_val2mop(params[i].get(), vreg);
    //                 ctx(loadinst); 
    //                 offset+=4;
    //             }
    //             else if(params[i]->GetType()->GetTypeEnum()==InnerDataType::IR_PTR){
    //                 auto loadinst = new RISCVMIR(RISCVMIR::_ld);
    //                 VirRegister* vreg = ctx.createVReg(RISCVTyper(params[i]->GetType()));
    //                 loadinst->SetDef(vreg);
    //                 PhyRegister* preg = PhyRegister::GetPhyReg(PhyRegister::PhyReg::s0);
    //                 StackRegister* stackreg = new StackRegister(PhyRegister::PhyReg::s0, offset);
    //                 loadinst->AddOperand(stackreg);
    //                 ctx.insert_val2mop(params[i].get(), vreg);
    //                 ctx(loadinst);
    //                 offset+=8;
    //             }
    //             else assert(0&&"Error type in param");
    //         }
    //     }

    //     int regnum=PhyRegister::PhyReg::a0;
    //     for (int i=0; i<min; ++i) {
    //         VirRegister* vreg = ctx.createVReg(RISCVTyper(params[i]->GetType()));
    //         auto minst = new RISCVMIR(RISCVMIR::mv);
    //         minst->SetDef(vreg);
    //         minst->AddOperand(PhyRegister::GetPhyReg(static_cast<PhyRegister::PhyReg>(regnum)));
    //         ctx.insert_val2mop(params[i].get(), vreg);
    //         ctx(minst);
    //         regnum+=1;
    //     }
    // }
}