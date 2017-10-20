//===-- Z80InstrInfo.cpp - Z80 Instruction Information --------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file contains the Z80 implementation of the TargetInstrInfo class.
//
//===----------------------------------------------------------------------===//

#include "Z80InstrInfo.h"
#include "Z80.h"
#include "Z80Subtarget.h"
#include "llvm/CodeGen/MachineFrameInfo.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/CodeGen/MachineRegisterInfo.h"
#include "llvm/MC/MCExpr.h"
#include "llvm/MC/MCInst.h"
using namespace llvm;

#define DEBUG_TYPE "z80-instr-info"

#define GET_INSTRINFO_CTOR_DTOR
#include "Z80GenInstrInfo.inc"

// Pin the vtable to this file.
void Z80InstrInfo::anchor() {}

Z80InstrInfo::Z80InstrInfo(Z80Subtarget &STI)
    : Z80GenInstrInfo((STI.is24Bit() ? Z80::ADJCALLSTACKDOWN24
                                     : Z80::ADJCALLSTACKDOWN16),
                      (STI.is24Bit() ? Z80::ADJCALLSTACKUP24
                                     : Z80::ADJCALLSTACKUP16)),
      Subtarget(STI), RI(STI.getTargetTriple()) {
}

int Z80InstrInfo::getSPAdjust(const MachineInstr &MI) const {
  switch (MI.getOpcode()) {
  case Z80::POP16r:
  case Z80::POP16AF:
    return 2;
  case Z80::POP24r:
  case Z80::POP24AF:
    return 3;
  case Z80::PUSH16r:
  case Z80::PUSH16AF:
  case Z80::PEA16o:
    return -2;
  case Z80::PUSH24r:
  case Z80::PUSH24AF:
  case Z80::PEA24o:
    return -3;
  }
  return TargetInstrInfo::getSPAdjust(MI);
}

static bool isIndex(const MachineOperand &MO, const MCRegisterInfo &RI) {
  if (MO.isFI())
    return true;
  if (MO.isReg())
    for (unsigned IndexReg : Z80::I24RegClass)
      if (RI.isSubRegisterEq(IndexReg, MO.getReg()))
        return true;
  return false;
}

static bool hasIndex(const MachineInstr &MI, const MCRegisterInfo &RI) {
  for (const MachineOperand &Op : MI.explicit_operands())
    if (isIndex(Op, RI))
      return true;
  return false;
}

unsigned Z80InstrInfo::getInstSizeInBytes(const MachineInstr &MI) const {
  auto TSFlags = MI.getDesc().TSFlags;
  // 1 byte for opcode
  unsigned Size = 1;
  // 1 byte if we need a suffix
  switch (TSFlags & Z80II::ModeMask) {
  case Z80II::AnyMode:
  case Z80II::CurMode:
    break;
  case Z80II::Z80Mode:
    Size += Subtarget.is24Bit();
    break;
  case Z80II::EZ80Mode:
    Size += Subtarget.is16Bit();
    break;
  }
  // prefix byte(s)
  unsigned Prefix = TSFlags & Z80II::PrefixMask;
  if (TSFlags & Z80II::IndexedIndexPrefix)
    Size += isIndex(MI.getOperand(Prefix >> Z80II::PrefixShift),
                    getRegisterInfo());
  else
    switch (Prefix) {
    case Z80II::NoPrefix:
      break;
    case Z80II::CBPrefix:
    case Z80II::DDPrefix:
    case Z80II::EDPrefix:
    case Z80II::FDPrefix:
      Size += 1;
      break;
    case Z80II::DDCBPrefix:
    case Z80II::FDCBPrefix:
      Size += 2;
      break;
    case Z80II::AnyIndexPrefix:
      Size += hasIndex(MI, getRegisterInfo());
      break;
    }
  // immediate byte(s)
  if (TSFlags & Z80II::HasImm)
    switch (TSFlags & Z80II::ModeMask) {
    case Z80II::AnyMode:
      Size += 1;
      break;
    case Z80II::CurMode:
      Size += Subtarget.is24Bit() ? 3 : 2;
      break;
    case Z80II::Z80Mode:
      Size += 2;
      break;
    case Z80II::EZ80Mode:
      Size += 3;
      break;
    }
  // 1 byte if we need an offset
  if (TSFlags & Z80II::HasOff)
    Size += 1;
  return Size;
}

/// Return the inverse of the specified condition,
/// e.g. turning COND_E to COND_NE.
Z80::CondCode Z80::GetOppositeBranchCondition(Z80::CondCode CC) {
  return Z80::CondCode(CC ^ 1);
}

bool Z80InstrInfo::isUnpredicatedTerminator(const MachineInstr &MI) const {
  if (!MI.isTerminator()) return false;

  // Conditional branch is a special case.
  if (MI.isBranch() && !MI.isBarrier())
    return true;
  if (!MI.isPredicable())
    return true;
  return !isPredicated(MI);
}

bool Z80InstrInfo::analyzeBranch(MachineBasicBlock &MBB,
                                 MachineBasicBlock *&TBB,
                                 MachineBasicBlock *&FBB,
                                 SmallVectorImpl<MachineOperand> &Cond,
                                 bool AllowModify) const {
  // Start from the bottom of the block and work up, examining the
  // terminator instructions.
  MachineBasicBlock::iterator I = MBB.end(), UnCondBrIter = I;
  while (I != MBB.begin()) {
    --I;
    if (I->isDebugValue())
      continue;

    // Working from the bottom, when we see a non-terminator instruction, we're
    // done.
    if (!isUnpredicatedTerminator(*I))
      break;

    // A terminator that isn't a branch can't easily be handled by this
    // analysis.
    if (!I->isBranch())
      return true;

    // Cannot handle branches that don't branch to a block.
    if (!I->getOperand(0).isMBB())
      return true;

    // Handle unconditional branches.
    if (I->getNumOperands() == 1) {
      UnCondBrIter = I;

      if (!AllowModify) {
        TBB = I->getOperand(0).getMBB();
        continue;
      }

      // If the block has any instructions after a JMP, delete them.
      while (std::next(I) != MBB.end())
        std::next(I)->eraseFromParent();
      Cond.clear();
      FBB = nullptr;

      // Delete the JMP if it's equivalent to a fall-through.
      if (MBB.isLayoutSuccessor(I->getOperand(0).getMBB())) {
        TBB = nullptr;
        I->eraseFromParent();
        I = MBB.end();
        UnCondBrIter = I;
        continue;
      }

      // TBB is used to indicate the unconditional destination.
      TBB = I->getOperand(0).getMBB();
      continue;
    }

    // Handle conditional branches.
    assert(I->getNumExplicitOperands() == 2 && "Invalid conditional branch");
    Z80::CondCode BranchCode = Z80::CondCode(I->getOperand(1).getImm());

    // Working from the bottom, handle the first conditional branch.
    if (Cond.empty()) {
      MachineBasicBlock *TargetBB = I->getOperand(0).getMBB();
      if (AllowModify && UnCondBrIter != MBB.end() &&
          MBB.isLayoutSuccessor(TargetBB)) {
        // If we can modify the code and it ends in something like:
        //
        //     jCC L1
        //     jmp L2
        //   L1:
        //     ...
        //   L2:
        //
        // Then we can change this to:
        //
        //     jnCC L2
        //   L1:
        //     ...
        //   L2:
        //
        // Which is a bit more efficient.
        // We conditionally jump to the fall-through block.
        BranchCode = GetOppositeBranchCondition(BranchCode);
        MachineBasicBlock::iterator OldInst = I;

        BuildMI(MBB, UnCondBrIter, MBB.findDebugLoc(I), get(Z80::JQCC))
          .addMBB(UnCondBrIter->getOperand(0).getMBB()).addImm(BranchCode);
        BuildMI(MBB, UnCondBrIter, MBB.findDebugLoc(I), get(Z80::JQ))
          .addMBB(TargetBB);

        OldInst->eraseFromParent();
        UnCondBrIter->eraseFromParent();

        // Restart the analysis.
        UnCondBrIter = MBB.end();
        I = MBB.end();
        continue;
      }

      FBB = TBB;
      TBB = I->getOperand(0).getMBB();
      Cond.push_back(MachineOperand::CreateImm(BranchCode));
      continue;
    }

    return true;
  }

  return false;
}

