//===-- CodeGen/RuntimeLibcalls.h - Runtime Library Calls -------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file defines the enum representing the list of runtime library calls
// the backend may emit during code generation, and also some helper functions.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_CODEGEN_RUNTIMELIBCALLS_H
#define LLVM_CODEGEN_RUNTIMELIBCALLS_H

#include "llvm/CodeGen/ValueTypes.h"

namespace llvm {
namespace RTLIB {
  /// RTLIB::Libcall enum - This enum defines all of the runtime library calls
  /// the backend can emit.  The various long double types cannot be merged,
  /// because 80-bit library functions use "xf" and 128-bit use "tf".
  ///
  /// When adding PPCF128 functions here, note that their names generally need
  /// to be overridden for Darwin with the xxx$LDBL128 form.  See
  /// PPCISelLowering.cpp.
  ///
  enum Libcall {
    // Integer
    ZEXT_I16_I24,
    SEXT_I16_I24,
    SEXT_I24_I32,
    NOT_I16,
    NOT_I24,
    NOT_I32,
    AND_I8,
    AND_I16,
    AND_I24,
    AND_I32,
    AND_I64,
    AND_I128,
    OR_I8,
    OR_I16,
    OR_I24,
    OR_I32,
    OR_I64,
    OR_I128,
    XOR_I8,
    XOR_I16,
    XOR_I24,
    XOR_I32,
    XOR_I64,
    XOR_I128,
    SHL_I8,
    SHL_I16,
    SHL_I24,
    SHL_I32,
    SHL_I64,
    SHL_I128,
    SHL_I16_I8,
    SHL_I24_I8,
    SRL_I8,
    SRL_I16,
    SRL_I16_I8,
    SRL_I24,
    SRL_I24_I8,
    SRL_I32,
    SRL_I64,
    SRL_I128,
    SRA_I8,
    SRA_I16,
    SRA_I16_I8,
    SRA_I24,
    SRA_I24_I8,
    SRA_I32,
    SRA_I64,
    SRA_I128,
    CMP_I32,
    CMP_I16_0,
    CMP_I24_0,
    CMP_I32_0,
    SCMP,
    NEG_I16,
    NEG_I24,
    NEG_I32,
    NEG_I64,
    ADD_I32,
    ADD_I32_I8,
    SUB_I32,
    MUL_I8,
    MUL_I16,
    MUL_I24,
    MUL_I24_I8,
    MUL_I32,
    MUL_I64,
    MUL_I128,
    MULO_I32,
    MULO_I64,
    MULO_I128,
    SDIV_I8,
    SDIV_I16,
    SDIV_I24,
    SDIV_I32,
    SDIV_I64,
    SDIV_I128,
    UDIV_I8,
    UDIV_I16,
    UDIV_I24,
    UDIV_I32,
    UDIV_I64,
    UDIV_I128,
    SREM_I8,
    SREM_I16,
    SREM_I24,
    SREM_I32,
    SREM_I64,
    SREM_I128,
    UREM_I8,
    UREM_I16,
    UREM_I24,
    UREM_I32,
    UREM_I64,
    UREM_I128,
    SDIVREM_I8,
    SDIVREM_I16,
    SDIVREM_I24,
    SDIVREM_I32,
    SDIVREM_I64,
    SDIVREM_I128,
    UDIVREM_I8,
    UDIVREM_I16,
    UDIVREM_I24,
    UDIVREM_I32,
    UDIVREM_I64,
    UDIVREM_I128,

