//===-- Z80TargetInfo.cpp - Z80 Target Implementation ---------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "MCTargetDesc/Z80MCTargetDesc.h"
#include "llvm/Support/TargetRegistry.h"
using namespace llvm;

Target &llvm::getTheZ80Target() {
  static Target TheZ80Target;
  return TheZ80Target;
}
Target &llvm::getTheEZ80Target() {
  static Target TheEZ80Target;
  return TheEZ80Target;
}

extern "C" void LLVMInitializeZ80TargetInfo() {
  RegisterTarget<Triple::z80> X(getTheZ80Target(), "z80",
                                "Z80 [experimental]", "Z80");
  RegisterTarget<Triple::ez80> Y(getTheEZ80Target(), "ez80",
                                 "eZ80 [experimental]", "Z80");
}

