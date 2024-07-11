#include "../include/backend/RISCVISel.hpp"
#include "../include/backend/RISCVMIR.hpp"
#include "../include/backend/RISCVFrameContext.hpp"
RISCVMIR* RISCVISel::Builder(RISCVMIR::RISCVISA _isa,User* inst){
    auto minst=new RISCVMIR(_isa);
    minst->SetDef(ctx.mapping(inst));
    for(int i=0;i<inst->Getuselist().size();i++)
        minst->AddOperand(ctx.mapping(inst->GetOperand(i)));
    return minst;
}
RISCVMIR* RISCVISel::Builder_withoutDef(RISCVMIR::RISCVISA _isa,User* inst){
    auto minst=new RISCVMIR(_isa);
    for(int i=0;i<inst->Getuselist().size();i++)
        minst->AddOperand(ctx.mapping(inst->GetOperand(i)));
    return minst;
}
RISCVMIR* RISCVISel::Builder_withoutDef(RISCVMIR::RISCVISA _isa,std::initializer_list<RISCVMOperand*> list) {
    auto minst=new RISCVMIR(_isa);
    for(auto it=list.begin(); it!=list.end(); ++it) {
        RISCVMOperand* i=*it;
        minst->AddOperand(i);
    }
    return minst;    
}

RISCVMIR* RISCVISel::Builder(RISCVMIR::RISCVISA _isa,std::initializer_list<RISCVMOperand*> list){
    auto minst=new RISCVMIR(_isa);
    minst->SetDef(list.begin()[0]); 
    for(auto it=list.begin()+1; it!=list.end(); ++it) {
        RISCVMOperand* i=*it;
        minst->AddOperand(i);
    }
    return minst;
}

bool RISCVISel::run(Function* m){
    if(m->GetParams().size()!=0) {
        RISCVBasicBlock* entry=RISCVBasicBlock::CreateRISCVBasicBlock();
        RISCVLoweringContext& ctx=this->ctx;
        ctx(entry);
        LowerFormalArguments(m,ctx);
        ctx.mapping(m->front())->as<RISCVBasicBlock>();
        RISCVMIR* uncondinst = new RISCVMIR(RISCVMIR::RISCVISA::_j);
        uncondinst->AddOperand(ctx.mapping(m->front())->as<RISCVBasicBlock>());
        entry->push_back(uncondinst);
    }
    for(auto i:*m){
        ctx(ctx.mapping(i)->as<RISCVBasicBlock>());
        for(auto inst:*i)
            InstLowering(inst);
    }
    return true;
}

void RISCVISel::InstLowering(AllocaInst* inst){
    ctx.mapping(inst);
}

void RISCVISel::InstLowering(StoreInst* inst){
    Operand op0 = inst->GetOperand(0);
    Operand op1 = inst->GetOperand(1);
    if(op0->GetType()==IntType::NewIntTypeGet()) {
        if(ConstIRInt* Intconst = dynamic_cast<ConstIRInt*>(op0)) {
            auto li = new RISCVMIR(RISCVMIR::li);
            VirRegister* vreg = new VirRegister(RISCVTyper(op0->GetType()));
            li->SetDef(vreg);
            Imm* imm = new Imm(Intconst);
            li->AddOperand(imm);
            ctx(li);

            auto minst=new RISCVMIR(RISCVMIR::_sw);
            minst->AddOperand(li->GetDef());
            minst->AddOperand(ctx.mapping(op1));
            ctx(minst);
        }
        else {
            ctx(Builder_withoutDef(RISCVMIR::_sw, inst));
            // auto minst=new RISCVMIR(RISCVMIR::_sw);
            // for(int i=0;i<inst->Getuselist().size();i++)
            //     minst->AddOperand(ctx.mapping(inst->GetOperand(i)));
            // ctx(minst);
        }
    }
    else if(op0->GetType()==FloatType::NewFloatTypeGet()) {
        if(ConstIRFloat* Floatconst = dynamic_cast<ConstIRFloat*>(op0)) {
            auto minst=new RISCVMIR(RISCVMIR::_fsw);
            minst->AddOperand(new Imm(Floatconst));
            minst->AddOperand(ctx.mapping(inst->GetOperand(1)));
            ctx(minst);
        }
        else {
            ctx(Builder_withoutDef(RISCVMIR::_fsw,inst));
            // auto minst=new RISCVMIR(RISCVMIR::_fsw);
            // for(int i=0;i<inst->Getuselist().size();i++)
            //     minst->AddOperand(ctx.mapping(inst->GetOperand(i)));
            // ctx(minst);
        }
    }
    else if(PointerType* ptrtype = dynamic_cast<PointerType*>(op0->GetType())) {
        ctx(Builder_withoutDef(RISCVMIR::_sd, inst));
    }
    else assert(0&&"invalid store type");
}

