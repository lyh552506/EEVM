#include "../include/backend/RISCVFrameContext.hpp"
std::string& NamedMOperand::GetName() {return name;}
NamedMOperand::NamedMOperand(std::string _name,RISCVType _tp):RISCVMOperand(_tp),name(_name){}

void NamedMOperand::print(){
    std::cout<<name;
}

RISCVObject::RISCVObject(Type* _tp,std::string _name):NamedMOperand(_name,riscv_ptr),tp(_tp){}
RISCVObject::RISCVObject(std::string _name):NamedMOperand(_name,riscv_ptr) {}
RISCVGlobalObject::RISCVGlobalObject(Type* _tp,std::string _name):RISCVObject(_tp,_name){
    local=false;
}
void RISCVGlobalObject::print(){
    // std::cout<<"***";
    // tp->print();
    // std::cout<<"GlobalObject:";
    NamedMOperand::print();
    // std::cout<<"***\n";
    std::cout<<"\n";
}

RISCVTempFloatObject::RISCVTempFloatObject(std::string _name):RISCVObject(FloatType::NewFloatTypeGet(), _name){
    local=true;
}
void RISCVTempFloatObject::print() {}

// RISCVFrameObject::RISCVFrameObject(Type* _tp,std::string _name):RISCVObject(_tp,_name){
//     local=true;
// }

RISCVFrameObject::RISCVFrameObject(Value* val) : RISCVMOperand(RISCVTyper(val->GetType())) {
    name = val->GetName();
    reg = new StackRegister(this, PhyRegister::PhyReg::s0, begin_addr_offsets);
    if (PointerType* pointtype = dynamic_cast<PointerType*>(val->GetType())) {
        size=pointtype->GetSubType()->get_size();
    }
    // will we meet arraytype here?
    else if (ArrayType* arrtype = dynamic_cast<ArrayType*>(val->GetType())) {
        size=arrtype->get_size();
    }
    else size=val->GetType()->get_size();
    contextype = RISCVTyper(val->GetType());
}
RISCVFrameObject::RISCVFrameObject(VirRegister* val) : RISCVMOperand(val->GetType()) {
    reg = new StackRegister(this, PhyRegister::PhyReg::s0, begin_addr_offsets);
    name = val->GetName();
    contextype = val->GetType();
    size = 8;
}
RISCVFrameObject::RISCVFrameObject(PhyRegister* preg) : RISCVMOperand(preg->GetType()){
    reg = new StackRegister(this, PhyRegister::PhyReg::s0, begin_addr_offsets);
    name = preg->GetName();
    contextype = preg->GetType();
    size = 8;
}

void RISCVFrameObject::GenerateStackRegister(int offset) {
    reg->SetOffset(offset);
}
size_t RISCVFrameObject::GetFrameObjSize() {return size;}
size_t RISCVFrameObject::GetBeginAddOff() {return begin_addr_offsets;}
size_t RISCVFrameObject::GetEndAddOff() {return end_addr_offsets;}
RISCVType RISCVFrameObject::GetContextType() {return contextype;}
void RISCVFrameObject::SetBeginAddOff(size_t add) {begin_addr_offsets = add;}
void RISCVFrameObject::SetEndAddOff(size_t add) {end_addr_offsets = add;}
StackRegister*& RISCVFrameObject::GetStackReg() {return reg;}
void RISCVFrameObject::print(){
    // std::cout<<"---";
    // std::cout<<"FrameObject";
    // std::cout<<"---";
    reg->print();
}

BegAddrRegister::BegAddrRegister(RISCVFrameObject* _frameobj)
    : Register(riscv_i32), frameobj(_frameobj) {}

// BegAddrRegister::BegAddrRegister(size_t offset)
//     : Register(riscv_i32), frameobj(nullptr) {}

void BegAddrRegister::print() {
    // should be the minus of the begin address
  std::cout << "-" << frameobj->GetBeginAddOff();
}


StackRegister::StackRegister(RISCVFrameObject* obj, PhyRegister::PhyReg _regenum, int _offset)
    : Register(riscv_ptr), offset(_offset), parent(obj) {
  reg = PhyRegister::GetPhyReg(_regenum);
}
StackRegister::StackRegister(RISCVFrameObject* obj, VirRegister* _vreg, int _offset)
    : Register(riscv_ptr), offset(_offset), reg(_vreg), parent(obj) {}

StackRegister::StackRegister(PhyRegister::PhyReg _regenum, int _offset)
    : Register(riscv_ptr), offset(_offset) {
  reg = PhyRegister::GetPhyReg(_regenum);
}

StackRegister::StackRegister(VirRegister* _vreg, int _offset)
    : Register(riscv_ptr), offset(_offset), reg(_vreg) {}

void StackRegister::SetOffset(int _offset) { offset = _offset; }
RISCVFrameObject*& StackRegister::GetParent() {return parent;}
VirRegister* StackRegister::GetVreg() { 
  if(VirRegister* vreg = dynamic_cast<VirRegister*>(reg))
    return vreg;
  else return nullptr;
}
Register*& StackRegister::GetReg() { return reg; }
void StackRegister::SetPreg(PhyRegister* &_reg) { this->reg = _reg; }
void StackRegister::SetReg(Register* _reg) { this->reg = _reg; }
void StackRegister::print() {
  if(VirRegister* vreg = dynamic_cast<VirRegister*>(reg))  {
    std::cout << offset << "(";
    vreg->print();
    std::cout << ")";
  } else if(PhyRegister* preg = dynamic_cast<PhyRegister*>(reg)) {
    PhyRegister::PhyReg regenum = preg->Getregenum();
    std::cout << offset << "(" <<  magic_enum::enum_name(regenum) << ")";
  }
  else assert(false&&"Error: StackRegister::print");
}
bool StackRegister::isPhysical() {
  if(VirRegister* vreg = dynamic_cast<VirRegister*>(reg)) return false;
  else return true;
}