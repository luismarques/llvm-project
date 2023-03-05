# RUN: not llvm-mc -triple riscv64 -mattr=+experimental-zbp < %s 2>&1 | FileCheck %s

# Immediate operand out of range
gorci t0, t1, 64 # CHECK: :[[@LINE]]:15: error: immediate must be an integer in the range [0, 63]
gorci t0, t1, -1 # CHECK: :[[@LINE]]:15: error: immediate must be an integer in the range [0, 63]
# Immediate operand out of range
grevi t0, t1, 64 # CHECK: :[[@LINE]]:15: error: immediate must be an integer in the range [0, 63]
grevi t0, t1, -1 # CHECK: :[[@LINE]]:15: error: immediate must be an integer in the range [0, 63]
# Immediate operand out of range
shfli t0, t1, 32 # CHECK: :[[@LINE]]:15: error: immediate must be an integer in the range [0, 31]
shfli t0, t1, -1 # CHECK: :[[@LINE]]:15: error: immediate must be an integer in the range [0, 31]
# Immediate operand out of range
unshfli t0, t1, 32 # CHECK: :[[@LINE]]:17: error: immediate must be an integer in the range [0, 31]
unshfli t0, t1, -1 # CHECK: :[[@LINE]]:17: error: immediate must be an integer in the range [0, 31]