void RISCVISel::InstLowering(LoadInst* inst){
    if (inst->GetType()==IntType::NewIntTypeGet()) {
        ctx(Builder(RISCVMIR::_lw,inst));
    }
    else if  (inst->GetType()==FloatType::NewFloatTypeGet()) {
        ctx(Builder(RISCVMIR::_flw,inst));
    }
    else if(PointerType* ptrtype = dynamic_cast<PointerType*>(inst->GetType())) {
        ctx(Builder(RISCVMIR::_ld,inst));
    }
    // else if(ArrayType* arraytype = dynamic_cast<ArrayType*>(inst->GetType())) {
    //     ctx(Builder(RISCVMIR::_lw,inst));
    // }
    else assert(0&&"invalid load type");
}

void RISCVISel::InstLowering(FPTSI* inst){
    ctx(Builder(RISCVMIR::_fcvt_w_s,inst));
}

void RISCVISel::InstLowering(SITFP* inst){
    ctx(Builder(RISCVMIR::_fcvt_s_w,inst));
}

void RISCVISel::InstLowering(UnCondInst* inst){
    ctx(Builder_withoutDef(RISCVMIR::_j, {ctx.mapping(inst->GetOperand(0))}));
}

void RISCVISel::InstLowering(CondInst* inst){
    #define M(x) ctx.mapping(x)
    if (auto cond=inst->GetOperand(0)->as<ConstIRBoolean>()) {
        bool condition = cond->GetVal();
        if(condition) {
            ctx(Builder_withoutDef(RISCVMIR::_j, {ctx.mapping(inst->GetOperand(1))}));
        }
        else ctx(Builder_withoutDef(RISCVMIR::_j, {ctx.mapping(inst->GetOperand(2))}));
        return;
    }
    else if(auto cond=inst->GetOperand(0)->as<BinaryInst>()) {
        assert((cond->GetDef()->GetType()==BoolType::NewBoolTypeGet())&&"Invalid Condition Type");
        
        bool onlyusr=cond->GetUserListSize()==1;
        // CondInst is the only user of condition, and the condition is not float
        if(onlyusr&&cond->GetOperand(0)->GetType()!=FloatType::NewFloatTypeGet()){
            auto opcode=cond->getopration();
            auto integer_cond_builder=[&](RISCVMIR::RISCVISA opcode){
                ctx(Builder_withoutDef(opcode,{ctx.mapping(cond->GetOperand(0)),ctx.mapping(cond->GetOperand(1)),ctx.mapping(inst->GetOperand(1))}));
                ctx(Builder_withoutDef(RISCVMIR::_j,{ctx.mapping(inst->GetOperand(2))}));
            };
            switch (opcode)
            {
            case BinaryInst::Op_L:
            {
                // blt
                integer_cond_builder(RISCVMIR::_blt);
                break;
            }
            case BinaryInst::Op_LE:
            {
                // ble
                integer_cond_builder(RISCVMIR::_ble);
                break;
            }
            case BinaryInst::Op_G:
            {
                // bgt
                integer_cond_builder(RISCVMIR::_bgt);
                break;
            }
            case BinaryInst::Op_GE:
            {
                // bge
                integer_cond_builder(RISCVMIR::_bge);
                break;
            }
            case BinaryInst::Op_E:
            {
                // beq
                integer_cond_builder(RISCVMIR::_beq);
                break;
            }
            case BinaryInst::Op_NE:
            {
                // bne
                integer_cond_builder(RISCVMIR::_bne);
                break;
            }
            default:
                assert(0&&"Impossible");
            }
            return;
        }
        
        ctx(Builder_withoutDef(RISCVMIR::_bne, {ctx.mapping(inst->GetOperand(0)), PhyRegister::GetPhyReg(PhyRegister::PhyReg::zero), ctx.mapping(inst->GetOperand(1))}));
        ctx(Builder_withoutDef(RISCVMIR::_j,{ctx.mapping(inst->GetOperand(2))}));
    }
    else assert(0&&"Error in CondInst Lowering");
    #undef M
}