unsigned Z80InstrInfo::removeBranch(MachineBasicBlock &MBB,
                                    int *BytesRemoved) const {
  assert(!BytesRemoved && "code size not handled");
  MachineBasicBlock::iterator I = MBB.end();
  unsigned Count = 0;

  while (I != MBB.begin()) {
    --I;
    if (I->isDebugValue())
      continue;
    if (I->getOpcode() != Z80::JQ &&
        I->getOpcode() != Z80::JQCC)
      break;
    // Remove the branch.
    I->eraseFromParent();
    I = MBB.end();
    ++Count;
  }

  return Count;
}

unsigned Z80InstrInfo::insertBranch(MachineBasicBlock &MBB,
                                    MachineBasicBlock *TBB,
                                    MachineBasicBlock *FBB,
                                    ArrayRef<MachineOperand> Cond,
                                    const DebugLoc &DL,
                                    int *BytesAdded) const {
  // Shouldn't be a fall through.
  assert(TBB && "InsertBranch must not be told to insert a fallthrough");
  assert(Cond.size() <= 1 && "Z80 branch conditions have one component!");
  assert(!BytesAdded && "code size not handled");

  if (Cond.empty()) {
    // Unconditional branch?
    assert(!FBB && "Unconditional branch with multiple successors!");
    BuildMI(&MBB, DL, get(Z80::JQ)).addMBB(TBB);
    return 1;
  }

  // Conditional branch.
  unsigned Count = 0;
  BuildMI(&MBB, DL, get(Z80::JQCC)).addMBB(TBB).addImm(Cond[0].getImm());
  ++Count;

  // If FBB is null, it is implied to be a fall-through block.
  if (FBB) {
    // Two-way Conditional branch. Insert the second branch.
    BuildMI(&MBB, DL, get(Z80::JQ)).addMBB(FBB);
    ++Count;
  }
  return Count;
}

bool Z80InstrInfo::
reverseBranchCondition(SmallVectorImpl<MachineOperand> &Cond) const {
  assert(Cond.size() == 1 && "Invalid Z80 branch condition!");
  Z80::CondCode CC = static_cast<Z80::CondCode>(Cond[0].getImm());
  Cond[0].setImm(GetOppositeBranchCondition(CC));
  return false;
}

bool Z80::splitReg(
    unsigned ByteSize, unsigned Opc8, unsigned Opc16, unsigned Opc24,
    unsigned &RC, unsigned &LoOpc, unsigned &LoIdx, unsigned &HiOpc,
    unsigned &HiIdx, unsigned &HiOff, bool Has16BitEZ80Ops) {
  switch (ByteSize) {
  default: llvm_unreachable("Unexpected Size!");
  case 1:
    RC = Z80::R8RegClassID;
    LoOpc = HiOpc = Opc8;
    LoIdx = HiIdx = Z80::NoSubRegister;
    HiOff = 0;
    return false;
  case 2:
    if (Has16BitEZ80Ops) {
      RC = Z80::R16RegClassID;
      LoOpc = HiOpc = Opc16;
      LoIdx = HiIdx = Z80::NoSubRegister;
      HiOff = 0;
      return false;
    }
    RC = Z80::R16RegClassID;
    LoOpc = HiOpc = Opc8;
    LoIdx = Z80::sub_low;
    HiIdx = Z80::sub_high;
    HiOff = 1;
    return true;
  case 3:
    // Legalization should have taken care of this if we don't have eZ80 ops
    //assert(Is24Bit && HasEZ80Ops && "Need eZ80 24-bit load/store");
    RC = Z80::R24RegClassID;
    LoOpc = HiOpc = Opc24;
    LoIdx = HiIdx = Z80::NoSubRegister;
    HiOff = 0;
    return false;
  }
}

bool Z80InstrInfo::canExchange(unsigned RegA, unsigned RegB) const {
  // The only regs that can be directly exchanged are DE and HL, in any order.
  bool DE = false, HL = false;
  for (unsigned Reg : {RegA, RegB}) {
    if (RI.isSubRegisterEq(Z80::UDE, Reg))
      DE = true;
    else if (RI.isSubRegisterEq(Z80::UHL, Reg))
      HL = true;
  }
  return DE && HL;
}