    // FLOATING POINT
    ADD_F32,
    ADD_F64,
    ADD_F80,
    ADD_F128,
    ADD_PPCF128,
    SUB_F32,
    SUB_F64,
    SUB_F80,
    SUB_F128,
    SUB_PPCF128,
    MUL_F32,
    MUL_F64,
    MUL_F80,
    MUL_F128,
    MUL_PPCF128,
    DIV_F32,
    DIV_F64,
    DIV_F80,
    DIV_F128,
    DIV_PPCF128,
    REM_F32,
    REM_F64,
    REM_F80,
    REM_F128,
    REM_PPCF128,
    FMA_F32,
    FMA_F64,
    FMA_F80,
    FMA_F128,
    FMA_PPCF128,
    POWI_F32,
    POWI_F64,
    POWI_F80,
    POWI_F128,
    POWI_PPCF128,
    SQRT_F32,
    SQRT_F64,
    SQRT_F80,
    SQRT_F128,
    SQRT_PPCF128,
    LOG_F32,
    LOG_F64,
    LOG_F80,
    LOG_F128,
    LOG_PPCF128,
    LOG2_F32,
    LOG2_F64,
    LOG2_F80,
    LOG2_F128,
    LOG2_PPCF128,
    LOG10_F32,
    LOG10_F64,
    LOG10_F80,
    LOG10_F128,
    LOG10_PPCF128,
    EXP_F32,
    EXP_F64,
    EXP_F80,
    EXP_F128,
    EXP_PPCF128,
    EXP2_F32,
    EXP2_F64,
    EXP2_F80,
    EXP2_F128,
    EXP2_PPCF128,
    SIN_F32,
    SIN_F64,
    SIN_F80,
    SIN_F128,
    SIN_PPCF128,
    COS_F32,
    COS_F64,
    COS_F80,
    COS_F128,
    COS_PPCF128,
    SINCOS_F32,
    SINCOS_F64,
    SINCOS_F80,
    SINCOS_F128,
    SINCOS_PPCF128,
    POW_F32,
    POW_F64,
    POW_F80,
    POW_F128,
    POW_PPCF128,
    CEIL_F32,
    CEIL_F64,
    CEIL_F80,
    CEIL_F128,
    CEIL_PPCF128,
    TRUNC_F32,
    TRUNC_F64,
    TRUNC_F80,
    TRUNC_F128,
    TRUNC_PPCF128,
    RINT_F32,
    RINT_F64,
    RINT_F80,
    RINT_F128,
    RINT_PPCF128,
    NEARBYINT_F32,
    NEARBYINT_F64,
    NEARBYINT_F80,
    NEARBYINT_F128,
    NEARBYINT_PPCF128,
    ROUND_F32,
    ROUND_F64,
    ROUND_F80,
    ROUND_F128,
    ROUND_PPCF128,
    FLOOR_F32,
    FLOOR_F64,
    FLOOR_F80,
    FLOOR_F128,
    FLOOR_PPCF128,
    COPYSIGN_F32,
    COPYSIGN_F64,
    COPYSIGN_F80,
    COPYSIGN_F128,
    COPYSIGN_PPCF128,
    FMIN_F32,
    FMIN_F64,
    FMIN_F80,
    FMIN_F128,
    FMIN_PPCF128,
    FMAX_F32,
    FMAX_F64,
    FMAX_F80,
    FMAX_F128,
    FMAX_PPCF128,