void RISCVISel::InstLowering(BinaryInst* inst){
    Operand temp = inst->GetOperand(0);
    if(inst->getopration()<BinaryInst::Op_And||inst->getopration()>BinaryInst::Op_Or){
        RISCVMIR* result;
        switch (inst->getopration())
        {
        case BinaryInst::Op_Add:
        {
            if(inst->GetType()==IntType::NewIntTypeGet()) {
                if(ConstIRInt* constint = dynamic_cast<ConstIRInt*>(inst->GetOperand(1))) {
                    // int inttemp = constint->GetVal();
                        ctx(Builder(RISCVMIR::_addiw,inst));
                    // if(inttemp<2048)
                    //     ctx(Builder(RISCVMIR::_addiw,inst));
                    // else if(inttemp>=2048) {
                    //     auto minst=new RISCVMIR(RISCVMIR::_add);
                    //     minst->SetDef(ctx.mapping(inst->GetDef()));
                    //     minst->AddOperand(ctx.mapping(inst->GetOperand(0)));
                    //     minst->AddOperand(Li_Intimm(constint));
                    //     ctx(minst);
                    // } else assert(0&&"Invalid constint in add inst"); 
                } else 
                    ctx(Builder(RISCVMIR::_addw,inst));
            } else if(inst->GetType()==FloatType::NewFloatTypeGet()) {
                ctx(Builder(RISCVMIR::_fadd_s,inst));
            }
            else assert("Illegal!");
            break;
        }
        case BinaryInst::Op_Sub:
        {
            if(inst->GetType()==IntType::NewIntTypeGet()) {
                if(ConstIRInt* constint = dynamic_cast<ConstIRInt*>(inst->GetOperand(1))) {
                    auto li = new RISCVMIR(RISCVMIR::li);
                    VirRegister* vreg = new VirRegister(riscv_i32);
                    li->SetDef(vreg);
                    Imm* imm = new Imm(constint);
                    li->AddOperand(imm);
                    ctx(li);

                    auto minst=new RISCVMIR(RISCVMIR::_subw);
                    minst->SetDef(ctx.mapping(inst->GetDef()));
                    minst->AddOperand(ctx.mapping(inst->GetOperand(0)));
                    // minst->AddOperand(Li_Intimm(constint));
                    minst->AddOperand(li->GetDef());
                    ctx(minst);
                } else ctx(Builder(RISCVMIR::_subw,inst));
            } else if(inst->GetType()==FloatType::NewFloatTypeGet())
                ctx(Builder(RISCVMIR::_fsub_s,inst));
            else assert("Illegal!");
            break;
        }
        case BinaryInst::Op_Mul:
        {
            if(inst->GetType()==IntType::NewIntTypeGet()) {
                if(ConstIRInt* constint = dynamic_cast<ConstIRInt*>(inst->GetOperand(1))) {
                    auto li = new RISCVMIR(RISCVMIR::li);
                    VirRegister* vreg = new VirRegister(riscv_i32);
                    li->SetDef(vreg);
                    Imm* imm = new Imm(constint);
                    li->AddOperand(imm);
                    ctx(li);

                    auto minst=new RISCVMIR(RISCVMIR::_mulw);
                    minst->SetDef(ctx.mapping(inst->GetDef()));
                    minst->AddOperand(ctx.mapping(inst->GetOperand(0)));
                    // minst->AddOperand(Li_Intimm(constint));
                    minst->AddOperand(li->GetDef());
                    ctx(minst);
                } else ctx(Builder(RISCVMIR::_mulw,inst));
            } else if(inst->GetType()==FloatType::NewFloatTypeGet())
                ctx(Builder(RISCVMIR::_fmul_s,inst));
            else assert("Illegal!");
            break;
        }
        case BinaryInst::Op_Div:
        {
            if(inst->GetType()==IntType::NewIntTypeGet()) {
                if(ConstIRInt* constint = dynamic_cast<ConstIRInt*>(inst->GetOperand(1))) {
                    auto li = new RISCVMIR(RISCVMIR::li);
                    VirRegister* vreg = new VirRegister(riscv_i32);
                    li->SetDef(vreg);
                    Imm* imm = new Imm(constint);
                    li->AddOperand(imm);
                    ctx(li);

                    auto minst=new RISCVMIR(RISCVMIR::_divw);
                    minst->SetDef(ctx.mapping(inst->GetDef()));
                    minst->AddOperand(ctx.mapping(inst->GetOperand(0)));
                    // minst->AddOperand(Li_Intimm(constint));
                    minst->AddOperand(li->GetDef());
                    ctx(minst);
                } else ctx(Builder(RISCVMIR::_divw,inst));            
            } else if(inst->GetType()==FloatType::NewFloatTypeGet())
                ctx(Builder(RISCVMIR::_fdiv_s,inst));
            else assert("Illegal!");
            break;
        }
        case BinaryInst::Op_Mod:
        {
            if(inst->GetType()==IntType::NewIntTypeGet()) {
                if(ConstIRInt* constint = dynamic_cast<ConstIRInt*>(inst->GetOperand(1))) {
                    auto li = new RISCVMIR(RISCVMIR::li);
                    VirRegister* vreg = ctx.createVReg(riscv_i32);
                    li->SetDef(vreg);
                    Imm* imm = new Imm(constint);
                    li->AddOperand(imm);
                    ctx(li);
                    
                    auto minst=new RISCVMIR(RISCVMIR::_remw);
                    minst->SetDef(ctx.mapping(inst->GetDef()));
                    minst->AddOperand(ctx.mapping(inst->GetOperand(0)));
                    // minst->AddOperand(Li_Intimm(constint));
                    minst->AddOperand(li->GetDef());
                    ctx(minst);
                } else ctx(Builder(RISCVMIR::_remw,inst));
            } else assert(0&&"Illegal!");
            break;
        }
        case BinaryInst::Op_L:
        case BinaryInst::Op_G:
        case BinaryInst::Op_LE:
        case BinaryInst::Op_GE:
        case BinaryInst::Op_E:
        case BinaryInst::Op_NE:
        {
            // unlikely
            if(inst->GetUserlist().is_empty())
                break;
            // if the only user is 
            bool onlyusr=inst->GetUserListSize()==1;
            bool condinst=false;
            for(auto us:inst->GetUserlist()){
                auto usr=us->GetUser();
                if(usr->as<CondInst>()==nullptr)
                    break;
                else condinst=true;
            }
            if(!(onlyusr==true&&condinst==true&&inst->GetOperand(0)->GetType()!=FloatType::NewFloatTypeGet()))
                condition_helper(inst);
            break;
        }
        default:
            assert(0&&"Invalid Opcode");
        }
    }
}