void Z80InstrInfo::copyPhysReg(MachineBasicBlock &MBB,
                               MachineBasicBlock::iterator MI,
                               const DebugLoc &DL, unsigned DstReg,
                               unsigned SrcReg, bool KillSrc) const {
  DEBUG(dbgs() << RI.getName(DstReg) << " = " << RI.getName(SrcReg) << '\n');
  /*for (auto Regs : {std::make_pair(DstReg, &SrcReg),
                    std::make_pair(SrcReg, &DstReg)}) {
    if (Z80::R8RegClass.contains(Regs.first) &&
        (Z80::R16RegClass.contains(*Regs.second) ||
         Z80::R24RegClass.contains(*Regs.second)))
      *Regs.second = RI.getSubReg(*Regs.second, Z80::sub_low);
  }*/
  // Identity copy.
  if (DstReg == SrcReg)
    return;
  if (Z80::R8RegClass.contains(DstReg, SrcReg)) {
    // Byte copy.
    if (Z80::G8RegClass.contains(DstReg, SrcReg)) {
      // Neither are index registers.
      BuildMI(MBB, MI, DL, get(Z80::LD8gg), DstReg)
        .addReg(SrcReg, getKillRegState(KillSrc));
    } else if (Z80::I8RegClass.contains(DstReg, SrcReg)) {
      assert(Subtarget.hasIndexHalfRegs() && "Need  index half registers");
      // Both are index registers.
      if (Z80::X8RegClass.contains(DstReg, SrcReg)) {
        BuildMI(MBB, MI, DL, get(Z80::LD8xx), DstReg)
          .addReg(SrcReg, getKillRegState(KillSrc));
      } else if (Z80::Y8RegClass.contains(DstReg, SrcReg)) {
        BuildMI(MBB, MI, DL, get(Z80::LD8yy), DstReg)
          .addReg(SrcReg, getKillRegState(KillSrc));
      } else {
        // We are copying between different index registers, so we need to use
        // an intermediate register.
        BuildMI(MBB, MI, DL, get(Subtarget.is24Bit() ? Z80::PUSH24AF
                                                     : Z80::PUSH16AF));
        BuildMI(MBB, MI, DL, get(Z80::X8RegClass.contains(SrcReg) ? Z80::LD8xx
                                                                  : Z80::LD8yy),
                Z80::A).addReg(SrcReg, getKillRegState(KillSrc));
        BuildMI(MBB, MI, DL, get(Z80::X8RegClass.contains(DstReg) ? Z80::LD8xx
                                                                  : Z80::LD8yy),
                DstReg).addReg(Z80::A);
        BuildMI(MBB, MI, DL, get(Subtarget.is24Bit() ? Z80::POP24AF
                                                     : Z80::POP16AF));
      }
    } else {
      assert(Subtarget.hasIndexHalfRegs() && "Need  index half registers");
      // Only one is an index register, which isn't directly possible if one of
      // them is from HL.  If so, surround with EX DE,HL and use DE instead.
      bool NeedEX = false;
      for (unsigned *Reg : {&DstReg, &SrcReg}) {
        switch (*Reg) {
        case Z80::H: *Reg = Z80::D; NeedEX = true; break;
        case Z80::L: *Reg = Z80::E; NeedEX = true; break;
        }
      }
      bool Is24Bit = Subtarget.is24Bit();
      unsigned ExOpc = Is24Bit ? Z80::EX24DE : Z80::EX16DE;
      if (NeedEX) {
        // If the prev instr was an EX DE,HL, just kill it.
        if (MI != MBB.begin() && std::prev(MI)->getOpcode() == ExOpc)
          std::prev(MI)->eraseFromParent();
        else
          BuildMI(MBB, MI, DL, get(ExOpc))
            ->findRegisterUseOperand(Is24Bit ? Z80::UDE : Z80::DE)
            ->setIsUndef();
      }
      BuildMI(MBB, MI, DL,
              get(Z80::X8RegClass.contains(DstReg, SrcReg) ? Z80::LD8xx
                                                           : Z80::LD8yy),
              DstReg).addReg(SrcReg, getKillRegState(KillSrc));
      if (NeedEX)
        BuildMI(MBB, MI, DL, get(ExOpc))
          ->findRegisterUseOperand(Is24Bit ? Z80::UDE : Z80::DE)->setIsUndef();
    }
    return;
  }
  // Specialized word copy.
  bool Is24Bit = Z80::R24RegClass.contains(DstReg, SrcReg);
  // Copies to SP.
  if (DstReg == Z80::SPS || DstReg == Z80::SPL) {
    assert((Z80::A16RegClass.contains(SrcReg) || SrcReg == Z80::DE ||
            Z80::A24RegClass.contains(SrcReg) || SrcReg == Z80::UDE) &&
           "Unimplemented");
    if (SrcReg == Z80::DE || SrcReg == Z80::UDE)
      BuildMI(MBB, MI, DL, get(Is24Bit ? Z80::EX24DE : Z80::EX16DE))
        .addReg(DstReg, RegState::ImplicitDefine)
        .addReg(SrcReg, RegState::ImplicitKill);
    BuildMI(MBB, MI, DL, get(DstReg == Z80::SPL ? Z80::LD24SP : Z80::LD16SP))
      .addReg(SrcReg, getKillRegState(KillSrc));
    if (SrcReg == Z80::DE || SrcReg == Z80::UDE)
      BuildMI(MBB, MI, DL, get(Is24Bit ? Z80::EX24DE : Z80::EX16DE))
        .addReg(DstReg, RegState::ImplicitDefine)
        .addReg(SrcReg, RegState::ImplicitKill);
    return;
  }
  // Copies from SP.
  if (SrcReg == Z80::SPS || SrcReg == Z80::SPL) {
    assert((Z80::A16RegClass.contains(DstReg) || DstReg == Z80::DE ||
            Z80::A24RegClass.contains(DstReg) || DstReg == Z80::UDE) &&
           "Unimplemented");
    if (DstReg == Z80::DE || DstReg == Z80::UDE)
      BuildMI(MBB, MI, DL, get(Is24Bit ? Z80::EX24DE : Z80::EX16DE))
        .addReg(DstReg, RegState::ImplicitDefine)
        .addReg(SrcReg, RegState::ImplicitKill);
    BuildMI(MBB, MI, DL, get(Subtarget.is24Bit() ? Z80::LD24ri : Z80::LD16ri),
            DstReg).addImm(0);
    BuildMI(MBB, MI, DL, get(SrcReg == Z80::SPL ? Z80::ADD24SP : Z80::ADD16SP),
            DstReg).addReg(DstReg);
    if (DstReg == Z80::DE || DstReg == Z80::UDE)
      BuildMI(MBB, MI, DL, get(Is24Bit ? Z80::EX24DE : Z80::EX16DE))
        .addReg(DstReg, RegState::ImplicitDefine)
        .addReg(SrcReg, RegState::ImplicitKill);
    return;
  }
  if (Is24Bit == Subtarget.is24Bit()) {
    // Special case DE/HL = HL/DE<kill> as EX DE,HL.
    if (KillSrc && canExchange(DstReg, SrcReg)) {
      MachineInstrBuilder MIB = BuildMI(MBB, MI, DL,
                                        get(Is24Bit ? Z80::EX24DE
                                                    : Z80::EX16DE));
      MIB->findRegisterUseOperand(SrcReg)->setIsKill();
      MIB->findRegisterDefOperand(SrcReg)->setIsDead();
      MIB->findRegisterUseOperand(DstReg)->setIsUndef();
      return;
    }
    bool IsSrcIndexReg = Z80::I16RegClass.contains(SrcReg) ||
                         Z80::I24RegClass.contains(SrcReg);
    // Special case copies from index registers when we have eZ80 ops.
    if (Subtarget.hasEZ80Ops() && IsSrcIndexReg) {
      BuildMI(MBB, MI, DL, get(Is24Bit ? Z80::LEA24ro : Z80::LEA16ro), DstReg)
        .addReg(SrcReg, getKillRegState(KillSrc)).addImm(0);
      return;
    }
    // If both are 24-bit then the upper byte needs to be preserved.
    // Otherwise copies of index registers may need to use this method if:
    // - We are optimizing for size and exactly one reg is an index reg because
    //     PUSH SrcReg \ POP DstReg is (2 + NumIndexRegs) bytes but slower
    //     LD DstRegLo,SrcRegLo \ LD DstRegHi,SrcRegHi is 4 bytes but faster
    // - We don't have undocumented half index copies
    bool IsDstIndexReg = Z80::I16RegClass.contains(DstReg) ||
                         Z80::I24RegClass.contains(DstReg);
    unsigned NumIndexRegs = IsSrcIndexReg + IsDstIndexReg;
    bool OptSize = MBB.getParent()->getFunction()->getAttributes()
      .hasAttribute(AttributeList::FunctionIndex, Attribute::OptimizeForSize);
    if (Is24Bit || (NumIndexRegs == 1 && OptSize) ||
        (NumIndexRegs && !Subtarget.hasIndexHalfRegs())) {
      BuildMI(MBB, MI, DL, get(Is24Bit ? Z80::PUSH24r : Z80::PUSH16r))
        .addReg(SrcReg, getKillRegState(KillSrc));
      BuildMI(MBB, MI, DL, get(Is24Bit ? Z80::POP24r : Z80::POP16r), DstReg);
      return;
    }
  }
  // Otherwise, implement as two copies. A 16-bit copy should copy high and low
  // 8 bits separately.
  assert(Z80::R16RegClass.contains(DstReg, SrcReg) && "Unknown copy width");
  unsigned SubLo = Z80::sub_low;
  unsigned SubHi = Z80::sub_high;
  unsigned DstLoReg = RI.getSubReg(DstReg, SubLo);
  unsigned SrcLoReg = RI.getSubReg(SrcReg, SubLo);
  unsigned DstHiReg = RI.getSubReg(DstReg, SubHi);
  unsigned SrcHiReg = RI.getSubReg(SrcReg, SubHi);
  /*bool DstLoSrcHiOverlap = RI.regsOverlap(DstLoReg, SrcHiReg);
  bool SrcLoDstHiOverlap = RI.regsOverlap(SrcLoReg, DstHiReg);
  if (DstLoSrcHiOverlap && SrcLoDstHiOverlap) {
    assert(KillSrc &&
           "Both parts of SrcReg and DstReg overlap but not killing source!");
    // e.g. EUHL = LUDE so just swap the operands
    unsigned OtherReg;
    if (canExchange(DstLoReg, SrcLoReg)) {
      BuildMI(MBB, MI, DL, get(Subtarget.is24Bit() ? Z80::EX24DE : Z80::EX16DE))
        .addReg(DstReg, RegState::ImplicitDefine)
        .addReg(SrcReg, RegState::ImplicitKill);
    } else if ((OtherReg = DstLoReg, RI.isSubRegisterEq(Z80::UHL, SrcLoReg)) ||
               (OtherReg = SrcLoReg, RI.isSubRegisterEq(Z80::UHL, DstLoReg))) {
      BuildMI(MBB, MI, DL, get(Subtarget.is24Bit() ? Z80::PUSH24r : Z80::PUSH16r))
        .addReg(OtherReg, RegState::Kill);
      BuildMI(MBB, MI, DL, get(Subtarget.is24Bit() ? Z80::EX24SP : Z80::EX16SP));
      BuildMI(MBB, MI, DL, get(Subtarget.is24Bit() ? Z80::POP24r : Z80::POP16r),
              OtherReg);
    } else {
      BuildMI(MBB, MI, DL, get(Subtarget.is24Bit() ? Z80::PUSH24r : Z80::PUSH16r))
        .addReg(SrcLoReg, RegState::Kill);
      BuildMI(MBB, MI, DL, get(Subtarget.is24Bit() ? Z80::PUSH24r : Z80::PUSH16r))
        .addReg(DstLoReg, RegState::Kill);
      BuildMI(MBB, MI, DL, get(Subtarget.is24Bit() ? Z80::POP24r : Z80::POP16r),
              SrcLoReg);
      BuildMI(MBB, MI, DL, get(Subtarget.is24Bit() ? Z80::POP24r : Z80::POP16r),
              DstLoReg);
    }
    // Check if top needs to be moved (e.g. EUHL = HUDE).
    unsigned DstHiIdx = RI.getSubRegIndex(SrcLoReg, DstHiReg);
    unsigned SrcHiIdx = RI.getSubRegIndex(DstLoReg, SrcHiReg);
    if (DstHiIdx != SrcHiIdx)
      copyPhysReg(MBB, MI, DL, DstHiReg,
                  RI.getSubReg(DstLoReg, SrcHiIdx), KillSrc);
  } else if (DstLoSrcHiOverlap) {
    // Copy out SrcHi before SrcLo overwrites it.
    copyPhysReg(MBB, MI, DL, DstHiReg, SrcHiReg, KillSrc);
    copyPhysReg(MBB, MI, DL, DstLoReg, SrcLoReg, KillSrc);
  } else*/ {
    // If SrcLoDstHiOverlap then copy out SrcLo before SrcHi overwrites it,
    // otherwise the order doesn't matter.
    copyPhysReg(MBB, MI, DL, DstLoReg, SrcLoReg, KillSrc);
    copyPhysReg(MBB, MI, DL, DstHiReg, SrcHiReg, KillSrc);
  }
  --MI;
  MI->addRegisterDefined(DstReg, &RI);
  if (KillSrc)
    MI->addRegisterKilled(SrcReg, &RI, true);
}

