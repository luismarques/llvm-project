//===-- ABISysV_riscv.cpp ---------------------------------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//===----------------------------------------------------------------------===//

#include "ABISysV_riscv.h"

#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/StringSwitch.h"
#include "llvm/ADT/Triple.h"

#include "lldb/Core/Module.h"
#include "lldb/Core/PluginManager.h"
#include "lldb/Core/Value.h"
#include "lldb/Core/ValueObjectConstResult.h"
#include "lldb/Core/ValueObjectMemory.h"
#include "lldb/Core/ValueObjectRegister.h"
#include "lldb/Symbol/UnwindPlan.h"
#include "lldb/Target/Process.h"
#include "lldb/Target/RegisterContext.h"
#include "lldb/Target/StackFrame.h"
#include "lldb/Target/Target.h"
#include "lldb/Target/Thread.h"
#include "lldb/Utility/ConstString.h"
#include "lldb/Utility/DataExtractor.h"
#include "lldb/Utility/Log.h"
#include "lldb/Utility/RegisterValue.h"
#include "lldb/Utility/Status.h"

using namespace lldb;
using namespace lldb_private;

LLDB_PLUGIN_DEFINE(ABISysV_riscv)

bool ABISysV_riscv::CreateFunctionEntryUnwindPlan(UnwindPlan &unwind_plan) {
  unwind_plan.Clear();
  unwind_plan.SetRegisterKind(eRegisterKindGeneric);

  uint32_t pc_reg_num = LLDB_REGNUM_GENERIC_PC;
  uint32_t sp_reg_num = LLDB_REGNUM_GENERIC_SP;
  uint32_t ra_reg_num = LLDB_REGNUM_GENERIC_RA;

  UnwindPlan::RowSP row(new UnwindPlan::Row);

  // Define CFA as the stack pointer
  row->GetCFAValue().SetIsRegisterPlusOffset(sp_reg_num, 0);

  // Previous frames pc is in ra
  row->SetRegisterLocationToRegister(pc_reg_num, ra_reg_num, true);

  unwind_plan.AppendRow(row);
  unwind_plan.SetSourceName("riscv function-entry unwind plan");
  unwind_plan.SetSourcedFromCompiler(eLazyBoolNo);
  return true;
}

bool ABISysV_riscv::CreateDefaultUnwindPlan(UnwindPlan &unwind_plan) {
  unwind_plan.Clear();
  unwind_plan.SetRegisterKind(eRegisterKindGeneric);

  uint32_t pc_reg_num = LLDB_REGNUM_GENERIC_PC;
  uint32_t sp_reg_num = LLDB_REGNUM_GENERIC_SP;
  uint32_t ra_reg_num = LLDB_REGNUM_GENERIC_RA;

  UnwindPlan::RowSP row(new UnwindPlan::Row);

  // Define the CFA as the current stack pointer.
  row->GetCFAValue().SetIsRegisterPlusOffset(sp_reg_num, 0);
  row->SetOffset(0);

  // The previous frames pc is stored in ra.
  row->SetRegisterLocationToRegister(pc_reg_num, ra_reg_num, true);

  unwind_plan.AppendRow(row);
  unwind_plan.SetSourceName("riscv default unwind plan");
  unwind_plan.SetSourcedFromCompiler(eLazyBoolNo);
  unwind_plan.SetUnwindPlanValidAtAllInstructions(eLazyBoolNo);
  return true;
}

bool ABISysV_riscv::RegisterIsVolatile(
    const lldb_private::RegisterInfo *reg_info) {
  return !RegisterIsCalleeSaved(reg_info);
}

// See "Register Convention" in the RISC-V psABI documentation, which is
// maintained at https://github.com/riscv/riscv-elf-psabi-doc
bool ABISysV_riscv::RegisterIsCalleeSaved(
    const lldb_private::RegisterInfo *reg_info) {
  if (!reg_info)
    return false;

  bool IsCalleeSaved =
      llvm::StringSwitch<bool>(reg_info->name)
          .Cases("x1", "x2", "x8", "x9", "x18", "x19", "x20", "x21", true)
          .Cases("x22", "x23", "x24", "x25", "x26", "x27", true)
          .Cases("f8", "f9", "f18", "f19", "f20", "f21", IsHardFloatProcess())
          .Cases("f22", "f23", "f24", "f25", "f26", "f27", IsHardFloatProcess())
          .Default(false);
  return IsCalleeSaved;
}

uint32_t ABISysV_riscv::GetGenericNum(llvm::StringRef name) {
  return llvm::StringSwitch<uint32_t>(name)
      .Case("pc", LLDB_REGNUM_GENERIC_PC)
      .Case("ra", LLDB_REGNUM_GENERIC_RA)
      .Case("sp", LLDB_REGNUM_GENERIC_SP)
      .Case("fp", LLDB_REGNUM_GENERIC_FP)
      .Case("a0", LLDB_REGNUM_GENERIC_ARG1)
      .Case("a1", LLDB_REGNUM_GENERIC_ARG2)
      .Case("a2", LLDB_REGNUM_GENERIC_ARG3)
      .Case("a3", LLDB_REGNUM_GENERIC_ARG4)
      .Case("a4", LLDB_REGNUM_GENERIC_ARG5)
      .Case("a5", LLDB_REGNUM_GENERIC_ARG6)
      .Case("a6", LLDB_REGNUM_GENERIC_ARG7)
      .Case("a7", LLDB_REGNUM_GENERIC_ARG8)
      .Default(LLDB_INVALID_REGNUM);
}

bool ABISysV_riscv::IsHardFloatProcess() const {
  bool is_hardfloat = false;
  ProcessSP process_sp(GetProcessSP());
  if (process_sp) {
    const ArchSpec &arch(process_sp->GetTarget().GetArchitecture());
    if (arch.GetFlags() & ArchSpec::eRISCV_abi_f ||
        arch.GetFlags() & ArchSpec::eRISCV_abi_d)
      is_hardfloat = true;
  }
  return is_hardfloat;
}

ABISP
ABISysV_riscv::CreateInstance(lldb::ProcessSP process_sp,
                              const ArchSpec &arch) {
  if (arch.GetTriple().getArch() == llvm::Triple::riscv32 ||
      arch.GetTriple().getArch() == llvm::Triple::riscv64) {
    return ABISP(
        new ABISysV_riscv(std::move(process_sp), MakeMCRegisterInfo(arch),
                          arch.GetTriple().getArch() == llvm::Triple::riscv64));
  }
  return ABISP();
}

void ABISysV_riscv::Initialize() {
  PluginManager::RegisterPlugin(
      GetPluginNameStatic(), "System V ABI for riscv targets", CreateInstance);
}

void ABISysV_riscv::Terminate() {
  PluginManager::UnregisterPlugin(CreateInstance);
}

// PluginInterface protocol

lldb_private::ConstString ABISysV_riscv::GetPluginNameStatic() {
  static ConstString g_name("sysv-riscv");
  return g_name;
}

lldb_private::ConstString ABISysV_riscv::GetPluginName() {
  return GetPluginNameStatic();
}
