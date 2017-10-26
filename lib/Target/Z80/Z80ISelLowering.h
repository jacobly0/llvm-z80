//===-- Z80ISelLowering.h - Z80 DAG Lowering Interface ----------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file defines the interfaces that Z80 uses to lower LLVM code into a
// selection DAG.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_Z80_Z80ISELLOWERING_H
#define LLVM_LIB_TARGET_Z80_Z80ISELLOWERING_H

#include "llvm/CodeGen/CallingConvLower.h"
#include "llvm/CodeGen/TargetLowering.h"

namespace llvm {
class Z80Subtarget;
class Z80TargetMachine;

namespace Z80ISD {
// Z80 Specific DAG Nodes
enum NodeType : unsigned {
  // Start the numbering where the builtin ops leave off.
  FIRST_NUMBER = ISD::BUILTIN_OP_END,

  /// A wrapper node for TargetConstantPool, TargetExternalSymbol, and
  /// TargetGlobalAddress.
  Wrapper,

  /// Shift/Rotate
  RLC, RRC, RL, RR, SLA, SRA, SRL,

  /// Arithmetic operation with flags results.
  INC, DEC, ADD, ADC, SUB, SBC, AND, XOR, OR,

  /// Z80 compare and test
  CP, TST,

  MLT,

  /// This produces an all zeros/ones value from an input carry (SBC r,r).
  SEXT,

  /// This operation represents an abstract Z80 call instruction, which
  /// includes a bunch of information.
  CALL,

  /// Return with a flag operand. Operand 0 is the chain operand, operand
  /// 1 is the number of bytes of stack to pop.
  RET_FLAG,

  /// Return from interrupt.
  RETN_FLAG, RETI_FLAG,

  /// Tail call return.
  TC_RETURN,

  /// BRCOND - Z80 conditional branch.  The first operand is the chain, the
  /// second is the block to branch to if the condition is true, the third is
  /// the condition, and the fourth is the flag operand.
  BRCOND,

  /// SELECT - Z80 select - This selects between a true value and a false
  /// value (ops #1 and #2) based on the condition in op #0 and flag in op #3.
  SELECT,

  /// Stack operations
  POP = ISD::FIRST_TARGET_MEMORY_OPCODE, PUSH
};
} // end Z80ISD namespace

//===----------------------------------------------------------------------===//
//  Z80 Implementation of the TargetLowering interface
class Z80TargetLowering final : public TargetLowering {
  /// Keep a reference to the Z80Subtarget around so that we can
  /// make the right decision when generating code for different targets.
  const Z80Subtarget &Subtarget;

public:
  explicit Z80TargetLowering(const Z80TargetMachine &TM,
                             const Z80Subtarget &STI);

  MVT getScalarShiftAmountTy(const DataLayout &, EVT) const override {
    return MVT::i8;
  }

  // Legalize Types Helpers

  /// Replace the results of node with an illegal result type with new values
  /// built out of custom code.
  void ReplaceNodeResults(SDNode *N, SmallVectorImpl<SDValue> &Results,
                          SelectionDAG &DAG) const override;

  // Legalize Helpers

  SDValue LowerAddSub(SDValue Op, SelectionDAG &DAG) const;
  SDValue LowerBitwise(SDValue Op, SelectionDAG &DAG) const;
  SDValue LowerShift(SDValue Op, SelectionDAG &DAG) const;
  SDValue LowerAnyExtend(SDValue Op, SelectionDAG &DAG) const;
  SDValue LowerZeroExtend(SDValue Op, SelectionDAG &DAG) const;
  SDValue LowerSignExtend(SDValue Op, SelectionDAG &DAG) const;
  SDValue LowerMul(SDValue Op, SelectionDAG &DAG) const;
  SDValue LowerLoad(LoadSDNode *Node, SelectionDAG &DAG) const;
  SDValue LowerStore(StoreSDNode *Node, SelectionDAG &DAG) const;
  SDValue LowerVAStart(SDValue Op, SelectionDAG &DAG) const;
  SDValue LowerOperation(SDValue Op, SelectionDAG &DAG) const override;

  /// ---------------------------------------------------------------------- ///

  bool useSoftFloat() const override { return true; }
  bool isSelectSupported(SelectSupportKind /*Kind*/) const override {
    return false;
  }
  bool canOpTrap(unsigned Op, EVT VT) const override { return false; }

  /// This method returs the name of a target specific DAG node.
  const char *getTargetNodeName(unsigned Opcode) const override;