static const MachineInstrBuilder &
addSubReg(const MachineInstrBuilder &MIB, unsigned Reg, unsigned Idx,
          const MCRegisterInfo *TRI, unsigned Flags = 0) {
  if (Idx && TargetRegisterInfo::isPhysicalRegister(Reg)) {
    Reg = TRI->getSubReg(Reg, Idx);
    Idx = 0;
  }
  return MIB.addReg(Reg, Flags, Idx);
}

void Z80InstrInfo::storeRegToStackSlot(MachineBasicBlock &MBB,
                                       MachineBasicBlock::iterator MI,
                                       unsigned SrcReg, bool IsKill, int FI,
                                       const TargetRegisterClass *TRC,
                                       const TargetRegisterInfo *TRI) const {
  unsigned Opc;
  switch (TRI->getSpillSize(*TRC)) {
  default:
    llvm_unreachable("Unexpected regclass size");
  case 1:
    Opc = Z80::LD8or;
    break;
  case 2:
    Opc = Subtarget.has16BitEZ80Ops() ? Z80::LD16or : Z80::LD88or;
    break;
  case 3:
    assert(Subtarget.is24Bit() && "Only 24-bit should have 3 byte stack slots");
    Opc = Z80::LD24or;
    break;
  }
  BuildMI(MBB, MI, MBB.findDebugLoc(MI), get(Opc)).addFrameIndex(FI).addImm(0)
    .addReg(SrcReg, getKillRegState(IsKill));
  return;
  unsigned RC, LoOpc, LoIdx, HiOpc, HiIdx, HiOff;
  bool Split =
    Z80::splitReg(TRI->getSpillSize(*TRC), Z80::LD8or, Z80::LD16or, Z80::LD24or,
                  RC, LoOpc, LoIdx, HiOpc, HiIdx, HiOff,
                  Subtarget.has16BitEZ80Ops());
  MachineInstrBuilder LoMIB =
  addSubReg(BuildMI(MBB, MI, MBB.findDebugLoc(MI), get(LoOpc))
            .addFrameIndex(FI).addImm(0), SrcReg, LoIdx, TRI,
            getKillRegState(IsKill));
  if (Split) {
    MachineInstrBuilder HiMIB = addSubReg(
        BuildMI(MBB, MI, MBB.findDebugLoc(MI), get(HiOpc))
        .addFrameIndex(FI).addImm(HiOff), SrcReg, HiIdx, TRI,
        getKillRegState(IsKill));
    if (IsKill)
      HiMIB->addRegisterKilled(SrcReg, TRI, true);
    //HiMIB->bundleWithPred();
    //finalizeBundle(MBB, MachineBasicBlock::instr_iterator(LoMIB),
    //               std::next(MachineBasicBlock::instr_iterator(HiMIB)));
  }
}

void Z80InstrInfo::loadRegFromStackSlot(MachineBasicBlock &MBB,
                                        MachineBasicBlock::iterator MI,
                                        unsigned DstReg, int FI,
                                        const TargetRegisterClass *TRC,
                                        const TargetRegisterInfo *TRI) const {
  unsigned Opc;
  switch (TRI->getSpillSize(*TRC)) {
  default:
    llvm_unreachable("Unexpected regclass size");
  case 1:
    Opc = Z80::LD8ro;
    break;
  case 2:
    Opc = Subtarget.has16BitEZ80Ops() ? Z80::LD16ro : Z80::LD88ro;
    break;
  case 3:
    assert(Subtarget.is24Bit() && "Only 24-bit should have 3 byte stack slots");
    Opc = Z80::LD24ro;
    break;
  }
  BuildMI(MBB, MI, MBB.findDebugLoc(MI), get(Opc), DstReg).addFrameIndex(FI)
    .addImm(0);
  return;
  unsigned RC, LoOpc, LoIdx, HiOpc, HiIdx, HiOff;
  bool Split =
    Z80::splitReg(TRI->getSpillSize(*TRC), Z80::LD8ro, Z80::LD16ro, Z80::LD24ro,
                  RC, LoOpc, LoIdx, HiOpc, HiIdx, HiOff,
                  Subtarget.hasEZ80Ops());
  MachineInstrBuilder LoMIB =
  addSubReg(BuildMI(MBB, MI, MBB.findDebugLoc(MI), get(LoOpc)), DstReg, LoIdx,
            TRI, RegState::DefineNoRead).addFrameIndex(FI).addImm(0);
  if (Split) {
    MachineInstrBuilder HiMIB = addSubReg(
        BuildMI(MBB, MI, MBB.findDebugLoc(MI), get(HiOpc)), DstReg, HiIdx,
        TRI, RegState::Define).addFrameIndex(FI).addImm(HiOff);
    HiMIB->addRegisterDefined(DstReg, TRI);
    //HiMIB->bundleWithPred();
    //finalizeBundle(MBB, MachineBasicBlock::instr_iterator(LoMIB),
    //               std::next(MachineBasicBlock::instr_iterator(HiMIB)));
  }
}

/// Return true and the FrameIndex if the specified
/// operand and follow operands form a reference to the stack frame.
bool Z80InstrInfo::isFrameOperand(const MachineInstr &MI, unsigned int Op,
                                  int &FrameIndex) const {
  if (MI.getOperand(Op).isFI() &&
      MI.getOperand(Op + 1).isImm() && MI.getOperand(Op + 1).getImm() == 0) {
    FrameIndex = MI.getOperand(Op).getIndex();
    return true;
  }
  return false;
}