void RISCVISel::InstLowering(GetElementPtrInst* inst){
    #define M(x) ctx.mapping(x)
    // cast it to multiple add and mul first 
    /// @todo 循环不变量外提很重要了这里，之后会做一个循环不变量外提的优化
    int limi=inst->Getuselist().size();
    auto baseptr=M(inst->GetOperand(0));
    auto hasSubtype=dynamic_cast<HasSubType*>(inst->GetOperand(0)->GetType());
    size_t offset=0;
    using PhyReg=PhyRegister::PhyReg;
    using ISA = RISCVMIR::RISCVISA;
    PhyRegister* s0 = PhyRegister::GetPhyReg(PhyReg::s0);
    // PhyRegister* t0 = PhyRegister::GetPhyReg(PhyReg::t0);
    VirRegister* vreg= new VirRegister(RISCVType::riscv_i32);
    PhyRegister* zero = PhyRegister::GetPhyReg(PhyReg::zero);
    RISCVMIR* inst1 = nullptr;
    if(RISCVFrameObject* frameobj = dynamic_cast<RISCVFrameObject*>(baseptr)) {
        // addi .1 s0 beginaddregister
        inst1 = new RISCVMIR(ISA::_addi);
        BegAddrRegister* breg1 = new BegAddrRegister(frameobj);
        inst1->SetDef(vreg);
        inst1->AddOperand(s0);
        inst1->AddOperand(breg1);
        ctx(inst1);
    }
    else if (RISCVGlobalObject* globl = dynamic_cast<RISCVGlobalObject*>(baseptr)) {
        inst1 = new RISCVMIR(ISA::_add);
        inst1->SetDef(vreg);
        inst1->AddOperand(zero);
        inst1->AddOperand(baseptr);
        ctx(inst1);
    }
    else {
        // add .1 s0, .x
        inst1 = new RISCVMIR(ISA::_add);
        inst1->SetDef(vreg);
        // inst1->AddOperand(s0);
        inst1->AddOperand(zero);
        inst1->AddOperand(baseptr);
        ctx(inst1);
    }

    for(int i=1;i<limi;i++){
        // just make sure
        assert(hasSubtype!=nullptr&&"Impossible Here");
        size_t size=hasSubtype->GetSubType()->get_size();
        auto index=inst->GetOperand(i);
        if(index->isConst()){
            if(auto constindex=dynamic_cast<ConstIRInt*>(index))
                offset+=size*(size_t)constindex->GetVal();
            else assert("?Impossible Here");
        }
        else{
            // mul and add inst to compute to offset.
            // li .1 imm
            // mul .2 .x .1
            ConstIRInt* _size = ConstIRInt::GetNewConstant(int(size)); 
            auto li=Builder(RISCVMIR::li,{ctx.createVReg(RISCVType::riscv_i32), Imm::GetImm(_size)});
            ctx(li);
            auto mul=Builder(RISCVMIR::_mulw,{ctx.createVReg(RISCVType::riscv_ptr),M(index), li->GetDef()});
            ctx(mul);
            // add .base .base .2
            ctx(Builder(RISCVMIR::_add,{inst1->GetDef(),inst1->GetDef(),mul->GetDef()}));
        }
        hasSubtype=dynamic_cast<HasSubType*>(hasSubtype->GetSubType());
    }
    #undef M

    StackRegister* stackreg = nullptr;
    stackreg = new StackRegister(dynamic_cast<VirRegister*>(inst1->GetDef()), offset);
    // stackreg = new StackRegister(dynamic_cast<PhyRegister*>(inst1->GetDef())->Getregenum(), offset);
    
    // if (RISCVGlobalObject* globl = dynamic_cast<RISCVGlobalObject*>(baseptr)) {
    //     stackreg = new StackRegister(PhyRegister::t0, offset);
    // }
    // else {
    //     stackreg = new StackRegister(dynamic_cast<PhyRegister*>(inst1->GetDef())->Getregenum(), offset);
    // }

    /// @details Legalize StackRegister's offset togeter in LegalizePass
    // if(offset <= 2047) {
    //     stackreg = new StackRegister(dynamic_cast<VirRegister*>(inst1->GetDef()), offset);
    // }
    // else if(offset >2047) {
    //     int mod = offset % 2047;
    //     offset-=mod;
    //     VirRegister* vreg2 = ctx.createVReg(riscv_i32);
    //     Imm* imm2 = new Imm(ConstIRInt::GetNewConstant(offset));
    //     auto inst2=Builder(ISA::_addi, {vreg2,vreg2, imm2});
    //     ctx(inst2);

    //     stackreg = new StackRegister(dynamic_cast<VirRegister*>(inst1->GetDef()), mod);
    // }
    // else assert(0&&"error offset in GetElement inst");

    // replace the GetElementptr inst with stackreg (off(.1))
    ctx.change_mapping(ctx.mapping(inst->GetDef()), stackreg);
}