    // CONVERSION
    FPEXT_F32_PPCF128,
    FPEXT_F64_PPCF128,
    FPEXT_F64_F128,
    FPEXT_F32_F128,
    FPEXT_F32_F64,
    FPEXT_F16_F32,
    FPROUND_F32_F16,
    FPROUND_F64_F16,
    FPROUND_F80_F16,
    FPROUND_F128_F16,
    FPROUND_PPCF128_F16,
    FPROUND_F64_F32,
    FPROUND_F80_F32,
    FPROUND_F128_F32,
    FPROUND_PPCF128_F32,
    FPROUND_F80_F64,
    FPROUND_F128_F64,
    FPROUND_PPCF128_F64,
    FPTOSINT_F32_I32,
    FPTOSINT_F32_I64,
    FPTOSINT_F32_I128,
    FPTOSINT_F64_I32,
    FPTOSINT_F64_I64,
    FPTOSINT_F64_I128,
    FPTOSINT_F80_I32,
    FPTOSINT_F80_I64,
    FPTOSINT_F80_I128,
    FPTOSINT_F128_I32,
    FPTOSINT_F128_I64,
    FPTOSINT_F128_I128,
    FPTOSINT_PPCF128_I32,
    FPTOSINT_PPCF128_I64,
    FPTOSINT_PPCF128_I128,
    FPTOUINT_F32_I32,
    FPTOUINT_F32_I64,
    FPTOUINT_F32_I128,
    FPTOUINT_F64_I32,
    FPTOUINT_F64_I64,
    FPTOUINT_F64_I128,
    FPTOUINT_F80_I32,
    FPTOUINT_F80_I64,
    FPTOUINT_F80_I128,
    FPTOUINT_F128_I32,
    FPTOUINT_F128_I64,
    FPTOUINT_F128_I128,
    FPTOUINT_PPCF128_I32,
    FPTOUINT_PPCF128_I64,
    FPTOUINT_PPCF128_I128,
    SINTTOFP_I32_F32,
    SINTTOFP_I32_F64,
    SINTTOFP_I32_F80,
    SINTTOFP_I32_F128,
    SINTTOFP_I32_PPCF128,
    SINTTOFP_I64_F32,
    SINTTOFP_I64_F64,
    SINTTOFP_I64_F80,
    SINTTOFP_I64_F128,
    SINTTOFP_I64_PPCF128,
    SINTTOFP_I128_F32,
    SINTTOFP_I128_F64,
    SINTTOFP_I128_F80,
    SINTTOFP_I128_F128,
    SINTTOFP_I128_PPCF128,
    UINTTOFP_I32_F32,
    UINTTOFP_I32_F64,
    UINTTOFP_I32_F80,
    UINTTOFP_I32_F128,
    UINTTOFP_I32_PPCF128,
    UINTTOFP_I64_F32,
    UINTTOFP_I64_F64,
    UINTTOFP_I64_F80,
    UINTTOFP_I64_F128,
    UINTTOFP_I64_PPCF128,
    UINTTOFP_I128_F32,
    UINTTOFP_I128_F64,
    UINTTOFP_I128_F80,
    UINTTOFP_I128_F128,
    UINTTOFP_I128_PPCF128,

    // COMPARISON
    OEQ_F32,
    OEQ_F64,
    OEQ_F128,
    OEQ_PPCF128,
    UNE_F32,
    UNE_F64,
    UNE_F128,
    UNE_PPCF128,
    OGE_F32,
    OGE_F64,
    OGE_F128,
    OGE_PPCF128,
    OLT_F32,
    OLT_F64,
    OLT_F128,
    OLT_PPCF128,
    OLE_F32,
    OLE_F64,
    OLE_F128,
    OLE_PPCF128,
    OGT_F32,
    OGT_F64,
    OGT_F128,
    OGT_PPCF128,
    UO_F32,
    UO_F64,
    UO_F128,
    UO_PPCF128,
    O_F32,
    O_F64,
    O_F128,
    O_PPCF128,

    // MEMORY
    MEMCPY,
    MEMSET,
    MEMMOVE,

    // ELEMENT-WISE ATOMIC MEMORY
    MEMCPY_ELEMENT_ATOMIC_1,
    MEMCPY_ELEMENT_ATOMIC_2,
    MEMCPY_ELEMENT_ATOMIC_4,
    MEMCPY_ELEMENT_ATOMIC_8,
    MEMCPY_ELEMENT_ATOMIC_16,

    // EXCEPTION HANDLING
    UNWIND_RESUME,

    // Note: there's two sets of atomics libcalls; see
    // <http://llvm.org/docs/Atomics.html> for more info on the
    // difference between them.