static bool isFrameLoadOpcode(int Opcode) {
  switch (Opcode) {
  default:
    return false;
  case Z80::LD8ro:
  case Z80::LD16ro:
  case Z80::LD88ro:
  case Z80::LD24ro:
    return true;
  }
}
unsigned Z80InstrInfo::isLoadFromStackSlot(const MachineInstr &MI,
                                           int &FrameIndex) const {
  if (isFrameLoadOpcode(MI.getOpcode()) && !MI.getOperand(0).getSubReg() &&
      isFrameOperand(MI, 1, FrameIndex))
    return MI.getOperand(0).getReg();
  return 0;
}

static bool isFrameStoreOpcode(int Opcode) {
  switch (Opcode) {
  default:
    return false;
  case Z80::LD8or:
  case Z80::LD16or:
  case Z80::LD88or:
  case Z80::LD24or:
    return true;
  }
}
unsigned Z80InstrInfo::isStoreToStackSlot(const MachineInstr &MI,
                                          int &FrameIndex) const {
  if (isFrameStoreOpcode(MI.getOpcode()) && !MI.getOperand(2).getSubReg() &&
      isFrameOperand(MI, 0, FrameIndex))
    return MI.getOperand(2).getReg();
  return 0;
}

bool Z80InstrInfo::isReallyTriviallyReMaterializable(const MachineInstr &MI,
                                                     AliasAnalysis *AA) const {
  switch (MI.getOpcode()) {
  case Z80::LD8r0:
  case Z80::LD24r0:
  case Z80::LD24r_1:
    return true;
  }
  return false;
}

void Z80InstrInfo::reMaterialize(MachineBasicBlock &MBB,
                                 MachineBasicBlock::iterator I,
                                 unsigned DstReg, unsigned SubIdx,
                                 const MachineInstr &Orig,
                                 const TargetRegisterInfo &TRI) const {
  if (!Orig.modifiesRegister(Z80::F, &TRI) ||
      MBB.computeRegisterLiveness(&TRI, Z80::F, I) ==
      MachineBasicBlock::LQR_Dead)
    return TargetInstrInfo::reMaterialize(MBB, I, DstReg, SubIdx, Orig, TRI);
  // The instruction clobbers F. Re-materialize as LDri to avoid side effects.
  unsigned Opc;
  int Val;
  switch (Orig.getOpcode()) {
  default: llvm_unreachable("Unexpected instruction!");
  case Z80::LD8r0:   Opc = Z80::LD8ri;  Val =  0; break;
  case Z80::LD24r0:  Opc = Z80::LD24ri; Val =  0; break;
  case Z80::LD24r_1: Opc = Z80::LD24ri; Val = -1; break;
  }
  BuildMI(MBB, I, Orig.getDebugLoc(), get(Opc))
    .addReg(DstReg, RegState::Define, SubIdx).addImm(Val);
}

void Z80InstrInfo::
expandLoadStoreWord(const TargetRegisterClass *ARC, unsigned AOpc,
                    const TargetRegisterClass *ORC, unsigned OOpc,
                    MachineInstr &MI, unsigned RegIdx) const {
  unsigned Reg = MI.getOperand(RegIdx).getReg();
  assert(ARC->contains(Reg) != ORC->contains(Reg) &&
         "RegClasses should be covering and disjoint");
  MI.setDesc(get(ARC->contains(Reg) ? AOpc : OOpc));
  (void)ORC;
}

