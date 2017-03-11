; NOTE: Assertions have been autogenerated by utils/update_llc_test_checks.py
; RUN: llc < %s -mtriple=i686-unknown-unknown -mattr=+sse3 | FileCheck %s --check-prefix=SSE3
; RUN: llc < %s -mtriple=i686-unknown-unknown -mattr=+sse4.1 | FileCheck %s --check-prefix=SSE41
; RUN: llc < %s -mtriple=i686-unknown-unknown -mattr=+avx | FileCheck %s --check-prefix=AVX-32
; RUN: llc < %s -mtriple=i686-unknown-unknown -mattr=+avx2 | FileCheck %s --check-prefix=AVX-32
; RUN: llc < %s -mtriple=x86_64-unknown-unknown -mattr=+avx2 | FileCheck %s --check-prefix=AVX-64

define <3 x i16> @zext_i8(<3 x i8>) {
; SSE3-LABEL: zext_i8:
; SSE3:       # BB#0:
; SSE3-NEXT:    movzbl {{[0-9]+}}(%esp), %eax
; SSE3-NEXT:    movd %eax, %xmm0
; SSE3-NEXT:    movzbl {{[0-9]+}}(%esp), %eax
; SSE3-NEXT:    pinsrw $1, %eax, %xmm0
; SSE3-NEXT:    movzbl {{[0-9]+}}(%esp), %eax
; SSE3-NEXT:    pinsrw $2, %eax, %xmm0
; SSE3-NEXT:    pxor %xmm1, %xmm1
; SSE3-NEXT:    pextrw $1, %xmm0, %edx
; SSE3-NEXT:    pextrw $2, %xmm0, %ecx
; SSE3-NEXT:    punpcklwd {{.*#+}} xmm0 = xmm0[0],xmm1[0],xmm0[1],xmm1[1],xmm0[2],xmm1[2],xmm0[3],xmm1[3]
; SSE3-NEXT:    movd %xmm0, %eax
; SSE3-NEXT:    # kill: %AX<def> %AX<kill> %EAX<kill>
; SSE3-NEXT:    # kill: %DX<def> %DX<kill> %EDX<kill>
; SSE3-NEXT:    # kill: %CX<def> %CX<kill> %ECX<kill>
; SSE3-NEXT:    retl
;
; SSE41-LABEL: zext_i8:
; SSE41:       # BB#0:
; SSE41-NEXT:    pxor %xmm0, %xmm0
; SSE41-NEXT:    pinsrb $0, {{[0-9]+}}(%esp), %xmm0
; SSE41-NEXT:    pinsrb $4, {{[0-9]+}}(%esp), %xmm0
; SSE41-NEXT:    pinsrb $8, {{[0-9]+}}(%esp), %xmm0
; SSE41-NEXT:    movd %xmm0, %eax
; SSE41-NEXT:    pextrw $2, %xmm0, %edx
; SSE41-NEXT:    pextrw $4, %xmm0, %ecx
; SSE41-NEXT:    # kill: %AX<def> %AX<kill> %EAX<kill>
; SSE41-NEXT:    # kill: %DX<def> %DX<kill> %EDX<kill>
; SSE41-NEXT:    # kill: %CX<def> %CX<kill> %ECX<kill>
; SSE41-NEXT:    retl
;
; AVX-32-LABEL: zext_i8:
; AVX-32:       # BB#0:
; AVX-32-NEXT:    vpxor %xmm0, %xmm0, %xmm0
; AVX-32-NEXT:    vpinsrb $0, {{[0-9]+}}(%esp), %xmm0, %xmm0
; AVX-32-NEXT:    vpinsrb $4, {{[0-9]+}}(%esp), %xmm0, %xmm0
; AVX-32-NEXT:    vpinsrb $8, {{[0-9]+}}(%esp), %xmm0, %xmm0
; AVX-32-NEXT:    vmovd %xmm0, %eax
; AVX-32-NEXT:    vpextrw $2, %xmm0, %edx
; AVX-32-NEXT:    vpextrw $4, %xmm0, %ecx
; AVX-32-NEXT:    # kill: %AX<def> %AX<kill> %EAX<kill>
; AVX-32-NEXT:    # kill: %DX<def> %DX<kill> %EDX<kill>
; AVX-32-NEXT:    # kill: %CX<def> %CX<kill> %ECX<kill>
; AVX-32-NEXT:    retl
;
; AVX-64-LABEL: zext_i8:
; AVX-64:       # BB#0:
; AVX-64-NEXT:    vpxor %xmm0, %xmm0, %xmm0
; AVX-64-NEXT:    vpinsrb $0, %edi, %xmm0, %xmm0
; AVX-64-NEXT:    vpinsrb $4, %esi, %xmm0, %xmm0
; AVX-64-NEXT:    vpinsrb $8, %edx, %xmm0, %xmm0
; AVX-64-NEXT:    vmovd %xmm0, %eax
; AVX-64-NEXT:    vpextrw $2, %xmm0, %edx
; AVX-64-NEXT:    vpextrw $4, %xmm0, %ecx
; AVX-64-NEXT:    # kill: %AX<def> %AX<kill> %EAX<kill>
; AVX-64-NEXT:    # kill: %DX<def> %DX<kill> %EDX<kill>
; AVX-64-NEXT:    # kill: %CX<def> %CX<kill> %ECX<kill>
; AVX-64-NEXT:    retq
  %2 = zext <3 x i8> %0 to <3 x i16>
  ret <3 x i16> %2
}

define <3 x i16> @sext_i8(<3 x i8>) {
; SSE3-LABEL: sext_i8:
; SSE3:       # BB#0:
; SSE3-NEXT:    movzbl {{[0-9]+}}(%esp), %eax
; SSE3-NEXT:    movd %eax, %xmm0
; SSE3-NEXT:    movzbl {{[0-9]+}}(%esp), %eax
; SSE3-NEXT:    pinsrw $1, %eax, %xmm0
; SSE3-NEXT:    movzbl {{[0-9]+}}(%esp), %eax
; SSE3-NEXT:    pinsrw $2, %eax, %xmm0
; SSE3-NEXT:    psllw $8, %xmm0
; SSE3-NEXT:    psraw $8, %xmm0
; SSE3-NEXT:    punpcklwd {{.*#+}} xmm0 = xmm0[0,0,1,1,2,2,3,3]
; SSE3-NEXT:    psrad $16, %xmm0
; SSE3-NEXT:    movd %xmm0, %eax
; SSE3-NEXT:    pextrw $2, %xmm0, %edx
; SSE3-NEXT:    pextrw $4, %xmm0, %ecx
; SSE3-NEXT:    # kill: %AX<def> %AX<kill> %EAX<kill>
; SSE3-NEXT:    # kill: %DX<def> %DX<kill> %EDX<kill>
; SSE3-NEXT:    # kill: %CX<def> %CX<kill> %ECX<kill>
; SSE3-NEXT:    retl
;
; SSE41-LABEL: sext_i8:
; SSE41:       # BB#0:
; SSE41-NEXT:    movd {{.*#+}} xmm0 = mem[0],zero,zero,zero
; SSE41-NEXT:    pinsrb $4, {{[0-9]+}}(%esp), %xmm0
; SSE41-NEXT:    pinsrb $8, {{[0-9]+}}(%esp), %xmm0
; SSE41-NEXT:    pslld $24, %xmm0
; SSE41-NEXT:    psrad $24, %xmm0
; SSE41-NEXT:    movd %xmm0, %eax
; SSE41-NEXT:    pextrw $2, %xmm0, %edx
; SSE41-NEXT:    pextrw $4, %xmm0, %ecx
; SSE41-NEXT:    # kill: %AX<def> %AX<kill> %EAX<kill>
; SSE41-NEXT:    # kill: %DX<def> %DX<kill> %EDX<kill>
; SSE41-NEXT:    # kill: %CX<def> %CX<kill> %ECX<kill>
; SSE41-NEXT:    retl
;
; AVX-32-LABEL: sext_i8:
; AVX-32:       # BB#0:
; AVX-32-NEXT:    vmovd {{.*#+}} xmm0 = mem[0],zero,zero,zero
; AVX-32-NEXT:    vpinsrb $4, {{[0-9]+}}(%esp), %xmm0, %xmm0
; AVX-32-NEXT:    vpinsrb $8, {{[0-9]+}}(%esp), %xmm0, %xmm0
; AVX-32-NEXT:    vpslld $24, %xmm0, %xmm0
; AVX-32-NEXT:    vpsrad $24, %xmm0, %xmm0
; AVX-32-NEXT:    vmovd %xmm0, %eax
; AVX-32-NEXT:    vpextrw $2, %xmm0, %edx
; AVX-32-NEXT:    vpextrw $4, %xmm0, %ecx
; AVX-32-NEXT:    # kill: %AX<def> %AX<kill> %EAX<kill>
; AVX-32-NEXT:    # kill: %DX<def> %DX<kill> %EDX<kill>
; AVX-32-NEXT:    # kill: %CX<def> %CX<kill> %ECX<kill>
; AVX-32-NEXT:    retl
;
; AVX-64-LABEL: sext_i8:
; AVX-64:       # BB#0:
; AVX-64-NEXT:    vmovd %edi, %xmm0
; AVX-64-NEXT:    vpinsrb $4, %esi, %xmm0, %xmm0
; AVX-64-NEXT:    vpinsrb $8, %edx, %xmm0, %xmm0
; AVX-64-NEXT:    vpslld $24, %xmm0, %xmm0
; AVX-64-NEXT:    vpsrad $24, %xmm0, %xmm0
; AVX-64-NEXT:    vmovd %xmm0, %eax
; AVX-64-NEXT:    vpextrw $2, %xmm0, %edx
; AVX-64-NEXT:    vpextrw $4, %xmm0, %ecx
; AVX-64-NEXT:    # kill: %AX<def> %AX<kill> %EAX<kill>
; AVX-64-NEXT:    # kill: %DX<def> %DX<kill> %EDX<kill>
; AVX-64-NEXT:    # kill: %CX<def> %CX<kill> %ECX<kill>
; AVX-64-NEXT:    retq
  %2 = sext <3 x i8> %0 to <3 x i16>
  ret <3 x i16> %2
}