  /// Return the value type to use for ISD::SETCC.
  EVT getSetCCResultType(const DataLayout &DL, LLVMContext &Context,
                         EVT VT) const override;
  MVT::SimpleValueType getCmpLibcallReturnType() const override;

  /// Provide custom lowering hooks for some operations.
  SDValue LowerLibCall(RTLIB::Libcall LC8, RTLIB::Libcall LC16,
                       RTLIB::Libcall LC24, RTLIB::Libcall LC32,
                       SDValue Op, SelectionDAG &DAG) const;
  SDValue NarrowOperation(SDValue Op, SelectionDAG &DAG) const;

  bool isOffsetFoldingLegal(const GlobalAddressSDNode *GA) const override;

  /// Return true if the addressing mode represented by AM is legal for this
  /// target, for a load/store of the specified type.
  bool isLegalAddressingMode(const DataLayout &DL, const AddrMode &AM,
                             Type *Ty, unsigned AS,
                             Instruction *I = nullptr) const override;

  /// Return true if the specified immediate is a legal icmp immediate, that is
  /// the target has icmp instructions which can compare a register against the
  /// immediate without having to materialize the immediate into a register.
  bool isLegalICmpImmediate(int64_t Imm) const override;

  /// Return true if the specified immediate is a legal add immediate, that is
  /// the target has add instructions which can add a register and the immediate
  /// without having to materialize the immediate into a register.
  bool isLegalAddImmediate(int64_t Imm) const override;

  /// Return true if it's free to truncate a value of
  /// type Ty1 to type Ty2. e.g. On z80 it's free to truncate an i16 value in
  /// register HL to i8 by referencing its sub-register L.
  bool isTruncateFree(Type *Ty1, Type *Ty2) const override;
  bool isTruncateFree(EVT VT1, EVT VT2) const override;

  /// Return true if any actual instruction that defines a
  /// value of type Ty1 implicit zero-extends the value to Ty2 in the result
  /// register. This does not necessarily include registers defined in
  /// unknown ways, such as incoming arguments, or copies from unknown
  /// virtual registers. Also, if isTruncateFree(Ty2, Ty1) is true, this
  /// does not necessarily apply to truncate instructions. e.g. on ez80,
  /// all instructions that define 16-bit values implicit zero-extend the
  /// result out to 24 bits.
  bool isZExtFree(Type *Ty1, Type *Ty2) const override;
  bool isZExtFree(EVT VT1, EVT VT2) const override;

  /// Return true if it's profitable to narrow operations of type VT1 to
  /// VT2. e.g. on ez80, it's profitable to narrow from i24 to i8 but not from
  /// i24 to i16.
  bool isNarrowingProfitable(EVT VT1, EVT VT2) const override;

  /// \brief Returns true if it is beneficial to convert a load of a constant
  /// to just the constant itself.
  bool shouldConvertConstantLoadToIntImm(const APInt &Imm,
                                         Type *Ty) const override;

  /// Replace the results of node with an illegal result type with new values
  /// built out of custom code.
  void ReplaceNodeResultsOld(SDNode *N, SmallVectorImpl<SDValue> &Results,
                          SelectionDAG &DAG) const;

  SDValue PerformDAGCombine(SDNode *N, DAGCombinerInfo &DCI) const override;

  /// Return true if the target has native support for
  /// the specified value type and it is 'desirable' to use the type for the
  /// given node type. e.g. On ez80 i16 is legal, but undesirable since i16
  /// instruction encodings are longer and slower.
  bool isTypeDesirableForOp(unsigned Opc, EVT VT) const override;

  /// Return true if x op y -> (SrcVT)((DstVT)x op (DstVT)y) is beneficial.
  bool isDesirableToShrinkOp(unsigned Opc, EVT SrcVT, EVT DstVT) const override;

  bool IsDesirableToPromoteOp(SDValue Op, EVT &PVT) const override;

  MachineBasicBlock *
    EmitInstrWithCustomInserter(MachineInstr &MI,
                                MachineBasicBlock *BB) const override;

  void AdjustInstrPostInstrSelection(MachineInstr &MI,
                                     SDNode *Node) const override;

