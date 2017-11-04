//===-- Z80InstrFormats.td - Z80 Instruction Formats -------*- tablegen -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

//===----------------------------------------------------------------------===//
// Z80 Instruction Format Definitions.
//

class Prefix<bits<4> val> {
  bits<4> Value = val;
}
def NoPre   : Prefix<0>;
def CBPre   : Prefix<1>;
def DDPre   : Prefix<2>;
def DDCBPre : Prefix<3>;
def EDPre   : Prefix<4>;
def FDPre   : Prefix<5>;
def FDCBPre : Prefix<6>;
def IdxPre  : Prefix<7>;
foreach i = 0-7 in
def Idx#i#Pre : Prefix<!add(8, i)>;

class ImmInfo<bit hasOff, bit hasImm> {
  bit HasOff = hasOff;
  bit HasImm = hasImm;
}
def  NoImm : ImmInfo<0, 0>;
def    Imm : ImmInfo<0, 1>;
def Off    : ImmInfo<1, 0>;
def OffImm : ImmInfo<1, 1>;

class Z80Inst<Prefix prefix, bits<8> opcode, ImmInfo imm, bits<2> immSize,
              dag outputs = (outs), dag inputs = (ins), list<dag> pattern = [],
              string asm = "", string constraints = "">
  : Instruction {
  let Namespace = "Z80";

  bits<8> Opcode = opcode;

  let OutOperandList = outputs;
  let InOperandList = inputs;
  let Pattern = pattern;
  let AsmString = asm;
  let Constraints = constraints;

  let TSFlags{3-0} = prefix.Value;
  let TSFlags{4} = imm.HasOff;
  let TSFlags{5} = imm.HasImm;
  let TSFlags{7-6} = immSize;
  let TSFlags{15-8} = opcode;

  let isCodeGenOnly = 1;
}

let isPseudo = 1 in
class Pseudo<string mnemonic, string arguments = "", string constraints = "",
             dag outputs = (outs), dag inputs = (ins), list<dag> pattern = []>
  : Z80Inst<NoPre,       0,     NoImm, 0, outputs, inputs, pattern,
            !strconcat(mnemonic,            arguments), constraints>;

class Inst  <Prefix prefix, bits<8> opcode, ImmInfo immediate,
             string mnemonic, string arguments = "", string constraints = "",
             dag outputs = (outs), dag inputs = (ins), list<dag> pattern = []>
  : Z80Inst<prefix, opcode, immediate, 0, outputs, inputs, pattern,
            !strconcat(mnemonic,            arguments), constraints>;
class Inst8 <Prefix prefix, bits<8> opcode, ImmInfo immediate,
             string mnemonic, string arguments = "", string constraints = "",
             dag outputs = (outs), dag inputs = (ins), list<dag> pattern = []>
  : Z80Inst<prefix, opcode, immediate, 1, outputs, inputs, pattern,
            !strconcat(mnemonic,            arguments), constraints>;
class Inst16<Prefix prefix, bits<8> opcode, ImmInfo immediate,
             string mnemonic, string arguments = "", string constraints = "",
             dag outputs = (outs), dag inputs = (ins), list<dag> pattern = []>
  : Z80Inst<prefix, opcode, immediate, 2, outputs, inputs, pattern,
            !strconcat(mnemonic, "{|.sis}", arguments), constraints>;
class Inst24<Prefix prefix, bits<8> opcode, ImmInfo immediate,
             string mnemonic, string arguments = "", string constraints = "",
             dag outputs = (outs), dag inputs = (ins), list<dag> pattern = []>
  : Z80Inst<prefix, opcode, immediate, 3, outputs, inputs, pattern,
            !strconcat(mnemonic, "{.lil|}", arguments), constraints>,
    Requires<[HaveEZ80Ops]>;

class I    <Prefix prefix, bits<8> opcode,
            string mnemonic, string arguments = "", string constraints = "",
            dag outputs = (outs), dag inputs = (ins), list<dag> pattern = []>
  : Inst  <prefix, opcode,  NoImm, mnemonic, arguments, constraints,
           outputs, inputs, pattern>;