bool Z80InstrInfo::expandPostRAPseudo(MachineInstr &MI) const {
  DebugLoc DL = MI.getDebugLoc();
  MachineBasicBlock &MBB = *MI.getParent();
  MachineFunction &MF = *MBB.getParent();
  auto Next = ++MachineBasicBlock::iterator(MI);
  MachineInstrBuilder MIB(MF, MI);
  const TargetRegisterInfo &TRI = getRegisterInfo();
  bool Is24Bit = Subtarget.is24Bit();
  bool UseLEA = Is24Bit && !MF.getFunction()->getAttributes()
    .hasAttribute(AttributeList::FunctionIndex, Attribute::OptimizeForSize);
  DEBUG(dbgs() << "\nZ80InstrInfo::expandPostRAPseudo:"; MI.dump());
  switch (unsigned Opc = MI.getOpcode()) {
  default:
    return false;
  case Z80::RCF:
    MI.setDesc(get(Z80::OR8ar));
    MIB.addReg(Z80::A, RegState::Undef);
    break;
  case Z80::LD8r0:
    if (MI.getOperand(0).getReg() == Z80::A) {
      MI.setDesc(get(Z80::XOR8ar));
      MI.getOperand(0).setIsUse();
      MI.getOperand(0).setIsUndef();
      MIB.addReg(Z80::A, RegState::ImplicitDefine);
    } else {
      MI.setDesc(get(Z80::LD8ri));
      MI.findRegisterDefOperand(Z80::F)->ChangeToImmediate(0);
    }
    break;
  case Z80::LD24r0:
  case Z80::LD24r_1:
    if (MI.getOperand(0).getReg() == Z80::UHL) {
      if (Opc == Z80::LD24r0)
        expandPostRAPseudo(*BuildMI(MBB, MI, DL, get(Z80::RCF)));
      else
        BuildMI(MBB, MI, DL, get(Z80::SCF));
      MI.setDesc(get(Z80::SBC24aa));
      MI.getOperand(0).setImplicit();
      MIB.addReg(Z80::UHL, RegState::Implicit | RegState::Undef);
      MIB.addReg(Z80::F, RegState::Implicit);
    } else {
      MI.setDesc(get(Z80::LD24ri));
      MI.findRegisterDefOperand(Z80::F)
        ->ChangeToImmediate(Opc == Z80::LD24r0 ? 0 : -1);
    }
    break;
  case Z80::CP16ao:
  case Z80::CP24ao: {
    unsigned Reg = Opc == Z80::CP24ao ? Z80::UHL : Z80::HL;
    if (MBB.computeRegisterLiveness(&TRI, Reg, Next) !=
        MachineBasicBlock::LQR_Dead) {
      BuildMI(MBB, Next, DL, get(Opc == Z80::CP24ao ? Z80::ADD24ao
                                 : Z80::ADD16ao), Reg)
        .addReg(Reg).add(MI.getOperand(0));
      MI.getOperand(0).setIsKill(false);
    }
    LLVM_FALLTHROUGH;
  }
  case Z80::SUB16ao:
  case Z80::SUB24ao:
    expandPostRAPseudo(*BuildMI(MBB, MI, DL, get(Z80::RCF)));
    MI.setDesc(get(Opc == Z80::CP24ao || Opc == Z80::SUB24ao ? Z80::SBC24ao
                                                             : Z80::SBC16ao));
    break;
  case Z80::CP16a0:
  case Z80::CP24a0: {
    unsigned Reg = Opc == Z80::CP24a0 ? Z80::UHL : Z80::HL;
    unsigned UndefReg = Opc == Z80::CP24a0 ? Z80::UBC : Z80::BC;
    BuildMI(MBB, MI, DL, get(Opc == Z80::CP24a0 ? Z80::ADD24ao : Z80::ADD16ao),
            Reg).addReg(Reg).addReg(UndefReg, RegState::Undef);
    expandPostRAPseudo(*BuildMI(MBB, MI, DL, get(Z80::RCF)));
    MI.setDesc(get(Opc == Z80::CP24a0 ? Z80::SBC24ao : Z80::SBC16ao));
    MIB.addReg(UndefReg, RegState::Undef);
    break;
  }
  case Z80::LD8ro:
  case Z80::LD8rp: {
    MachineOperand &DstOp = MI.getOperand(0);
    if (Z80::I8RegClass.contains(DstOp.getReg())) {
      BuildMI(MBB, MI, DL, get(Is24Bit ? Z80::PUSH24AF : Z80::PUSH16AF));
      DstOp.setReg(Z80::A);
      copyPhysReg(MBB, Next, DL, DstOp.getReg(), Z80::A, true);
      BuildMI(MBB, Next, DL, get(Is24Bit ? Z80::POP24AF : Z80::POP16AF));
    }
    MI.setDesc(get(Opc == Z80::LD8ro ? Z80::LD8go : Z80::LD8gp));
    break;
  }
  case Z80::LD88rp:
  case Z80::LD88ro: {
    assert(!Subtarget.has16BitEZ80Ops() &&
           "LD88rp/LD88ro is not used on the ez80 in 16-bit mode");
    unsigned NewOpc = Opc == Z80::LD88rp ? Z80::LD8rp : Z80::LD8ro;
    MachineOperand &DstOp = MI.getOperand(0);
    const MachineOperand &AddrOp = MI.getOperand(1);
    unsigned OrigReg = DstOp.getReg();
    unsigned Reg = OrigReg;
    bool Index = Z80::I16RegClass.contains(Reg);
    bool Overlap = NewOpc == Z80::LD8rp && Reg == Z80::HL;
    unsigned ScratchReg;
    if (Index || Overlap) {
      Reg = Index ? Z80::HL : Z80::DE;
      ScratchReg = Is24Bit ? Index ? Z80::UHL : Z80::UDE : Reg;
      BuildMI(MBB, MI, DL, get(Is24Bit ? Z80::PUSH24r : Z80::PUSH16r))
        .addReg(ScratchReg, RegState::Undef);
    }
    MIB = BuildMI(MBB, MI, DL, get(NewOpc), TRI.getSubReg(Reg, Z80::sub_low))
      .addReg(AddrOp.getReg());
    if (NewOpc == Z80::LD8rp)
      BuildMI(MBB, MI, DL, get(Is24Bit ? Z80::INC24r : Z80::INC16r),
              AddrOp.getReg()).addReg(AddrOp.getReg());
    MI.setDesc(get(NewOpc));
    DstOp.setReg(TRI.getSubReg(Reg, Z80::sub_high));
    if (NewOpc == Z80::LD8ro) {
      MachineOperand &OffOp = MI.getOperand(2);
      MIB.addImm(OffOp.getImm());
      OffOp.setImm(OffOp.getImm() + 1);
    } else if (!AddrOp.isKill())
      BuildMI(MBB, Next, DL, get(Is24Bit ? Z80::DEC24r : Z80::DEC16r),
              AddrOp.getReg()).addReg(AddrOp.getReg());
    if (Index)
      BuildMI(MBB, Next, DL, get(Is24Bit ? Z80::EX24SP : Z80::EX16SP),
              ScratchReg).addReg(ScratchReg);
    else if (Overlap)
      copyPhysReg(MBB, Next, DL, AddrOp.getReg(), ScratchReg, true);
    if (Index || Overlap)
      BuildMI(MBB, Next, DL, get(Is24Bit ? Z80::POP24r : Z80::POP16r),
              Overlap ? ScratchReg : Is24Bit ?
                  TRI.getMatchingSuperReg(OrigReg, Z80::sub_short,
                                          &Z80::R24RegClass) : OrigReg);
    expandPostRAPseudo(*MIB);
    expandPostRAPseudo(MI);
    DEBUG(MI.dump());
    break;
  }
  case Z80::LD8or:
  case Z80::LD8pr: {
    MachineOperand &SrcOp = MI.getOperand(MI.getNumExplicitOperands() - 1);
    if (Z80::I8RegClass.contains(SrcOp.getReg())) {
      BuildMI(MBB, MI, DL, get(Is24Bit ? Z80::PUSH24AF : Z80::PUSH16AF));
      copyPhysReg(MBB, MI, DL, Z80::A, SrcOp.getReg(), SrcOp.isKill());
      SrcOp.setReg(Z80::A);
      SrcOp.setIsKill();
      BuildMI(MBB, Next, DL, get(Is24Bit ? Z80::POP24AF : Z80::POP16AF));
    }
    MI.setDesc(get(Opc == Z80::LD8or ? Z80::LD8og : Z80::LD8pg));
    break;
  }
  case Z80::LD88pr:
  case Z80::LD88or: {
    assert(!Subtarget.has16BitEZ80Ops() &&
           "LD88pr/LD88or is not used on the ez80 in 16-bit mode");
    unsigned NewOpc = Opc == Z80::LD88pr ? Z80::LD8pr : Z80::LD8or;
    const MachineOperand &AddrOp = MI.getOperand(0);
    MachineOperand &SrcOp = MI.getOperand(MI.getNumExplicitOperands() - 1);
    unsigned Reg = SrcOp.getReg();
    bool Index = Z80::I16RegClass.contains(Reg);
    bool Overlap = NewOpc == Z80::LD8pr && Reg == Z80::HL;
    unsigned ScratchReg = Is24Bit ? Z80::UHL : Z80::HL;
    if (Index) {
      unsigned SuperReg = TRI.getMatchingSuperReg(Reg, Z80::sub_short,
                                                  &Z80::R24RegClass);
      BuildMI(MBB, MI, DL, get(Is24Bit ? Z80::PUSH24r : Z80::PUSH16r))
        .addReg(UseLEA || Overlap ? ScratchReg : Is24Bit ? SuperReg : Reg,
                RegState::Undef);
      if (UseLEA)
        BuildMI(MBB, MI, DL, get(Z80::LEA24ro), Z80::UHL)
          .addReg(SuperReg).addImm(0);
      else
        BuildMI(MBB, MI, DL, get(Is24Bit ? Z80::EX24SP : Z80::EX16SP),
                ScratchReg).addReg(ScratchReg);
      Reg = Z80::HL;
    } else if (Overlap) {
      BuildMI(MBB, MI, DL, get(Is24Bit ? Z80::PUSH24AF : Z80::PUSH16AF));
      Reg = Z80::HL;
    }
    MIB = BuildMI(MBB, MI, DL, get(NewOpc)).addReg(AddrOp.getReg());
    if (NewOpc == Z80::LD8or) {
      MachineOperand &OffOp = MI.getOperand(1);
      MIB.addImm(OffOp.getImm());
      OffOp.setImm(OffOp.getImm() + 1);
    } else if (!AddrOp.isKill())
      BuildMI(MBB, Next, DL, get(Is24Bit ? Z80::DEC24r : Z80::DEC16r),
              AddrOp.getReg()).addReg(AddrOp.getReg());
    MIB.addReg(TRI.getSubReg(Reg, Z80::sub_low));
    Reg = TRI.getSubReg(Reg, Z80::sub_high);
    if (Overlap) {
      copyPhysReg(MBB, MI, DL, Z80::A, Reg, false);
      Reg = Z80::A;
    }
    if (NewOpc == Z80::LD8pr)
      BuildMI(MBB, MI, DL, get(Is24Bit ? Z80::INC24r : Z80::INC16r),
              AddrOp.getReg()).addReg(AddrOp.getReg());
    MI.setDesc(get(NewOpc));
    SrcOp.setReg(Reg);
    if (Index)
      BuildMI(MBB, Next, DL, get(Is24Bit ? Z80::POP24r : Z80::POP16r),
              ScratchReg);
    else if (Overlap)
      BuildMI(MBB, Next, DL, get(Is24Bit ? Z80::POP24AF : Z80::POP16AF));
    expandPostRAPseudo(*MIB);
    expandPostRAPseudo(MI);
    DEBUG(MI.dump());
    break;
  }
  case Z80::LD16rm:
    expandLoadStoreWord(&Z80::A16RegClass, Z80::LD16am,
                        &Z80::O16RegClass, Z80::LD16om, MI, 0);
    break;
  case Z80::LD24rm:
    expandLoadStoreWord(&Z80::A24RegClass, Z80::LD24am,
                        &Z80::O24RegClass, Z80::LD24om, MI, 0);
    break;
  case Z80::LD16mr:
    expandLoadStoreWord(&Z80::A16RegClass, Z80::LD16ma,
                        &Z80::O16RegClass, Z80::LD16mo, MI, 1);
    break;
  case Z80::LD24mr:
    expandLoadStoreWord(&Z80::A24RegClass, Z80::LD24ma,
                        &Z80::O24RegClass, Z80::LD24mo, MI, 1);
    break;
  case Z80::CALL16r:
  case Z80::CALL24r: {
    const char *Symbol;
    switch (MIB->getOperand(0).getReg()) {
    default: llvm_unreachable("Unexpected indcall register");
    case Z80::HL: case Z80::UHL: Symbol = "_indcallhl"; break;
    case Z80::IX: case Z80::UIX: Symbol = "_indcallix"; break;
    case Z80::IY: case Z80::UIY: Symbol = "_indcall"; break;
    }
    MI.setDesc(get(Opc == Z80::CALL24r ? Z80::CALL24i : Z80::CALL16i));
    MI.getOperand(0).ChangeToES(Symbol);
    break;
  }
  case Z80::EI_RETI:
    BuildMI(MBB, MI, DL, get(Z80::EI));
    MI.setDesc(get(Z80::RETI));
    break;
  case Z80::TCRETURN16i:
    MI.setDesc(get(Z80::JP16));
    break;
  case Z80::TCRETURN24i:
    MI.setDesc(get(Z80::JP24));
    break;
  case Z80::TCRETURN16r:
    MI.setDesc(get(Z80::JP16r));
    break;
  case Z80::TCRETURN24r:
    MI.setDesc(get(Z80::JP24r));
    break;
  }
  DEBUG(MIB->dump());
  return true;
}

