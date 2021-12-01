// RUN: %clang_cc1 -S -emit-llvm %s -triple riscv32 -o - | FileCheck %s
// RUN: %clang_cc1 -S %s -triple riscv32 -o - | FileCheck -check-prefix=CHECK-ASM %s

extern "C" {

void test_eq(int a, int b) {
  // CHECK: @test_eq
  // CHECK: callbr void asm sideeffect "bne $0, $1, $3\0Abeq $0, $1, $2\0Aunimp\0Aunimp", "r,r,X,X"(i32 %0, i32 %1, i8* blockaddress(@test_eq, %if.then), i8* blockaddress(@test_eq, %if.end))
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
  // CHECK-ASM-NEXT:   bne a0, a1, .Ltmp0
  // CHECK-ASM-NEXT:   beq a0, a1, .Ltmp1
  // CHECK-ASM-NEXT:   unimp
  // CHECK-ASM-NEXT:   unimp
  // CHECK-ASM-NEXT:   #NO_APP
  // CHECK-ASM-NEXT:   j .LBB0_1
  // CHECK-ASM-NEXT: .LBB0_1:                                # %asm.fallthrough
  // CHECK-ASM-NEXT:   j .LBB0_2
  // CHECK-ASM-NEXT: .Ltmp1:                                 # Block address taken
  // CHECK-ASM-NEXT: .LBB0_2:                                # %if.then
  // CHECK-ASM-NEXT:   j .LBB0_3
  // CHECK-ASM-NEXT: .Ltmp0:                                 # Block address taken
  // CHECK-ASM-NEXT: .LBB0_3:                                # %if.end
  [[clang::double_check]] if (a == b) {}
}

void test_ne(int a, int b) {
  // CHECK: @test_ne
  // CHECK: callbr void asm sideeffect "beq $0, $1, $3\0Abne $0, $1, $2\0Aunimp\0Aunimp", "r,r,X,X"(i32 %0, i32 %1, i8* blockaddress(@test_ne, %if.then), i8* blockaddress(@test_ne, %if.end))
  // CHECK-ASM:      test_ne:
  // CHECK-ASM:        beq a0, a1, .Ltmp2
  // CHECK-ASM-NEXT:   bne a0, a1, .Ltmp3
  [[clang::double_check]] if (a != b) {}
}

void test_gt(int a, int b) {
  // CHECK: @test_gt
  // CHECK: callbr void asm sideeffect "ble $0, $1, $3\0Abgt $0, $1, $2\0Aunimp\0Aunimp", "r,r,X,X"(i32 %0, i32 %1, i8* blockaddress(@test_gt, %if.then), i8* blockaddress(@test_gt, %if.end))
  // CHECK-ASM:      test_gt:
  // CHECK-ASM:        bge a1, a0, .Ltmp4
  // CHECK-ASM-NEXT:   blt a1, a0, .Ltmp5
  [[clang::double_check]] if (a > b) {}
}

void test_ge(int a, int b) {
  // CHECK: @test_ge
  // CHECK: callbr void asm sideeffect "blt $0, $1, $3\0Abge $0, $1, $2\0Aunimp\0Aunimp", "r,r,X,X"(i32 %0, i32 %1, i8* blockaddress(@test_ge, %if.then), i8* blockaddress(@test_ge, %if.end))
  // CHECK-ASM:      test_ge:
  // CHECK-ASM:        blt a0, a1, .Ltmp6
  // CHECK-ASM-NEXT:   bge a0, a1, .Ltmp7
  [[clang::double_check]] if (a >= b) {}
}

void test_lt(int a, int b) {
  // CHECK: @test_lt
  // CHECK: callbr void asm sideeffect "bge $0, $1, $3\0Ablt $0, $1, $2\0Aunimp\0Aunimp", "r,r,X,X"(i32 %0, i32 %1, i8* blockaddress(@test_lt, %if.then), i8* blockaddress(@test_lt, %if.end))
  // CHECK-ASM:      test_lt:
  // CHECK-ASM:        bge a0, a1, .Ltmp8
  // CHECK-ASM-NEXT:   blt a0, a1, .Ltmp9
  [[clang::double_check]] if (a < b) {}
}

void test_le(int a, int b) {
  // CHECK: @test_le
  // CHECK: callbr void asm sideeffect "bgt $0, $1, $3\0Able $0, $1, $2\0Aunimp\0Aunimp", "r,r,X,X"(i32 %0, i32 %1, i8* blockaddress(@test_le, %if.then), i8* blockaddress(@test_le, %if.end))
  // CHECK-ASM:      test_le:
  // CHECK-ASM:        blt a1, a0, .Ltmp10
  // CHECK-ASM-NEXT:   bge a1, a0, .Ltmp11
  [[clang::double_check]] if (a <= b) {}
}

// The double check applies to the outer comparison expression only.
void test_sub_expr(int a, int b) {
  // CHECK: @test_sub_expr
  // CHECK: callbr void asm sideeffect "bne $0, $1, $3\0Abeq $0, $1, $2\0Aunimp\0Aunimp", "r,r,X,X"(i32 %mul, i32 %add, i8* blockaddress(@test_sub_expr, %if.then), i8* blockaddress(@test_sub_expr, %if.end))
  // CHECK-NOT: callbr
  // CHECK: ret void
  [[clang::double_check]] if ((a*b) == (a+b)) {}
}

int test_else(int a, int b) {
  // CHECK: @test_else
  // CHECK: callbr void asm sideeffect "bne $0, $1, $3\0Abeq $0, $1, $2\0Aunimp\0Aunimp", "r,r,X,X"(i32 %0, i32 %1, i8* blockaddress(@test_else, %if.then), i8* blockaddress(@test_else, %if.else))
  // CHECK-NEXT: to label %asm.fallthrough [label %if.then, label %if.else]
  // CHECK-EMPTY:
  // CHECK-NEXT: asm.fallthrough:                                  ; preds = %entry
  // CHECK-NEXT:   br label %if.then
  // CHECK-EMPTY:
  // CHECK-NEXT: if.then:                                          ; preds = %asm.fallthrough, %entry
  // CHECK-NEXT:   store i32 1, i32* %retval, align 4
  // CHECK-NEXT:   br label %return
  // CHECK-EMPTY:
  // CHECK-NEXT: if.else:                                          ; preds = %entry
  // CHECK-NEXT:   store i32 2, i32* %retval, align 4
  // CHECK-NEXT:   br label %return
  // CHECK-EMPTY:
  // CHECK-NEXT: return:                                           ; preds = %if.else, %if.then
  // CHECK-NEXT:  %2 = load i32, i32* %retval, align 4
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
  // CHECK-ASM:        bne a0, a1, .Ltmp18
  // CHECK-ASM-NEXT:   beq a0, a1, .Ltmp19
  // CHECK-ASM:        addi a1, zero, 42
  // CHECK-ASM:        bne a0, a1, .Ltmp20
  // CHECK-ASM-NEXT:   beq a0, a1, .Ltmp21
  [[clang::double_check]] if (a == b) {} else [[clang::double_check]] if (b == 42) {}
}

}
