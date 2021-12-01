// RUN: %clang_cc1 -triple=riscv32 -verify -fsyntax-only %s

void valid_eq(int a, int b) {
  __attribute__((double_check)) if (a == b) {}
}

void valid_ne(int a, char b) {
  __attribute__((double_check)) if (a != b) {}
}

void valid_gt(int a, unsigned long b) {
  __attribute__((double_check)) if (a > b) {}
}

void valid_ge(unsigned char a, long b) {
  __attribute__((double_check)) if (a >= b) {}
}

void valid_lt(unsigned a, unsigned b) {
  __attribute__((double_check)) if (a < b) {}
}

void valid_le(unsigned a, int b) {
  __attribute__((double_check)) if (a <= b) {}
}

// The double check applies to the outer comparison expression only.
void valid_sub_expr(int a, int b) {
  __attribute__((double_check)) if ((a*b) == (a+b)) {}
}

void valid_else(int a, int b) {
  __attribute__((double_check)) if (a == b) {} else {}
}

void valid_else_unguarded_if(int a, int b) {
  __attribute__((double_check)) if (a == b) {} else if (b == 42) {}
}

void valid_else_guarded_if(int a, int b) {
  __attribute__((double_check)) if (a == b) {} else __attribute__((double_check)) if (b == 42) {}
}

void invalid_expr(int a) {
  __attribute__((double_check)) if (a) {} // expected-error{{invalid expression for if statement with `double_check` attribute}}
}

void invalid_op(int a, int b) {
  __attribute__((double_check)) if (a + b) {} // expected-error{{invalid expression for if statement with `double_check` attribute}}
}

void invalid_type(int a, float b) {
  __attribute__((double_check)) if (a == b) {} // expected-error{{invalid expression for if statement with `double_check` attribute}}
}
