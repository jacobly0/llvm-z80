//===----- Z80MachineLateOptimization.cpp - Optimize late machine inst -----===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file defines a pass that optimizes machine instructions after register
// selection.
//
//===----------------------------------------------------------------------===//

#include "Z80.h"
#include "Z80RegisterInfo.h"
#include "llvm/CodeGen/MachineFunctionPass.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/CodeGen/MachineRegisterInfo.h"
#include "llvm/CodeGen/TargetInstrInfo.h"
#include "llvm/CodeGen/TargetSubtargetInfo.h"
using namespace llvm;

#define DEBUG_TYPE "z80-ml-opt"

namespace {
class Z80MachineLateOptimization : public MachineFunctionPass {
public:
  Z80MachineLateOptimization() : MachineFunctionPass(ID) {}
protected:
  bool runOnMachineFunction(MachineFunction &MF) override;

  MachineFunctionProperties getRequiredProperties() const override {
    return MachineFunctionProperties()
      .set(MachineFunctionProperties::Property::TracksLiveness);
  }

private:
  void computeKnownFlags(MachineInstr &MI, APInt &KnownZero,
                         APInt &KnownOne) const;

  StringRef getPassName() const override {
    return "Z80 Machine Late Optimization";
  }

  static const uint8_t Carry, Subtract, ParityOverflow, HalfCarry, Zero, Sign;
  static char ID;
};

const uint8_t Z80MachineLateOptimization::Carry = 1 << 0;
const uint8_t Z80MachineLateOptimization::Subtract = 1 << 0;
const uint8_t Z80MachineLateOptimization::ParityOverflow = 1 << 0;
const uint8_t Z80MachineLateOptimization::HalfCarry = 1 << 0;
const uint8_t Z80MachineLateOptimization::Zero = 1 << 0;
const uint8_t Z80MachineLateOptimization::Sign = 1 << 0;

char Z80MachineLateOptimization::ID = 0;
} // end anonymous namespace

FunctionPass *llvm::createZ80MachineLateOptimization() {
  return new Z80MachineLateOptimization();
}

bool Z80MachineLateOptimization::runOnMachineFunction(MachineFunction &MF) {
  const TargetSubtargetInfo &STI = MF.getSubtarget();
  assert(MF.getRegInfo().tracksLiveness() && "Liveness not being tracked!");
  const TargetInstrInfo *TII = STI.getInstrInfo();
  bool Changed = false;
  if (!MF.getRegInfo().isPhysRegModified(Z80::F))
    return Changed;
  bool OptSize = MF.getFunction()->getAttributes()
    .hasAttribute(AttributeList::FunctionIndex, Attribute::OptimizeForSize);
  APInt FlagsZero(8, 0), FlagsOne(8, 0);
  for (auto &MBB : MF) {
    bool UnusedFlags = true;
    for (MachineBasicBlock *Successor : MBB.successors())
      if (Successor->isLiveIn(Z80::F))
        UnusedFlags = false;
    for (auto I = MBB.rbegin(), E = MBB.rend(); I != E; ++I) {
      if (UnusedFlags)
        switch (I->getOpcode()) {
        case Z80::LD8ri: // ld a, 0 -> xor a, a
          if (I->getOperand(0).getReg() == Z80::A &&
              I->getOperand(1).getImm() == 0) {
            I->setDesc(TII->get(Z80::XOR8ar));
            I->RemoveOperand(1);
            I->getOperand(0).setIsUse();
            I->addImplicitDefUseOperands(MF);
            for (auto &Op : I->uses())
              Op.setIsUndef();
            FlagsZero = HalfCarry | Subtract | Carry;
            FlagsOne = 0;
            DEBUG(dbgs() << '!');
            Changed = true;
          }
          break;
        case Z80::LD24ri: // ld hl, -1/0 -> set-cf \ sbc hl, hl
          if (I->getOperand(0).getReg() == Z80::UHL &&
              I->getOperand(1).isImm()) {
            int Imm = I->getOperand(1).getImm();
            if (Imm == 0 || Imm == -1) {
              while (I->getNumOperands())
                I->RemoveOperand(0);
              I->setDesc(TII->get(Z80::SBC24aa));
              I->addImplicitDefUseOperands(MF);
              for (auto &Op : I->uses())
                Op.setIsUndef();
              MachineInstrBuilder MIB =
                BuildMI(MBB, I.getReverse(), I->getDebugLoc(),
                        TII->get(Imm ? Z80::SCF : Z80::OR8ar));
              if (!Imm)
                MIB.addReg(Z80::A);
              I = *MIB;
              for (auto &Op : I->uses())
                Op.setIsUndef();
              FlagsZero = (Imm & (Sign | HalfCarry | Carry)) | (~Imm & Zero) |
                ParityOverflow;
              FlagsOne = (~Imm & (Sign | HalfCarry | Carry)) | (Imm & Zero) |
                Subtract;
              DEBUG(dbgs() << '!');
              Changed = true;
            }
          }
          break;
        case Z80::POP24r: // push reg \ pop hl -> rcf \ sbc hl,hl \ add hl,reg
          if (!OptSize && I->getOperand(0).getReg() == Z80::UHL) {
            auto P = std::next(I);
            if (P != E && P->getOpcode() == Z80::PUSH24r &&
                Z80::O24RegClass.contains(P->getOperand(0).getReg())) {
              while (I->getNumOperands() != I->getNumExplicitOperands())
                I->RemoveOperand(I->getNumExplicitOperands());
              I->setDesc(TII->get(Z80::ADD24ao));
              I->addOperand(MachineOperand::CreateReg(Z80::UHL,
                                                      /*isDef*/false));
              I->addOperand(P->getOperand(0));
              I->addImplicitDefUseOperands(MF);
              I->findRegisterDefOperand(Z80::F)->setIsDead();
              while (P->getNumOperands())
                P->RemoveOperand(0);
              P->setDesc(TII->get(Z80::SBC24aa));
              P->addImplicitDefUseOperands(MF);
              for (auto &Op : P->uses())
                Op.setIsUndef();
              I = *BuildMI(MBB, P.getReverse(), P->getDebugLoc(),
                           TII->get(Z80::OR8ar)).addReg(Z80::A);
              for (auto &Op : I->uses())
                Op.setIsUndef();
              FlagsZero = Sign | ParityOverflow | Subtract | Carry;
              FlagsOne = Zero;
              DEBUG(dbgs() << '!');
              Changed = true;
            }
          }
          break;
        default:
          computeKnownFlags(*I, FlagsZero, FlagsOne);
          break;
        }
      UnusedFlags |= I->definesRegister(Z80::F);
      if (MachineOperand *FlagsUse = I->findRegisterUseOperand(Z80::F))
        UnusedFlags &= FlagsUse->isUndef();
      DEBUG(dbgs() << (UnusedFlags ? "unused" : "used") << '\t'; I->dump());
    }
    DEBUG(dbgs() << '\n');
  }
  return Changed;
}

void Z80MachineLateOptimization::
computeKnownFlags(MachineInstr &MI, APInt &KnownZero, APInt &KnownOne) const {
  switch (MI.getOpcode()) {
  default:
    if (MI.definesRegister(Z80::F)) {
      KnownZero = KnownOne = 0;
      DEBUG(dbgs() << '?');
    }
    break;
  case Z80::RCF:
    KnownZero = HalfCarry | Subtract | Carry;
    KnownOne = 0;
    break;
  case Z80::SCF:
    KnownZero = HalfCarry | Subtract;
    KnownOne = Carry;
    break;
  }
}