void RISCVISel::InstLowering(PhiInst* inst){
    ///@note phi elimination not here, no op
    return;
}

void RISCVISel::InstLowering(CallInst* inst){
    // 把VReg Copy到PhyReg
    // 如果溢出则按照规约store
    // 如果有返回值则copy a0 到 VReg
    #define M(x) ctx.mapping(x)
    int IntMaxNum=8, FloatMaxNum=8;
    std::unique_ptr<RISCVFrame>& frame = ctx.GetCurFunction()->GetFrame();
    Function* called_midfunc;
    RISCVFunction* called_func;
    BuildInFunction* buildin_func;
    std::vector<int> spillnodes;
    RISCVMIR* call = new RISCVMIR(RISCVMIR::call);
    call->AddOperand(M(inst->GetOperand(0)));

    if(called_midfunc = dynamic_cast<Function*>(inst->GetOperand(0))) {
        called_func = dynamic_cast<RISCVFunction*>(M(called_midfunc));
        spillnodes = called_func->GetParamNeedSpill();
    }
    else {
        buildin_func = dynamic_cast<BuildInFunction*>(inst->GetOperand(0));
        called_func = dynamic_cast<RISCVFunction*>(M(buildin_func));
        spillnodes = called_func->GetParamNeedSpill();
    }

    if(!inst->Getuselist().empty()) {
        int offset = 0;
        size_t local_param_size=0;
        for(auto it = spillnodes.rbegin(); it!=spillnodes.rend(); it++) {
            // spill to stack
            // load params to end of the frame
            int index = *it+1;
            Operand op = inst->GetOperand(index);
            RISCVMIR* store = nullptr;
            StackRegister* sreg = nullptr;
            switch(RISCVTyper(op->GetType())) {
                case riscv_i32:
                    sreg = new StackRegister(PhyRegister::sp, offset);
                    store = new RISCVMIR(RISCVMIR::_sw);
                    offset += 4;
                    break;
                case riscv_float32:
                    sreg = new StackRegister(PhyRegister::sp, offset);
                    store = new RISCVMIR(RISCVMIR::_fsw);
                    offset += 4;
                    break;
                case riscv_ptr:
                    sreg = new StackRegister(PhyRegister::sp, offset);
                    store = new RISCVMIR(RISCVMIR::_sd);
                    offset += 8;
                    break;
                default:
                    assert(0&&"Error param type");
            }
            store->AddOperand(M(inst->GetOperand(index)));
            store->AddOperand(sreg);
            ctx(store);

            local_param_size += op->GetType()->get_size();
        }
        if (local_param_size > ctx.GetCurFunction()->GetMaxParamSize()) {
            ctx.GetCurFunction()->SetMaxParamSize(local_param_size);
        }

        int regint=PhyRegister::PhyReg::a0;
        int regfloat=PhyRegister::PhyReg::fa0;
        for(int index=1;index<inst->Getuselist().size();index++) {
            if(std::find(spillnodes.begin(), spillnodes.end(), index-1)!=spillnodes.end()) {
                continue;
            }
            else {
                Operand op = inst->GetOperand(index);
                if(ConstIRInt* constint = dynamic_cast<ConstIRInt*>(op)) {
                    auto li = new RISCVMIR(RISCVMIR::li);
                    PhyRegister* preg = PhyRegister::GetPhyReg(static_cast<PhyRegister::PhyReg>(regint));
                    call->AddOperand(preg);
                    li->SetDef(preg);
                    Imm* imm = new Imm(constint);
                    li->AddOperand(imm);
                    ctx.insert_val2mop(constint, imm);
                    ctx(li);
                    regint++;
                }
                else if(RISCVTyper(op->GetType())==riscv_i32 || RISCVTyper(op->GetType())==riscv_ptr) {
                    PhyRegister* preg = PhyRegister::GetPhyReg(static_cast<PhyRegister::PhyReg>(regint));
                    ctx(Builder(RISCVMIR::mv,{preg,M(op)}));
                    call->AddOperand(preg);
                    regint++;
                }
                else if(ConstIRFloat* constfloat = dynamic_cast<ConstIRFloat*>(op)) {
                    auto load = new RISCVMIR(RISCVMIR::_fmv_s); 
                    // VirRegister* vreg = ctx.createVReg(RISCVTyper(op->GetType()));
                    PhyRegister* freg = PhyRegister::GetPhyReg(static_cast<PhyRegister::PhyReg>(regfloat));
                    call->AddOperand(freg);
                    load->SetDef(freg);
                    Imm* imm = new Imm(constfloat);
                    load->AddOperand(imm);
                    ctx.insert_val2mop(constint, imm);
                    ctx(load);
                    regfloat++;
                }
                else if(RISCVTyper(op->GetType())==riscv_float32) {
                    PhyRegister* freg = PhyRegister::GetPhyReg(static_cast<PhyRegister::PhyReg>(regfloat));
                    ctx(Builder(RISCVMIR::_fmv_s,{freg,M(op)}));
                    regfloat++;
                }
                else assert(0 && "CallInst selection illegal type");
            }
        }
    }

    ctx(call);

    if(!inst->GetUserlist().is_empty()){
        // ctx(Builder_withoutDef(RISCVMIR::call, inst));
        if(RISCVTyper(inst->GetOperand(0)->GetType())==RISCVType::riscv_float32) {
            ctx(Builder(RISCVMIR::_fmv_s, {ctx.mapping(inst), PhyRegister::GetPhyReg(PhyRegister::PhyReg::fa0)}));
        }
        else if(RISCVTyper(inst->GetOperand(0)->GetType())==RISCVType::riscv_i32){
            ctx(Builder(RISCVMIR::mv, {ctx.mapping(inst), PhyRegister::GetPhyReg(PhyRegister::PhyReg::a0)}));
        }
        else {
            // no return
        }
    }
    else {
        // ctx(Builder_withoutDef(RISCVMIR::call, inst));
    }
    #undef M
}

