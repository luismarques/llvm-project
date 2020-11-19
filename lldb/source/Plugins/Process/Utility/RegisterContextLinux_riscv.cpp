//===-- RegisterContextLinux_riscv.cpp -----------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===---------------------------------------------------------------------===//


#include <stddef.h>
#include <vector>

#include "RegisterContextLinux_riscv.h"
#include "lldb-riscv-linux-register-enums.h"

using namespace lldb;
using namespace lldb_private;

// Include RegisterInfos_riscv64 to declare our g_register_infos_rv64
// structure.
#define DECLARE_REGISTER_INFOS_RISCV_STRUCT
#define LINUX_RISCV64
#include "RegisterInfos_riscv.h"
#undef LINUX_RISCV64
#undef DECLARE_REGISTER_INFOS_RISCV_STRUCT

// riscv general purpose registers.
const uint32_t g_gp_regnums_riscv[] = {
    gpr_zero_riscv,    gpr_r1_riscv,    gpr_r2_riscv,
    gpr_r3_riscv,      gpr_r4_riscv,    gpr_r5_riscv,
    gpr_r6_riscv,      gpr_r7_riscv,    gpr_r8_riscv,
    gpr_r9_riscv,      gpr_r10_riscv,   gpr_r11_riscv,
    gpr_r12_riscv,     gpr_r13_riscv,   gpr_r14_riscv,
    gpr_r15_riscv,     gpr_r16_riscv,   gpr_r17_riscv,
    gpr_r18_riscv,     gpr_r19_riscv,   gpr_r20_riscv,
    gpr_r21_riscv,     gpr_r22_riscv,   gpr_r23_riscv,
    gpr_r24_riscv,     gpr_r25_riscv,   gpr_r26_riscv,
    gpr_r27_riscv,     gpr_gp_riscv,    gpr_sp_riscv,
    gpr_r30_riscv,     gpr_ra_riscv,    gpr_sr_riscv,
    gpr_mullo_riscv,   gpr_mulhi_riscv, gpr_badvaddr_riscv,
    gpr_cause_riscv,   gpr_pc_riscv,    gpr_config5_riscv,
    LLDB_INVALID_REGNUM // register sets need to end with this flag
};

static_assert((sizeof(g_gp_regnums_riscv) / sizeof(g_gp_regnums_riscv[0])) -
                      1 ==
                  k_num_gpr_registers_riscv,
              "g_gp_regnums_riscv64 has wrong number of register infos");

// riscv floating point registers.
const uint32_t g_fp_regnums_riscv[] = {
    fpr_f0_riscv,      fpr_f1_riscv,  fpr_f2_riscv,      fpr_f3_riscv,
    fpr_f4_riscv,      fpr_f5_riscv,  fpr_f6_riscv,      fpr_f7_riscv,
    fpr_f8_riscv,      fpr_f9_riscv,  fpr_f10_riscv,     fpr_f11_riscv,
    fpr_f12_riscv,     fpr_f13_riscv, fpr_f14_riscv,     fpr_f15_riscv,
    fpr_f16_riscv,     fpr_f17_riscv, fpr_f18_riscv,     fpr_f19_riscv,
    fpr_f20_riscv,     fpr_f21_riscv, fpr_f22_riscv,     fpr_f23_riscv,
    fpr_f24_riscv,     fpr_f25_riscv, fpr_f26_riscv,     fpr_f27_riscv,
    fpr_f28_riscv,     fpr_f29_riscv, fpr_f30_riscv,     fpr_f31_riscv,
    fpr_fcsr_riscv,    fpr_fir_riscv, fpr_config5_riscv,
    LLDB_INVALID_REGNUM // register sets need to end with this flag
};

static_assert((sizeof(g_fp_regnums_riscv) / sizeof(g_fp_regnums_riscv[0])) -
                      1 ==
                  k_num_fpr_registers_riscv,
              "g_fp_regnums_riscv has wrong number of register infos");

// Number of register sets provided by this context.
constexpr size_t k_num_register_sets = 2;

static const RegisterSet g_reg_sets_riscv[k_num_register_sets] = {
    {"General Purpose Registers", "gpr", k_num_gpr_registers_riscv,
     g_gp_regnums_riscv},
    {"Floating Point Registers", "fpu", k_num_fpr_registers_riscv,
     g_fp_regnums_riscv},
};

const RegisterSet *
RegisterContextLinux_riscv::GetRegisterSet(size_t set) const {
  if (set >= k_num_register_sets)
    return nullptr;

  switch (m_target_arch.GetMachine()) {
  case llvm::Triple::riscv64:
    return &g_reg_sets_riscv[set];
  default:
    assert(false && "Unhandled target architecture.");
    return nullptr;
  }
  return nullptr;
}

size_t
RegisterContextLinux_riscv::GetRegisterSetCount() const {
  return k_num_register_sets;
}

static const RegisterInfo *GetRegisterInfoPtr(const ArchSpec &target_arch) {
  switch (target_arch.GetMachine()) {
  case llvm::Triple::riscv64:
    return g_register_infos_rv64;
  default:
    assert(false && "Unhandled target architecture.");
    return nullptr;
  }
}

static uint32_t GetRegisterInfoCount(const ArchSpec &target_arch) {
  switch (target_arch.GetMachine()) {
  case llvm::Triple::riscv64:
    return static_cast<uint32_t>(sizeof(g_register_infos_rv64) /
                                 sizeof(g_register_infos_rv64[0]));
  default:
    assert(false && "Unhandled target architecture.");
    return 0;
  }
}

uint32_t GetUserRegisterInfoCount(const ArchSpec &target_arch) {
  switch (target_arch.GetMachine()) {
  case llvm::Triple::riscv64:
    return static_cast<uint32_t>(k_num_user_registers_riscv);
  default:
    assert(false && "Unhandled target architecture.");
    return 0;
  }
}

RegisterContextLinux_riscv::RegisterContextLinux_riscv(
    const ArchSpec &target_arch)
    : lldb_private::RegisterInfoInterface(target_arch),
      m_register_info_p(GetRegisterInfoPtr(target_arch)),
      m_register_info_count(GetRegisterInfoCount(target_arch)),
      m_user_register_count(
          GetUserRegisterInfoCount(target_arch)) {}

size_t RegisterContextLinux_riscv::GetGPRSize() const {
  return sizeof(32);
}

const RegisterInfo *RegisterContextLinux_riscv::GetRegisterInfo() const {
  return m_register_info_p;
}

uint32_t RegisterContextLinux_riscv::GetRegisterCount() const {
  return m_register_info_count;
}

uint32_t RegisterContextLinux_riscv::GetUserRegisterCount() const {
  return m_user_register_count;
}