    // Atomic '__sync_*' libcalls.
    SYNC_VAL_COMPARE_AND_SWAP_1,
    SYNC_VAL_COMPARE_AND_SWAP_2,
    SYNC_VAL_COMPARE_AND_SWAP_4,
    SYNC_VAL_COMPARE_AND_SWAP_8,
    SYNC_VAL_COMPARE_AND_SWAP_16,
    SYNC_LOCK_TEST_AND_SET_1,
    SYNC_LOCK_TEST_AND_SET_2,
    SYNC_LOCK_TEST_AND_SET_4,
    SYNC_LOCK_TEST_AND_SET_8,
    SYNC_LOCK_TEST_AND_SET_16,
    SYNC_FETCH_AND_ADD_1,
    SYNC_FETCH_AND_ADD_2,
    SYNC_FETCH_AND_ADD_4,
    SYNC_FETCH_AND_ADD_8,
    SYNC_FETCH_AND_ADD_16,
    SYNC_FETCH_AND_SUB_1,
    SYNC_FETCH_AND_SUB_2,
    SYNC_FETCH_AND_SUB_4,
    SYNC_FETCH_AND_SUB_8,
    SYNC_FETCH_AND_SUB_16,
    SYNC_FETCH_AND_AND_1,
    SYNC_FETCH_AND_AND_2,
    SYNC_FETCH_AND_AND_4,
    SYNC_FETCH_AND_AND_8,
    SYNC_FETCH_AND_AND_16,
    SYNC_FETCH_AND_OR_1,
    SYNC_FETCH_AND_OR_2,
    SYNC_FETCH_AND_OR_4,
    SYNC_FETCH_AND_OR_8,
    SYNC_FETCH_AND_OR_16,
    SYNC_FETCH_AND_XOR_1,
    SYNC_FETCH_AND_XOR_2,
    SYNC_FETCH_AND_XOR_4,
    SYNC_FETCH_AND_XOR_8,
    SYNC_FETCH_AND_XOR_16,
    SYNC_FETCH_AND_NAND_1,
    SYNC_FETCH_AND_NAND_2,
    SYNC_FETCH_AND_NAND_4,
    SYNC_FETCH_AND_NAND_8,
    SYNC_FETCH_AND_NAND_16,
    SYNC_FETCH_AND_MAX_1,
    SYNC_FETCH_AND_MAX_2,
    SYNC_FETCH_AND_MAX_4,
    SYNC_FETCH_AND_MAX_8,
    SYNC_FETCH_AND_MAX_16,
    SYNC_FETCH_AND_UMAX_1,
    SYNC_FETCH_AND_UMAX_2,
    SYNC_FETCH_AND_UMAX_4,
    SYNC_FETCH_AND_UMAX_8,
    SYNC_FETCH_AND_UMAX_16,
    SYNC_FETCH_AND_MIN_1,
    SYNC_FETCH_AND_MIN_2,
    SYNC_FETCH_AND_MIN_4,
    SYNC_FETCH_AND_MIN_8,
    SYNC_FETCH_AND_MIN_16,
    SYNC_FETCH_AND_UMIN_1,
    SYNC_FETCH_AND_UMIN_2,
    SYNC_FETCH_AND_UMIN_4,
    SYNC_FETCH_AND_UMIN_8,
    SYNC_FETCH_AND_UMIN_16,

    // Atomic '__atomic_*' libcalls.
    ATOMIC_LOAD,
    ATOMIC_LOAD_1,
    ATOMIC_LOAD_2,
    ATOMIC_LOAD_4,
    ATOMIC_LOAD_8,
    ATOMIC_LOAD_16,

    ATOMIC_STORE,
    ATOMIC_STORE_1,
    ATOMIC_STORE_2,
    ATOMIC_STORE_4,
    ATOMIC_STORE_8,
    ATOMIC_STORE_16,

    ATOMIC_EXCHANGE,
    ATOMIC_EXCHANGE_1,
    ATOMIC_EXCHANGE_2,
    ATOMIC_EXCHANGE_4,
    ATOMIC_EXCHANGE_8,
    ATOMIC_EXCHANGE_16,