void RISCVISel::InstLowering(RetInst* inst){
    #define M(x) ctx.mapping(x)
    if(inst->Getuselist().empty() || inst->GetOperand(0)->isUndefVal()) {
        auto minst=new RISCVMIR(RISCVMIR::ret);
        ctx(minst);
    }
    else if(inst->GetOperand(0)&&inst->GetOperand(0)->GetType()==IntType::NewIntTypeGet()) {
        ctx(Builder(RISCVMIR::mv,{PhyRegister::GetPhyReg(PhyRegister::PhyReg::a0),M(inst->GetOperand(0))}));
        ctx(Builder_withoutDef(RISCVMIR::ret,inst));
    }
    else if(inst->GetOperand(0)&&inst->GetOperand(0)->GetType()==FloatType::NewFloatTypeGet()) {
        ctx(Builder(RISCVMIR::_fmv_s,{PhyRegister::GetPhyReg(PhyRegister::PhyReg::fa0),M(inst->GetOperand(0))}));
        ctx(Builder_withoutDef(RISCVMIR::ret,inst));
    }
    else 
        assert(0&&"Invalid return type");
    #undef M
}

RISCVMOperand* RISCVISel::Li_Intimm(ConstIRInt* Intconst) {
    // int inttemp = Intconst->GetVal();
    // VirRegister* vreg = ctx.createVReg(RISCVType::riscv_i32);
    // if(inttemp>=-2048 && inttemp<2048) {
    //     Imm* const_imm = new Imm(Intconst);
    //     ctx(Builder(RISCVMIR::li, {vreg, const_imm}));
    // } else {
    //     int mod = inttemp%4096;
    //     if(mod==0) {
    //         //li .a int
    //         Imm* const_imm = new Imm(Intconst);
    //         ctx(Builder(RISCVMIR::li, {vreg, const_imm}));
    //     } else if ((mod>0&&mod<2048)||(mod>=-2048&&mod<0)) {
    //         //li .a int-m
    //         //addi .a, .a, m
    //         Imm* const_imm = new Imm(ConstIRInt::GetNewConstant(inttemp-mod));
    //         ctx(Builder(RISCVMIR::li, {vreg, const_imm}));
    //         Imm* mod_imm = new Imm(ConstIRInt::GetNewConstant(mod));
    //         ctx(Builder(RISCVMIR::_addiw, {vreg, vreg, mod_imm}));
    //     } else if (mod >=2048 && mod <4096) {
    //         //li .a int-m+4096
    //         //addi .a, .a, m-4096
    //         Imm* const_imm = new Imm(ConstIRInt::GetNewConstant(inttemp-mod+4096));
    //         ctx(Builder(RISCVMIR::li, {vreg, const_imm}));
    //         Imm* mod_imm = new Imm(ConstIRInt::GetNewConstant(mod-4096));
    //         ctx(Builder(RISCVMIR::_addiw, {vreg, vreg, mod_imm}));
    //     }   
    //     else if (mod>=-4095&&mod<-2048) {
    //         //li .a int-m-4096
    //         //addi .a, .a, m+4096
    //         Imm* const_imm = new Imm(ConstIRInt::GetNewConstant(inttemp-mod-4096));
    //         ctx(Builder(RISCVMIR::li, {vreg, const_imm}));
    //         Imm* mod_imm = new Imm(ConstIRInt::GetNewConstant(mod+4096));
    //         ctx(Builder(RISCVMIR::_addiw, {vreg, vreg, mod_imm}));
    //     } else assert(0&&"error imm");
    // }
    // return vreg;
    assert(0&&"Unreachable");
}