 private:
  // SelectionDAG helpers
  SDValue EmitOffset(int64_t Amount, const SDLoc &DL, SDValue Op,
                     SelectionDAG &DAG) const;
  SDValue EmitNegate(const SDLoc &DL, SDValue Op, SelectionDAG &DAG) const;
  SDValue EmitFlipSign(const SDLoc &DL, SDValue Op, SelectionDAG &DAG) const;
  SDValue EmitLow(SDValue Op, SelectionDAG &DAG) const;
  SDValue EmitHigh(SDValue Op, SelectionDAG &DAG) const;
  SDValue EmitPair(const SDLoc &DL, SDValue Hi, SDValue Lo,
                   SelectionDAG &DAG) const;
  SDValue EmitSignToCarry(SDValue Op, SelectionDAG &DAG) const;
  // Legalize Helpers
  SDValue EmitCmp(SDValue LHS, SDValue RHS, SDValue &TargetCC,
                  ISD::CondCode CC, const SDLoc &DL, SelectionDAG &DAG) const;
  // Old SelectionDAG Helpers
  SDValue EmitExtractSubreg(unsigned Idx, const SDLoc &DL, SDValue Op,
                            SelectionDAG &DAG) const;
  SDValue EmitInsertSubreg(unsigned Idx, const SDLoc &DL, MVT VT, SDValue Op,
                           SelectionDAG &DAG) const;
  SDValue LowerBR_CC(SDValue Op, SelectionDAG &DAG) const;
  SDValue LowerSETCC(SDValue Op, SelectionDAG &DAG) const;
  SDValue LowerSELECT_CC(SDValue Op, SelectionDAG &DAG) const;

  SDValue LowerGlobalAddress(GlobalAddressSDNode *Node,
                             SelectionDAG &DAG) const;
  SDValue LowerExternalSymbol(ExternalSymbolSDNode *Node,
                              SelectionDAG &DAG) const;
  SDValue LowerBlockAddress(BlockAddressSDNode *Node, SelectionDAG &DAG) const;

  CCAssignFn *getCCAssignFn(CallingConv::ID CallConv) const;
  CCAssignFn *getRetCCAssignFn(CallingConv::ID CallConv) const;

  SDValue LowerFormalArguments(SDValue Chain,
                               CallingConv::ID CallConv, bool isVarArg,
                               const SmallVectorImpl<ISD::InputArg> &Ins,
                               const SDLoc &DL, SelectionDAG &DAG,
                               SmallVectorImpl<SDValue> &InVals) const override;
  SDValue LowerCall(CallLoweringInfo &CLI,
                    SmallVectorImpl<SDValue> &InVals) const override;
  SDValue LowerCallResult(SDValue Chain, SDValue InFlag,
                          CallingConv::ID CallConv, bool IsVarArg,
                          const SmallVectorImpl<ISD::InputArg> &Ins,
                          SDLoc DL, SelectionDAG &DAG,
                          SmallVectorImpl<SDValue> &InVals) const;
  SDValue LowerReturn(SDValue Chain,
                      CallingConv::ID CallConv, bool isVarArg,
                      const SmallVectorImpl<ISD::OutputArg> &Outs,
                      const SmallVectorImpl<SDValue> &OutVals,
                      const SDLoc &DL, SelectionDAG &DAG) const override;

  /// Check whether the call is eligible for tail call optimization. Targets
  /// that want to do tail call optimization should implement this function.
  bool IsEligibleForTailCallOptimization(
      SDValue Callee, CallingConv::ID CalleeCC, bool isVarArg, Type *RetTy,
      const SmallVectorImpl<ISD::OutputArg> &Outs,
      const SmallVectorImpl<SDValue> &OutVals,
      const SmallVectorImpl<ISD::InputArg> &Ins, SelectionDAG &DAG) const;

  void AdjustAdjCallStack(MachineInstr &MI) const;
  MachineBasicBlock *EmitLoweredSub0(MachineInstr &MI,
                                     MachineBasicBlock *BB) const;
  MachineBasicBlock *EmitLoweredSub(MachineInstr &MI,
                                    MachineBasicBlock *BB) const;
  MachineBasicBlock *EmitLoweredCmp0(MachineInstr &MI,
                                     MachineBasicBlock *BB) const;
  MachineBasicBlock *EmitLoweredCmp(MachineInstr &MI,
                                    MachineBasicBlock *BB) const;
  MachineBasicBlock *EmitLoweredSelect(MachineInstr &MI,
                                       MachineBasicBlock *BB) const;
  MachineBasicBlock *EmitLoweredSExt(MachineInstr &MI,
                                     MachineBasicBlock *BB) const;

  SDValue combineCopyFromReg(SDNode *N, DAGCombinerInfo &DCI) const;
  SDValue combineStore(StoreSDNode *N, DAGCombinerInfo &DCI) const;
  SDValue combineINSERT_SUBREG(SDNode *N, DAGCombinerInfo &DCI) const;
  SDValue combineADD(SDNode *N, DAGCombinerInfo &DCI) const;
  SDValue combineSUB(SDNode *N, DAGCombinerInfo &DCI) const;
};
} // End llvm namespace

#endif