bool Z80InstrInfo::analyzeCompare(const MachineInstr &MI,
                                  unsigned &SrcReg, unsigned &SrcReg2,
                                  int &CmpMask, int &CmpValue) const {
  switch (MI.getOpcode()) {
  default: return false;
  case Z80::OR8ar:
    SrcReg = Z80::A;
    if (MI.getOperand(1).getReg() != SrcReg)
      return false;
    // Compare against zero.
    SrcReg2 = 0;
    CmpMask = ~0;
    CmpValue = 0;
    break;
  case Z80::CP8ai:
  case Z80::SUB8ai:
    SrcReg = Z80::A;
    SrcReg2 = 0;
    CmpMask = CmpValue = 0;
    if (MI.getOperand(0).isImm()) {
      CmpMask = ~0;
      CmpValue = MI.getOperand(0).getImm();
    }
    break;
  case Z80::CP8ar:
  case Z80::SUB8ar:
    SrcReg = Z80::A;
    SrcReg2 = MI.getOperand(0).getReg();
    CmpMask = CmpValue = 0;
    break;
  case Z80::CP8ap:
  case Z80::CP8ao:
  case Z80::SUB8ap:
  case Z80::SUB8ao:
    SrcReg = Z80::A;
    SrcReg2 = CmpMask = CmpValue = 0;
    break;
  }
  MachineBasicBlock::const_reverse_iterator I = MI, E = MI.getParent()->rend();
  while (++I != E && I->isFullCopy())
    for (unsigned *Reg : {&SrcReg, &SrcReg2})
      if (TargetRegisterInfo::isPhysicalRegister(*Reg) &&
          *Reg == I->getOperand(0).getReg())
        *Reg = I->getOperand(1).getReg();
  return true;
}

/// Check whether the first instruction, whose only purpose is to update flags,
/// can be made redundant. CP8ar is made redundant by SUB8ar if the operands are
/// the same.
/// SrcReg, SrcReg2: register operands for FlagI.
/// ImmValue: immediate for FlagI if it takes an immediate.
inline static bool isRedundantFlagInstr(MachineInstr &FI, unsigned SrcReg,
                                        unsigned SrcReg2, int ImmMask,
                                        int ImmValue, MachineInstr &OI) {
  if (ImmMask)
    return (FI.getOpcode() == Z80::CP8ai && OI.getOpcode() == Z80::SUB8ai) &&
      OI.getOperand(1).getImm() == ImmValue;
  else
    return (FI.getOpcode() == Z80::CP8ar && OI.getOpcode() == Z80::SUB8ar) &&
      OI.getOperand(1).getReg() == SrcReg2;
}

/// Check whether the instruction sets the sign and zero flag based on its
/// result.
inline static bool isSZSettingInstr(MachineInstr &MI) {
  switch (MI.getOpcode()) {
  default: return false;
  case Z80::INC8r:  case Z80::INC8p:  case Z80::INC8o:
  case Z80::DEC8r:  case Z80::DEC8p:  case Z80::DEC8o:
  case Z80::ADD8ar: case Z80::ADD8ai: case Z80::ADD8ap: case Z80::ADD8ao:
  case Z80::ADC8ar: case Z80::ADC8ai: case Z80::ADC8ap: case Z80::ADC8ao:
  case Z80::SUB8ar: case Z80::SUB8ai: case Z80::SUB8ap: case Z80::SUB8ao:
  case Z80::SBC8ar: case Z80::SBC8ai: case Z80::SBC8ap: case Z80::SBC8ao:
  case Z80::AND8ar: case Z80::AND8ai: case Z80::AND8ap: case Z80::AND8ao:
  case Z80::XOR8ar: case Z80::XOR8ai: case Z80::XOR8ap: case Z80::XOR8ao:
  case Z80:: OR8ar: case Z80:: OR8ai: case Z80:: OR8ap: case Z80:: OR8ao:
  case Z80::SBC16ao:case Z80::NEG:    case Z80::ADC16ao:
  case Z80::SUB16ao:case Z80::SUB24ao:
  case Z80::RLC8r:  case Z80::RLC8p:  case Z80::RLC8o:
  case Z80::RRC8r:  case Z80::RRC8p:  case Z80::RRC8o:
  case Z80:: RL8r:  case Z80:: RL8p:  case Z80:: RL8o:
  case Z80:: RR8r:  case Z80:: RR8p:  case Z80:: RR8o:
  case Z80::SLA8r:  case Z80::SLA8p:  case Z80::SLA8o:
  case Z80::SRA8r:  case Z80::SRA8p:  case Z80::SRA8o:
  case Z80::SRL8r:  case Z80::SRL8p:  case Z80::SRL8o:
    return true;
  }
}