void RISCVISel::InstLowering(ZextInst* zext){
    auto mreg=ctx.mapping(zext->GetOperand(0));
    ctx.insert_val2mop(zext,mreg);
}

void RISCVISel::InstLowering(User* inst){
    if(auto store=dynamic_cast<StoreInst*>(inst))InstLowering(store);
    else if(auto load=dynamic_cast<LoadInst*>(inst))InstLowering(load);
    else if(auto alloca=dynamic_cast<AllocaInst*>(inst))InstLowering(alloca);
    else if(auto fptsi=dynamic_cast<FPTSI*>(inst))InstLowering(fptsi);
    else if(auto sitfp=dynamic_cast<SITFP*>(inst))InstLowering(sitfp);
    else if(auto uncond=dynamic_cast<UnCondInst*>(inst))InstLowering(uncond);
    else if(auto cond=dynamic_cast<CondInst*>(inst))InstLowering(cond);
    else if(auto binary=dynamic_cast<BinaryInst*>(inst))InstLowering(binary);
    else if(auto gep=dynamic_cast<GetElementPtrInst*>(inst))InstLowering(gep);
    else if(auto phi=dynamic_cast<PhiInst*>(inst))InstLowering(phi);
    else if(auto call=dynamic_cast<CallInst*>(inst))InstLowering(call);
    else if(auto ret=dynamic_cast<RetInst*>(inst))InstLowering(ret);
    else if(auto zext=dynamic_cast<ZextInst*>(inst))InstLowering(zext);
    else assert(0&&"Invalid Inst Type");
}

RISCVISel::RISCVISel(RISCVLoweringContext& _ctx):ctx(_ctx){}

