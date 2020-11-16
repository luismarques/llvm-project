//===-- ABISysV_riscv.h -----------------------------------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef liblldb_ABISysV_riscv_h_
#define liblldb_ABISysV_riscv_h_

#include "lldb/Target/ABI.h"
#include "lldb/lldb-private.h"

class ABISysV_riscv : public lldb_private::MCBasedABI {
  bool isRV64;

public:
  ~ABISysV_riscv() override = default;

  size_t GetRedZoneSize() const override { return 0; }

  bool PrepareTrivialCall(lldb_private::Thread &thread, lldb::addr_t sp,
                          lldb::addr_t functionAddress,
                          lldb::addr_t returnAddress,
                          llvm::ArrayRef<lldb::addr_t> args) const override {
    // TODO: Implement
    return false;
  }

  bool GetArgumentValues(lldb_private::Thread &thread,
                         lldb_private::ValueList &values) const override {
    // TODO: Implement
    return false;
  }

  lldb_private::Status
  SetReturnValueObject(lldb::StackFrameSP &frame_sp,
                       lldb::ValueObjectSP &new_value) override {
    // TODO: Implement
    lldb_private::Status error;
    error.SetErrorString("Not yet implemented");
    return error;
  }

  lldb::ValueObjectSP
  GetReturnValueObjectImpl(lldb_private::Thread &thread,
                           lldb_private::CompilerType &type) const override {
    // TODO: Implement
    lldb::ValueObjectSP return_valobj;
    return return_valobj;
  }

  bool
  CreateFunctionEntryUnwindPlan(lldb_private::UnwindPlan &unwind_plan) override;

  bool CreateDefaultUnwindPlan(lldb_private::UnwindPlan &unwind_plan) override;

  bool RegisterIsVolatile(const lldb_private::RegisterInfo *reg_info) override;

  bool CallFrameAddressIsValid(lldb::addr_t cfa) override {
    // Assume any address except zero is valid
    if (cfa == 0)
      return false;
    return true;
  }

  bool CodeAddressIsValid(lldb::addr_t pc) override {
    // Ensure addresses are smaller than XLEN bits wide. Calls can use the least
    // significant bit to store auxiliary information, so no strict check is
    // done for alignment.
    if (!isRV64)
      return (pc <= UINT32_MAX);
    return (pc <= UINT64_MAX);
  }

  lldb::addr_t FixCodeAddress(lldb::addr_t pc) override {
    // Since the least significant bit of a code address can be used to store
    // auxiliary information, that bit must be zeroed in any addresses.
    return pc & ~(lldb::addr_t)1;
  }

  // Static Functions

  static void Initialize();

  static void Terminate();

  static lldb::ABISP CreateInstance(lldb::ProcessSP process_sp,
                                    const lldb_private::ArchSpec &arch);

  // PluginInterface protocol

  static lldb_private::ConstString GetPluginNameStatic();

  lldb_private::ConstString GetPluginName() override;

  uint32_t GetPluginVersion() override { return 1; }

protected:
  bool RegisterIsCalleeSaved(const lldb_private::RegisterInfo *reg_info);

  uint32_t GetGenericNum(llvm::StringRef reg) override;

  bool IsHardFloatProcess() const;

private:
  ABISysV_riscv(lldb::ProcessSP process_sp,
                std::unique_ptr<llvm::MCRegisterInfo> info_up, bool _isRV64)
      : lldb_private::MCBasedABI(std::move(process_sp), std::move(info_up)),
        isRV64(_isRV64) {
    // Call CreateInstance instead.
  }
};

#endif // liblldb_ABISysV_riscv_h_