class Ii   <Prefix prefix, bits<8> opcode,
            string mnemonic, string arguments = "", string constraints = "",
            dag outputs = (outs), dag inputs = (ins), list<dag> pattern = []>
  : Inst  <prefix, opcode,    Imm, mnemonic, arguments, constraints,
           outputs, inputs, pattern>;

class I8   <Prefix prefix, bits<8> opcode,
            string mnemonic, string arguments = "", string constraints = "",
            dag outputs = (outs), dag inputs = (ins), list<dag> pattern = []>
  : Inst8 <prefix, opcode,  NoImm, mnemonic, arguments, constraints,
           outputs, inputs, pattern>;
class I8i  <Prefix prefix, bits<8> opcode,
            string mnemonic, string arguments = "", string constraints = "",
            dag outputs = (outs), dag inputs = (ins), list<dag> pattern = []>
  : Inst8 <prefix, opcode,    Imm, mnemonic, arguments, constraints,
           outputs, inputs, pattern>;
class I8o  <Prefix prefix, bits<8> opcode,
            string mnemonic, string arguments = "", string constraints = "",
            dag outputs = (outs), dag inputs = (ins), list<dag> pattern = []>
  : Inst8 <prefix, opcode, Off   , mnemonic, arguments, constraints,
           outputs, inputs, pattern>;
class I8oi <Prefix prefix, bits<8> opcode,
            string mnemonic, string arguments = "", string constraints = "",
            dag outputs = (outs), dag inputs = (ins), list<dag> pattern = []>
  : Inst8 <prefix, opcode, OffImm, mnemonic, arguments, constraints,
           outputs, inputs, pattern>;

class I16  <Prefix prefix, bits<8> opcode,
            string mnemonic, string arguments = "", string constraints = "",
            dag outputs = (outs), dag inputs = (ins), list<dag> pattern = []>
  : Inst16<prefix, opcode,  NoImm, mnemonic, arguments, constraints,
           outputs, inputs, pattern>;
class I16i <Prefix prefix, bits<8> opcode,
            string mnemonic, string arguments = "", string constraints = "",
            dag outputs = (outs), dag inputs = (ins), list<dag> pattern = []>
  : Inst16<prefix, opcode,    Imm, mnemonic, arguments, constraints,
           outputs, inputs, pattern>;
class I16o <Prefix prefix, bits<8> opcode,
            string mnemonic, string arguments = "", string constraints = "",
            dag outputs = (outs), dag inputs = (ins), list<dag> pattern = []>
  : Inst16<prefix, opcode, Off   , mnemonic, arguments, constraints,
           outputs, inputs, pattern>;

class I24  <Prefix prefix, bits<8> opcode,
            string mnemonic, string arguments = "", string constraints = "",
            dag outputs = (outs), dag inputs = (ins), list<dag> pattern = []>
  : Inst24<prefix, opcode,  NoImm, mnemonic, arguments, constraints,
           outputs, inputs, pattern>;
class I24i <Prefix prefix, bits<8> opcode,
            string mnemonic, string arguments = "", string constraints = "",
            dag outputs = (outs), dag inputs = (ins), list<dag> pattern = []>
  : Inst24<prefix, opcode,    Imm, mnemonic, arguments, constraints,
           outputs, inputs, pattern>;
class I24o <Prefix prefix, bits<8> opcode,
            string mnemonic, string arguments = "", string constraints = "",
            dag outputs = (outs), dag inputs = (ins), list<dag> pattern = []>
  : Inst24<prefix, opcode, Off   , mnemonic, arguments, constraints,
           outputs, inputs, pattern>;

class P<dag outs = (outs), dag ins = (ins), list<dag> pattern = []>
  : Z80Inst<NoPre, 0, NoImm, 0, outs, ins, pattern> {
  let isPseudo = 1;
}