void RISCVISel::condition_helper(BinaryInst* inst){
    assert(inst->GetType()==BoolType::NewBoolTypeGet());
    assert(inst->GetOperand(0)->GetType()==inst->GetOperand(1)->GetType());
    assert(inst->GetOperand(0)->GetType()==BoolType::NewBoolTypeGet()||inst->GetOperand(0)->GetType()==IntType::NewIntTypeGet()||inst->GetOperand(1)->GetType()==FloatType::NewFloatTypeGet());
    bool isint=(inst->GetOperand(0)->GetType()!=FloatType::NewFloatTypeGet());
    switch (inst->getopration())
    {
    case BinaryInst::Op_E:{
        if(isint){
            // xor vreg0, a, b
            // seqz vreg1, vreg0
            auto vreg0=ctx.createVReg(riscv_i32);
            auto vreg1=ctx.createVReg(riscv_i32);
            ctx.insert_val2mop(inst->GetDef(),vreg1);
            ctx(Builder(RISCVMIR::_xor,{vreg0,ctx.mapping(inst->GetOperand(0)),ctx.mapping(inst->GetOperand(1))}));
            ctx(Builder(RISCVMIR::_seqz,{vreg1,vreg0}));
        }
        else{
            // feq.s vreg0, a, b
            auto vreg0=ctx.createVReg(riscv_i32);
            ctx.insert_val2mop(inst->GetDef(),vreg0);
            ctx(Builder(RISCVMIR::_feq_s,{vreg0,ctx.mapping(inst->GetOperand(0)),ctx.mapping(inst->GetOperand(1))}));
        }
        break;
    }
    case BinaryInst::Op_NE:{
        if(isint){
            // xor vreg0, a, b
            // snez vreg1, vreg0
            auto vreg0=ctx.createVReg(riscv_i32);
            auto vreg1=ctx.createVReg(riscv_i32);
            ctx.insert_val2mop(inst->GetDef(),vreg1);
            ctx(Builder(RISCVMIR::_xor,{vreg0,ctx.mapping(inst->GetOperand(0)),ctx.mapping(inst->GetOperand(1))}));
            ctx(Builder(RISCVMIR::_snez,{vreg1,vreg0}));
        }
        else{
            // feq.s vreg0, a, b
            // seqz vreg1, vreg0
            auto vreg0=ctx.createVReg(riscv_i32);
            auto vreg1=ctx.createVReg(riscv_i32);
            ctx.insert_val2mop(inst->GetDef(),vreg1);
            ctx(Builder(RISCVMIR::_feq_s,{vreg0,ctx.mapping(inst->GetOperand(0)),ctx.mapping(inst->GetOperand(1))}));
            ctx(Builder(RISCVMIR::_seqz,{vreg1,vreg0}));
        }
        break;
    }
    case BinaryInst::Op_G:{
        if(isint){
            // slt vreg0, b, a
            // if b<a, vreg0=1, that's a>b
            auto vreg0=ctx.createVReg(riscv_i32);
            ctx.insert_val2mop(inst->GetDef(),vreg0);
            ctx(Builder(RISCVMIR::_slt,{vreg0,ctx.mapping(inst->GetOperand(1)),ctx.mapping(inst->GetOperand(0))}));
        }
        else{
            // flt vreg0, b, a
            auto vreg0=ctx.createVReg(riscv_i32);
            ctx.insert_val2mop(inst->GetDef(),vreg0);
            ctx(Builder(RISCVMIR::_flt_s,{vreg0,ctx.mapping(inst->GetOperand(1)),ctx.mapping(inst->GetOperand(0))}));
        }
        break;
    }
    case BinaryInst::Op_GE:{
        if(isint){
            // slt vreg0, a, b: if a>=b, vreg0 will be 0
            // seqz vreg1, vreg0: thus vreg1 will be 1
            auto vreg0=ctx.createVReg(riscv_i32);
            auto vreg1=ctx.createVReg(riscv_i32);
            ctx.insert_val2mop(inst->GetDef(),vreg1);
            ctx(Builder(RISCVMIR::_slt,{vreg0,ctx.mapping(inst->GetOperand(0)),ctx.mapping(inst->GetOperand(1))}));
            ctx(Builder(RISCVMIR::_seqz,{vreg1,vreg0}));
        }
        else{
            // fle vreg0 b, a
            auto vreg0=ctx.createVReg(riscv_i32);
            ctx.insert_val2mop(inst->GetDef(),vreg0);
            ctx(Builder(RISCVMIR::_fle_s,{vreg0,ctx.mapping(inst->GetOperand(1)),ctx.mapping(inst->GetOperand(0))}));
        }
        break;
    }
    case BinaryInst::Op_LE:{
        if(isint){
            // slt vreg0, b, a: if a<=b vreg0 will be 0
            // seqz vreg1, vreg0: thus vreg1 will be 1
            auto vreg0=ctx.createVReg(riscv_i32);
            auto vreg1=ctx.createVReg(riscv_i32);
            ctx.insert_val2mop(inst->GetDef(),vreg1);
            ctx(Builder(RISCVMIR::_slt,{vreg0,ctx.mapping(inst->GetOperand(1)),ctx.mapping(inst->GetOperand(0))}));
            ctx(Builder(RISCVMIR::_seqz,{vreg1,vreg0}));
        }
        else{
            // fle.s
            auto vreg0=ctx.createVReg(riscv_i32);
            ctx.insert_val2mop(inst->GetDef(),vreg0);
            ctx(Builder(RISCVMIR::_fle_s,{vreg0,ctx.mapping(inst->GetOperand(0)),ctx.mapping(inst->GetOperand(1))}));
        }
        break;
    }
    case BinaryInst::Op_L:{
        if(isint){
            // slt vreg0, a, b: if a<b, vreg0 will be 1
            auto vreg0=ctx.createVReg(riscv_i32);
            ctx.insert_val2mop(inst->GetDef(),vreg0);
            ctx(Builder(RISCVMIR::_slt,{vreg0,ctx.mapping(inst->GetOperand(0)),ctx.mapping(inst->GetOperand(1))}));
        }
        else{
            // flt.s
            auto vreg0=ctx.createVReg(riscv_i32);
            ctx.insert_val2mop(inst->GetDef(),vreg0);
            ctx(Builder(RISCVMIR::_flt_s,{vreg0,ctx.mapping(inst->GetOperand(0)),ctx.mapping(inst->GetOperand(1))}));
        }
        break;
    }
    default:
        assert(0&&"Invalid operation");
    }
}