    ATOMIC_COMPARE_EXCHANGE,
    ATOMIC_COMPARE_EXCHANGE_1,
    ATOMIC_COMPARE_EXCHANGE_2,
    ATOMIC_COMPARE_EXCHANGE_4,
    ATOMIC_COMPARE_EXCHANGE_8,
    ATOMIC_COMPARE_EXCHANGE_16,

    ATOMIC_FETCH_ADD_1,
    ATOMIC_FETCH_ADD_2,
    ATOMIC_FETCH_ADD_4,
    ATOMIC_FETCH_ADD_8,
    ATOMIC_FETCH_ADD_16,

    ATOMIC_FETCH_SUB_1,
    ATOMIC_FETCH_SUB_2,
    ATOMIC_FETCH_SUB_4,
    ATOMIC_FETCH_SUB_8,
    ATOMIC_FETCH_SUB_16,

    ATOMIC_FETCH_AND_1,
    ATOMIC_FETCH_AND_2,
    ATOMIC_FETCH_AND_4,
    ATOMIC_FETCH_AND_8,
    ATOMIC_FETCH_AND_16,

    ATOMIC_FETCH_OR_1,
    ATOMIC_FETCH_OR_2,
    ATOMIC_FETCH_OR_4,
    ATOMIC_FETCH_OR_8,
    ATOMIC_FETCH_OR_16,

    ATOMIC_FETCH_XOR_1,
    ATOMIC_FETCH_XOR_2,
    ATOMIC_FETCH_XOR_4,
    ATOMIC_FETCH_XOR_8,
    ATOMIC_FETCH_XOR_16,

    ATOMIC_FETCH_NAND_1,
    ATOMIC_FETCH_NAND_2,
    ATOMIC_FETCH_NAND_4,
    ATOMIC_FETCH_NAND_8,
    ATOMIC_FETCH_NAND_16,

    // Stack Protector Fail.
    STACKPROTECTOR_CHECK_FAIL,

    // Deoptimization.
    DEOPTIMIZE,

    UNKNOWN_LIBCALL
  };

  /// getFPEXT - Return the FPEXT_*_* value for the given types, or
  /// UNKNOWN_LIBCALL if there is none.
  Libcall getFPEXT(EVT OpVT, EVT RetVT);

  /// getFPROUND - Return the FPROUND_*_* value for the given types, or
  /// UNKNOWN_LIBCALL if there is none.
  Libcall getFPROUND(EVT OpVT, EVT RetVT);

  /// getFPTOSINT - Return the FPTOSINT_*_* value for the given types, or
  /// UNKNOWN_LIBCALL if there is none.
  Libcall getFPTOSINT(EVT OpVT, EVT RetVT);

  /// getFPTOUINT - Return the FPTOUINT_*_* value for the given types, or
  /// UNKNOWN_LIBCALL if there is none.
  Libcall getFPTOUINT(EVT OpVT, EVT RetVT);

  /// getSINTTOFP - Return the SINTTOFP_*_* value for the given types, or
  /// UNKNOWN_LIBCALL if there is none.
  Libcall getSINTTOFP(EVT OpVT, EVT RetVT);

  /// getUINTTOFP - Return the UINTTOFP_*_* value for the given types, or
  /// UNKNOWN_LIBCALL if there is none.
  Libcall getUINTTOFP(EVT OpVT, EVT RetVT);

  /// Return the SYNC_FETCH_AND_* value for the given opcode and type, or
  /// UNKNOWN_LIBCALL if there is none.
  Libcall getSYNC(unsigned Opc, MVT VT);

  /// getMEMCPY_ELEMENT_ATOMIC - Return MEMCPY_ELEMENT_ATOMIC_* value for the
  /// given element size or UNKNOW_LIBCALL if there is none.
  Libcall getMEMCPY_ELEMENT_ATOMIC(uint64_t ElementSize);
}
}

#endif