/// Check if there exists an earlier instruction that operates on the same
/// source operands and sets flags in the same way as Compare; remove Compare if
/// possible.
bool Z80InstrInfo::optimizeCompareInstr(MachineInstr &CmpInstr,
                                        unsigned SrcReg, unsigned SrcReg2,
                                        int CmpMask, int CmpValue,
                                        const MachineRegisterInfo *MRI) const {
  // If we are comparing against zero, check whether we can use MI to update F.
  bool IsCmpZero = CmpMask && !CmpValue;

  // Check whether we can replace SUB with CP.
  unsigned CpOp;
  switch (CmpInstr.getOpcode()) {
  default: CpOp = 0; break;
  case Z80::SUB8ai: CpOp = IsCmpZero ? Z80::OR8ar : Z80::CP8ai; break;
  case Z80::SUB8ar: CpOp = Z80::CP8ar; break;
  case Z80::SUB8ap: CpOp = Z80::CP8ap; break;
  case Z80::SUB8ao: CpOp = Z80::CP8ao; break;
  }
  if (CpOp) {
    int DeadDef = CmpInstr.findRegisterDefOperandIdx(Z80::A, /*isDead*/true);
    if (DeadDef == -1)
      return false;
    // There is no use of the destination register, so we replace SUB with CP.
    CmpInstr.setDesc(get(CpOp));
    if (CpOp == Z80::OR8ar)
      CmpInstr.getOperand(0).ChangeToRegister(Z80::A, false);
    else
      CmpInstr.RemoveOperand(DeadDef);
  }

  // Get the unique definition of SrcReg.
  MachineInstr *MI = MRI->getUniqueVRegDef(SrcReg);
  if (!MI) return false;

  MachineBasicBlock::iterator I = CmpInstr, Def = MI;

  const TargetRegisterInfo *TRI = &getRegisterInfo();
  for (auto RI = ++Def.getReverse(), RE = MI->getParent()->rend();
       MI->isFullCopy() && RI != RE; ++RI)
    if (RI->definesRegister(MI->getOperand(1).getReg(), TRI))
      MI = &*RI;

  // If MI is not in the same BB as CmpInstr, do not optimize.
  if (IsCmpZero && (MI->getParent() != CmpInstr.getParent() ||
                    !isSZSettingInstr(*MI)))
    return false;

  // We are searching for an earlier instruction, which will be stored in
  // SubInstr, that can make CmpInstr redundant.
  MachineInstr *SubInstr = nullptr;

  // We iterate backwards, starting from the instruction before CmpInstr, and
  // stopping when we reach the definition of a source register or the end of
  // the BB. RI points to the instruction before CmpInstr. If the definition is
  // in this BB, RE points to it, otherwise RE is the beginning of the BB.
  MachineBasicBlock::reverse_iterator RE = CmpInstr.getParent()->rend();
  if (CmpInstr.getParent() == MI->getParent())
    RE = Def.getReverse(); // points to MI
  for (auto RI = ++I.getReverse(); RI != RE; ++RI) {
    MachineInstr &Instr = *RI;
    // Check whether CmpInstr can be made redundant by the current instruction.
    if (!IsCmpZero && isRedundantFlagInstr(CmpInstr, SrcReg, SrcReg2, CmpMask,
                                           CmpValue, Instr)) {
      SubInstr = &Instr;
      break;
    }

    // If this instruction modifies F, we can't remove CmpInstr.
    if (Instr.modifiesRegister(Z80::F, TRI))
      return false;
  }

  // Return false if no candidates exist.
  if (!IsCmpZero && !SubInstr)
    return false;

  // Scan forward from the instruction after CmpInstr for uses of F.
  // It is safe to remove CmpInstr if F is redefined or killed.
  // If we are at the end of the BB, we need to check whether F is live-out.
  bool IsSafe = false;
  MachineBasicBlock::iterator E = CmpInstr.getParent()->end();
  for (++I; I != E; ++I) {
    const MachineInstr &Instr = *I;
    bool ModifiesFlags = Instr.modifiesRegister(Z80::F, TRI);
    bool UsesFlags = Instr.readsRegister(Z80::F, TRI);
    if (ModifiesFlags && !UsesFlags) {
      IsSafe = true;
      break;
    }
    if (!ModifiesFlags && !UsesFlags)
      continue;
    if (IsCmpZero) {
      Z80::CondCode OldCC;
      switch (Instr.getOpcode()) {
      default:
        OldCC = Z80::COND_INVALID;
        break;
      case Z80::JQCC:
        OldCC = static_cast<Z80::CondCode>(Instr.getOperand(1).getImm());
        break;
      case Z80::ADC8ar: case Z80::ADC8ai: case Z80::ADC8ap: case Z80::ADC8ao:
      case Z80::SBC8ar: case Z80::SBC8ai: case Z80::SBC8ap: case Z80::SBC8ao:
      case Z80::SBC16ao:case Z80::ADC16ao:
        OldCC = Z80::COND_C;
        break;
      }
      switch (OldCC) {
      default: break;
      case Z80::COND_NC: case Z80::COND_C:
      case Z80::COND_PO: case Z80::COND_PE:
        // CF or PV are used, we can't perform this optimization.
        return false;
      }
    }
    if (ModifiesFlags || Instr.killsRegister(Z80::F, TRI)) {
      // It is safe to remove CmpInstr if F is updated again or killed.
      IsSafe = true;
      break;
    }
  }
  if (IsCmpZero && !IsSafe) {
    MachineBasicBlock *MBB = CmpInstr.getParent();
    for (MachineBasicBlock *Successor : MBB->successors())
      if (Successor->isLiveIn(Z80::F))
        return false;
  }

  // The instruction to be updated is either Sub or MI.
  if (IsCmpZero)
    SubInstr = MI;

  // Make sure Sub instruction defines F and mark the def live.
  unsigned i = 0, e = SubInstr->getNumOperands();
  for (; i != e; ++i) {
    MachineOperand &MO = SubInstr->getOperand(i);
    if (MO.isReg() && MO.isDef() && MO.getReg() == Z80::F) {
      MO.setIsDead(false);
      break;
    }
  }
  assert(i != e && "Unable to locate a def EFLAGS operand");

  CmpInstr.eraseFromParent();
  return true;

  // Check whether we can replace SUB with CMP.
  switch (CmpInstr.getOpcode()) {
  default: return false;
  case Z80::SUB8ai:
    // cp a,0 -> or a,a (a szhc have same behavior)
    // FIXME: This doesn't work if the pv flag is used.
    if (!CmpInstr.getOperand(0).getImm()) {
      CmpInstr.setDesc(get(Z80::OR8ar));
      CmpInstr.getOperand(0).ChangeToRegister(Z80::A, /*isDef=*/false);
      return true;
    }
    LLVM_FALLTHROUGH;
  case Z80::SUB8ar:
  case Z80::SUB8ap:
  case Z80::SUB8ao: {
    if (!CmpInstr.registerDefIsDead(Z80::A))
      return false;
    // There is no use of the destination register, we can replace SUB with CMP.
    unsigned NewOpcode = 0;
    switch (CmpInstr.getOpcode()) {
    default: llvm_unreachable("Unreachable!");
    case Z80::SUB8ai: NewOpcode = Z80::CP8ai; break;
    case Z80::SUB8ar: NewOpcode = Z80::CP8ar; break;
    case Z80::SUB8ap: NewOpcode = Z80::CP8ap; break;
    case Z80::SUB8ao: NewOpcode = Z80::CP8ao; break;
    }
    CmpInstr.setDesc(get(NewOpcode));
    //CmpInstr.findRegisterDefOperand(Z80::A)->setIsDead(false);
    //BuildMI(*CmpInstr.getParent(), ++MachineBasicBlock::iterator(CmpInstr), CmpInstr.getDebugLoc(), get(TargetOpcode::COPY), SrcReg).addReg(Z80::A, RegState::Kill);
    return true;
  }
  }
}

MachineInstr *
Z80InstrInfo::foldMemoryOperandImpl(MachineFunction &MF, MachineInstr &MI,
                                    ArrayRef<unsigned> Ops,
                                    MachineBasicBlock::iterator InsertPt,
                                    int FrameIndex, LiveIntervals *LIS) const {
  return nullptr;
  bool Is24Bit = Subtarget.is24Bit();
  MachineBasicBlock &MBB = *InsertPt->getParent();
  if (Ops.size() == 1 && Ops[0] == 1 && MI.isFullCopy()) {
    unsigned DstReg = MI.getOperand(0).getReg();
    if (TargetRegisterInfo::isPhysicalRegister(DstReg)) {
      unsigned Opc;
      if (Z80::R8RegClass.contains(DstReg)) {
        Opc = Z80::LD8ro;
      } else {
        assert((Is24Bit ? Z80::R24RegClass : Z80::R16RegClass)
               .contains(DstReg) && "Unexpected physical reg");
        Opc = Is24Bit ? Z80::LD24ro : Z80::LD16ro;
      }
      return BuildMI(MBB, InsertPt, MI.getDebugLoc(), get(Opc), DstReg)
        .addFrameIndex(FrameIndex).addImm(0);
    }
  }
  dbgs() << Ops.size() << ": ";
  for (unsigned Op : Ops)
    dbgs() << Op << ' ';
  MI.dump();
  return nullptr;
}
MachineInstr *
Z80InstrInfo::foldMemoryOperandImpl(MachineFunction &MF, MachineInstr &MI,
                                    ArrayRef<unsigned> Ops,
                                    MachineBasicBlock::iterator InsertPt,
                                    MachineInstr &LoadMI,
                                    LiveIntervals *LIS) const {
  return nullptr;
  llvm_unreachable("Unimplemented");
}
