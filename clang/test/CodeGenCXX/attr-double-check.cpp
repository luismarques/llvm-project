// RUN: %clang_cc1 -S -emit-llvm %s -triple riscv32 -o - | FileCheck %s
// RUN: %clang_cc1 -S %s -triple riscv32 -o - | FileCheck -check-prefix=CHECK-ASM %s

extern "C" {

void test_eq(int a, int b) {
  // CHECK: @test_eq
  // CHECK: callbr void asm sideeffect "bne $0, $1, $3\0Abeq $0, $1, $2\0Aunimp\0Aunimp", "r,r,!i,!i"(i32 %0, i32 %1)
  // CHECK-NEXT: to label %asm.fallthrough [label %if.then, label %if.end]
  // CHECK-EMPTY:
  // CHECK-NEXT: asm.fallthrough:                                  ; preds = %entry
  // CHECK-NEXT:   br label %if.then
  // CHECK-EMPTY:
  // CHECK-NEXT: if.then:                                          ; preds = %asm.fallthrough, %entry
  // CHECK-NEXT:   br label %if.end
  // CHECK-EMPTY:
  // CHECK-NEXT: if.end:                                           ; preds = %if.then, %entry
  // CHECK-NEXT:   ret void
  //
  // CHECK-ASM:      test_eq:
  // CHECK-ASM:        #APP
  // CHECK-ASM-NEXT:   bne a0, a1, .LBB0_3
  // CHECK-ASM-NEXT:   beq a0, a1, .LBB0_2
  // CHECK-ASM-NEXT:   unimp
  // CHECK-ASM-NEXT:   unimp
  // CHECK-ASM-NEXT:   #NO_APP
  // CHECK-ASM-NEXT:   j .LBB0_1
  // CHECK-ASM-NEXT: .LBB0_1:                                # %asm.fallthrough
  // CHECK-ASM-NEXT:   j .LBB0_2
  // CHECK-ASM-NEXT: .LBB0_2:                                # Block address taken
  // CHECK-ASM-NEXT:                                         # %if.then
  // CHECK-ASM-NEXT:                                         # Label of block must be emitted
  // CHECK-ASM-NEXT:   j .LBB0_3
  // CHECK-ASM-NEXT: .LBB0_3:                                # Block address taken
  // CHECK-ASM-NEXT:                                         # %if.end
  // CHECK-ASM-NEXT:                                         # Label of block must be emitted
  [[clang::double_check]] if (a == b) {}
}

void test_ne(int a, int b) {
  // CHECK: @test_ne
  // CHECK: callbr void asm sideeffect "beq $0, $1, $3\0Abne $0, $1, $2\0Aunimp\0Aunimp", "r,r,!i,!i"(i32 %0, i32 %1)
  // CHECK-ASM:      test_ne:
  // CHECK-ASM:        beq a0, a1, .LBB1_3
  // CHECK-ASM-NEXT:   bne a0, a1, .LBB1_2
  [[clang::double_check]] if (a != b) {}
}

void test_gt(int a, int b) {
  // CHECK: @test_gt
  // CHECK: callbr void asm sideeffect "ble $0, $1, $3\0Abgt $0, $1, $2\0Aunimp\0Aunimp", "r,r,!i,!i"(i32 %0, i32 %1)
  // CHECK-ASM:      test_gt:
  // CHECK-ASM:        bge a1, a0, .LBB2_3
  // CHECK-ASM-NEXT:   blt a1, a0, .LBB2_2
  [[clang::double_check]] if (a > b) {}
}

void test_ge(int a, int b) {
  // CHECK: @test_ge
  // CHECK: callbr void asm sideeffect "blt $0, $1, $3\0Abge $0, $1, $2\0Aunimp\0Aunimp", "r,r,!i,!i"(i32 %0, i32 %1)
  // CHECK-ASM:      test_ge:
  // CHECK-ASM:        blt a0, a1, .LBB3_3
  // CHECK-ASM-NEXT:   bge a0, a1, .LBB3_2
  [[clang::double_check]] if (a >= b) {}
}

void test_lt(int a, int b) {
  // CHECK: @test_lt
  // CHECK: callbr void asm sideeffect "bge $0, $1, $3\0Ablt $0, $1, $2\0Aunimp\0Aunimp", "r,r,!i,!i"(i32 %0, i32 %1)
  // CHECK-ASM:      test_lt:
  // CHECK-ASM:        bge a0, a1, .LBB4_3
  // CHECK-ASM-NEXT:   blt a0, a1, .LBB4_2
  [[clang::double_check]] if (a < b) {}
}

void test_le(int a, int b) {
  // CHECK: @test_le
  // CHECK: callbr void asm sideeffect "bgt $0, $1, $3\0Able $0, $1, $2\0Aunimp\0Aunimp", "r,r,!i,!i"(i32 %0, i32 %1)
  // CHECK-ASM:      test_le:
  // CHECK-ASM:        blt a1, a0, .LBB5_3
  // CHECK-ASM-NEXT:   bge a1, a0, .LBB5_2
  [[clang::double_check]] if (a <= b) {}
}

// The double check applies to the outer comparison expression only.
void test_sub_expr(int a, int b) {
  // CHECK: @test_sub_expr
  // CHECK: callbr void asm sideeffect "bne $0, $1, $3\0Abeq $0, $1, $2\0Aunimp\0Aunimp", "r,r,!i,!i"(i32 %mul, i32 %add)
  // CHECK-NOT: callbr
  // CHECK: ret void
  [[clang::double_check]] if ((a*b) == (a+b)) {}
}

int test_else(int a, int b) {
  // CHECK: @test_else
  // CHECK: callbr void asm sideeffect "bne $0, $1, $3\0Abeq $0, $1, $2\0Aunimp\0Aunimp", "r,r,!i,!i"(i32 %0, i32 %1)
  // CHECK-NEXT: to label %asm.fallthrough [label %if.then, label %if.else]
  // CHECK-EMPTY:
  // CHECK-NEXT: asm.fallthrough:                                  ; preds = %entry
  // CHECK-NEXT:   br label %if.then
  // CHECK-EMPTY:
  // CHECK-NEXT: if.then:                                          ; preds = %asm.fallthrough, %entry
  // CHECK-NEXT:   store i32 1, ptr %retval, align 4
  // CHECK-NEXT:   br label %return
  // CHECK-EMPTY:
  // CHECK-NEXT: if.else:                                          ; preds = %entry
  // CHECK-NEXT:   store i32 2, ptr %retval, align 4
  // CHECK-NEXT:   br label %return
  // CHECK-EMPTY:
  // CHECK-NEXT: return:                                           ; preds = %if.else, %if.then
  // CHECK-NEXT:  %2 = load i32, ptr %retval, align 4
  // CHECK-NEXT:  ret i32 %2
  [[clang::double_check]] if (a == b) { return 1; } else { return 2; }
}

void test_else_unguarded_if(int a, int b) {
  // CHECK: @test_else_unguarded_if
  // CHECK: callbr void asm sideeffect
  // CHECK-NOT: callbr
  // CHECK: ret void
  [[clang::double_check]] if (a == b) {} else if (b == 42) {}
}

void test_else_guarded_if(int a, int b) {
  // CHECK: @test_else_guarded_if
  // CHECK: callbr void asm sideeffect
  // CHECK: callbr void asm sideeffect
  // CHECK: ret void
  // CHECK-ASM:      test_else_guarded_if:
  // CHECK-ASM:        bne a0, a1, .LBB9_3
  // CHECK-ASM-NEXT:   beq a0, a1, .LBB9_2
  // CHECK-ASM:        li a1, 42
  // CHECK-ASM:        bne a0, a1, .LBB9_6
  // CHECK-ASM-NEXT:   beq a0, a1, .LBB9_5
  [[clang::double_check]] if (a == b) {} else [[clang::double_check]] if (b == 42) {}
}

}
