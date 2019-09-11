//===-- RISCVDisassembler.cpp - Disassembler for RISCV --------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file implements the RISCVDisassembler class.
//
//===----------------------------------------------------------------------===//

#include "MCTargetDesc/RISCVMCTargetDesc.h"
#include "TargetInfo/RISCVTargetInfo.h"
#include "Utils/RISCVBaseInfo.h"
#include "llvm/CodeGen/Register.h"
#include "llvm/MC/MCContext.h"
#include "llvm/MC/MCDisassembler/MCDisassembler.h"
#include "llvm/MC/MCFixedLenDisassembler.h"
#include "llvm/MC/MCInst.h"
#include "llvm/MC/MCRegisterInfo.h"
#include "llvm/MC/MCSubtargetInfo.h"
#include "llvm/Support/Endian.h"
#include "llvm/Support/TargetRegistry.h"

using namespace llvm;

#define DEBUG_TYPE "riscv-disassembler"

typedef MCDisassembler::DecodeStatus DecodeStatus;

namespace {
class RISCVDisassembler : public MCDisassembler {

public:
  RISCVDisassembler(const MCSubtargetInfo &STI, MCContext &Ctx)
      : MCDisassembler(STI, Ctx) {}

  DecodeStatus getInstruction(MCInst &Instr, uint64_t &Size,
                              ArrayRef<uint8_t> Bytes, uint64_t Address,
                              raw_ostream &VStream,
                              raw_ostream &CStream) const override;
};
} // end anonymous namespace

static MCDisassembler *createRISCVDisassembler(const Target &T,
                                               const MCSubtargetInfo &STI,
                                               MCContext &Ctx) {
  return new RISCVDisassembler(STI, Ctx);
}

extern "C" void LLVMInitializeRISCVDisassembler() {
  // Register the disassembler for each target.
  TargetRegistry::RegisterMCDisassembler(getTheRISCV32Target(),
                                         createRISCVDisassembler);
  TargetRegistry::RegisterMCDisassembler(getTheRISCV64Target(),
                                         createRISCVDisassembler);
}

static const Register GPRDecoderTable[] = {
  RISCV::X0,  RISCV::X1,  RISCV::X2,  RISCV::X3,
  RISCV::X4,  RISCV::X5,  RISCV::X6,  RISCV::X7,
  RISCV::X8,  RISCV::X9,  RISCV::X10, RISCV::X11,
  RISCV::X12, RISCV::X13, RISCV::X14, RISCV::X15,
  RISCV::X16, RISCV::X17, RISCV::X18, RISCV::X19,
  RISCV::X20, RISCV::X21, RISCV::X22, RISCV::X23,
  RISCV::X24, RISCV::X25, RISCV::X26, RISCV::X27,
  RISCV::X28, RISCV::X29, RISCV::X30, RISCV::X31
};

static DecodeStatus DecodeGPRRegisterClass(MCInst &Inst, uint64_t RegNo,
                                           uint64_t Address,
                                           const void *Decoder) {
  const FeatureBitset &FeatureBits =
      static_cast<const MCDisassembler *>(Decoder)
          ->getSubtargetInfo()
          .getFeatureBits();
  bool IsRV32E = FeatureBits[RISCV::FeatureRV32E];

  if (RegNo > array_lengthof(GPRDecoderTable) || (IsRV32E && RegNo > 15))
    return MCDisassembler::Fail;

  // We must define our own mapping from RegNo to register identifier.
  // Accessing index RegNo in the register class will work in the case that
  // registers were added in ascending order, but not in general.
  Register Reg = GPRDecoderTable[RegNo];
  Inst.addOperand(MCOperand::createReg(Reg));
  return MCDisassembler::Success;
}

static const Register FPR32DecoderTable[] = {
  RISCV::F0,  RISCV::F1,  RISCV::F2,  RISCV::F3,
  RISCV::F4,  RISCV::F5,  RISCV::F6,  RISCV::F7,
  RISCV::F8,  RISCV::F9,  RISCV::F10, RISCV::F11,
  RISCV::F12, RISCV::F13, RISCV::F14, RISCV::F15,
  RISCV::F16, RISCV::F17, RISCV::F18, RISCV::F19,
  RISCV::F20, RISCV::F21, RISCV::F22, RISCV::F23,
  RISCV::F24, RISCV::F25, RISCV::F26, RISCV::F27,
  RISCV::F28, RISCV::F29, RISCV::F30, RISCV::F31
};

static DecodeStatus DecodeFPR32RegisterClass(MCInst &Inst, uint64_t RegNo,
                                             uint64_t Address,
                                             const void *Decoder) {
  if (RegNo > array_lengthof(FPR32DecoderTable))
    return MCDisassembler::Fail;

  // We must define our own mapping from RegNo to register identifier.
  // Accessing index RegNo in the register class will work in the case that
  // registers were added in ascending order, but not in general.
  Register Reg = FPR32DecoderTable[RegNo];
  Inst.addOperand(MCOperand::createReg(Reg));
  return MCDisassembler::Success;
}

static DecodeStatus DecodeFPR32CRegisterClass(MCInst &Inst, uint64_t RegNo,
                                              uint64_t Address,
                                              const void *Decoder) {
  if (RegNo > 8) {
    return MCDisassembler::Fail;
  }
  Register Reg = FPR32DecoderTable[RegNo + 8];
  Inst.addOperand(MCOperand::createReg(Reg));
  return MCDisassembler::Success;
}

static const Register FPR64DecoderTable[] = {
  RISCV::D0,  RISCV::D1,  RISCV::D2,  RISCV::D3,
  RISCV::D4,  RISCV::D5,  RISCV::D6,  RISCV::D7,
  RISCV::D8,  RISCV::D9,  RISCV::D10, RISCV::D11,
  RISCV::D12, RISCV::D13, RISCV::D14, RISCV::D15,
  RISCV::D16, RISCV::D17, RISCV::D18, RISCV::D19,
  RISCV::D20, RISCV::D21, RISCV::D22, RISCV::D23,
  RISCV::D24, RISCV::D25, RISCV::D26, RISCV::D27,
  RISCV::D28, RISCV::D29, RISCV::D30, RISCV::D31
};

static DecodeStatus DecodeFPR64RegisterClass(MCInst &Inst, uint64_t RegNo,
                                             uint64_t Address,
                                             const void *Decoder) {
  if (RegNo > array_lengthof(FPR64DecoderTable))
    return MCDisassembler::Fail;

  // We must define our own mapping from RegNo to register identifier.
  // Accessing index RegNo in the register class will work in the case that
  // registers were added in ascending order, but not in general.
  Register Reg = FPR64DecoderTable[RegNo];
  Inst.addOperand(MCOperand::createReg(Reg));
  return MCDisassembler::Success;
}

static DecodeStatus DecodeFPR64CRegisterClass(MCInst &Inst, uint64_t RegNo,
                                              uint64_t Address,
                                              const void *Decoder) {
  if (RegNo > 8) {
    return MCDisassembler::Fail;
  }
  Register Reg = FPR64DecoderTable[RegNo + 8];
  Inst.addOperand(MCOperand::createReg(Reg));
  return MCDisassembler::Success;
}

static DecodeStatus DecodeGPRNoX0RegisterClass(MCInst &Inst, uint64_t RegNo,
                                               uint64_t Address,
                                               const void *Decoder) {
  if (RegNo == 0) {
    return MCDisassembler::Fail;
  }

  return DecodeGPRRegisterClass(Inst, RegNo, Address, Decoder);
}

static DecodeStatus DecodeGPRNoX0X2RegisterClass(MCInst &Inst, uint64_t RegNo,
                                                 uint64_t Address,
                                                 const void *Decoder) {
  if (RegNo == 2) {
    return MCDisassembler::Fail;
  }

  return DecodeGPRNoX0RegisterClass(Inst, RegNo, Address, Decoder);
}

static DecodeStatus DecodeGPRCRegisterClass(MCInst &Inst, uint64_t RegNo,
                                            uint64_t Address,
                                            const void *Decoder) {
  if (RegNo > 8)
    return MCDisassembler::Fail;

  Register Reg = GPRDecoderTable[RegNo + 8];
  Inst.addOperand(MCOperand::createReg(Reg));
  return MCDisassembler::Success;
}

// Add implied SP operand for instructions *SP compressed instructions. The SP
// operand isn't explicitly encoded in the instruction.
static void addImplySP(MCInst &Inst, int64_t Address, const void *Decoder) {
  if (Inst.getOpcode() == RISCV::C_LWSP || Inst.getOpcode() == RISCV::C_SWSP ||
      Inst.getOpcode() == RISCV::C_LDSP || Inst.getOpcode() == RISCV::C_SDSP ||
      Inst.getOpcode() == RISCV::C_FLWSP ||
      Inst.getOpcode() == RISCV::C_FSWSP ||
      Inst.getOpcode() == RISCV::C_FLDSP ||
      Inst.getOpcode() == RISCV::C_FSDSP ||
      Inst.getOpcode() == RISCV::C_ADDI4SPN) {
    DecodeGPRRegisterClass(Inst, 2, Address, Decoder);
  }
  if (Inst.getOpcode() == RISCV::C_ADDI16SP) {
    DecodeGPRRegisterClass(Inst, 2, Address, Decoder);
    DecodeGPRRegisterClass(Inst, 2, Address, Decoder);
  }
}

template <unsigned N>
static DecodeStatus decodeUImmOperand(MCInst &Inst, uint64_t Imm,
                                      int64_t Address, const void *Decoder) {
  assert(isUInt<N>(Imm) && "Invalid immediate");
  addImplySP(Inst, Address, Decoder);
  Inst.addOperand(MCOperand::createImm(Imm));
  return MCDisassembler::Success;
}

template <unsigned N>
static DecodeStatus decodeUImmNonZeroOperand(MCInst &Inst, uint64_t Imm,
                                             int64_t Address,
                                             const void *Decoder) {
  if (Imm == 0)
    return MCDisassembler::Fail;
  return decodeUImmOperand<N>(Inst, Imm, Address, Decoder);
}

template <unsigned N>
static DecodeStatus decodeSImmOperand(MCInst &Inst, uint64_t Imm,
                                      int64_t Address, const void *Decoder) {
  assert(isUInt<N>(Imm) && "Invalid immediate");
  addImplySP(Inst, Address, Decoder);
  // Sign-extend the number in the bottom N bits of Imm
  Inst.addOperand(MCOperand::createImm(SignExtend64<N>(Imm)));
  return MCDisassembler::Success;
}

template <unsigned N>
static DecodeStatus decodeSImmNonZeroOperand(MCInst &Inst, uint64_t Imm,
                                             int64_t Address,
                                             const void *Decoder) {
  if (Imm == 0)
    return MCDisassembler::Fail;
  return decodeSImmOperand<N>(Inst, Imm, Address, Decoder);
}

template <unsigned N>
static DecodeStatus decodeSImmOperandAndLsl1(MCInst &Inst, uint64_t Imm,
                                             int64_t Address,
                                             const void *Decoder) {
  assert(isUInt<N>(Imm) && "Invalid immediate");
  // Sign-extend the number in the bottom N bits of Imm after accounting for
  // the fact that the N bit immediate is stored in N-1 bits (the LSB is
  // always zero)
  Inst.addOperand(MCOperand::createImm(SignExtend64<N>(Imm << 1)));
  return MCDisassembler::Success;
}

static DecodeStatus decodeCLUIImmOperand(MCInst &Inst, uint64_t Imm,
                                         int64_t Address,
                                         const void *Decoder) {
  assert(isUInt<6>(Imm) && "Invalid immediate");
  if (Imm > 31) {
    Imm = (SignExtend64<6>(Imm) & 0xfffff);
  }
  Inst.addOperand(MCOperand::createImm(Imm));
  return MCDisassembler::Success;
}

static DecodeStatus decodeFRMArg(MCInst &Inst, uint64_t Imm,
                                 int64_t Address,
                                 const void *Decoder) {
  assert(isUInt<3>(Imm) && "Invalid immediate");
  if (!llvm::RISCVFPRndMode::isValidRoundingMode(Imm))
    return MCDisassembler::Fail;

  Inst.addOperand(MCOperand::createImm(Imm));
  return MCDisassembler::Success;
}

static DecodeStatus decodeRVCInstrSImm(MCInst &Inst, unsigned Insn,
                                       uint64_t Address, const void *Decoder);

static DecodeStatus decodeRVCInstrRdSImm(MCInst &Inst, unsigned Insn,
                                         uint64_t Address, const void *Decoder);

static DecodeStatus decodeRVCInstrRdRs1UImm(MCInst &Inst, unsigned Insn,
                                            uint64_t Address,
                                            const void *Decoder);

static DecodeStatus decodeRVCInstrRdRs2(MCInst &Inst, unsigned Insn,
                                        uint64_t Address, const void *Decoder);

static DecodeStatus decodeRVCInstrRdRs1Rs2(MCInst &Inst, unsigned Insn,
                                           uint64_t Address,
                                           const void *Decoder);

#include "RISCVGenDisassemblerTables.inc"

static DecodeStatus decodeRVCInstrSImm(MCInst &Inst, unsigned Insn,
                                       uint64_t Address, const void *Decoder) {
  uint64_t SImm6 =
      fieldFromInstruction(Insn, 12, 1) << 5 | fieldFromInstruction(Insn, 2, 5);
  DecodeStatus Result = decodeSImmOperand<6>(Inst, SImm6, Address, Decoder);
  (void)Result;
  assert(Result == MCDisassembler::Success && "Invalid immediate");
  return MCDisassembler::Success;
}

static DecodeStatus decodeRVCInstrRdSImm(MCInst &Inst, unsigned Insn,
                                         uint64_t Address,
                                         const void *Decoder) {
  DecodeGPRRegisterClass(Inst, 0, Address, Decoder);
  uint64_t SImm6 =
      fieldFromInstruction(Insn, 12, 1) << 5 | fieldFromInstruction(Insn, 2, 5);
  DecodeStatus Result = decodeSImmOperand<6>(Inst, SImm6, Address, Decoder);
  (void)Result;
  assert(Result == MCDisassembler::Success && "Invalid immediate");
  return MCDisassembler::Success;
}

static DecodeStatus decodeRVCInstrRdRs1UImm(MCInst &Inst, unsigned Insn,
                                            uint64_t Address,
                                            const void *Decoder) {
  DecodeGPRRegisterClass(Inst, 0, Address, Decoder);
  Inst.addOperand(Inst.getOperand(0));
  uint64_t UImm6 =
      fieldFromInstruction(Insn, 12, 1) << 5 | fieldFromInstruction(Insn, 2, 5);
  DecodeStatus Result = decodeUImmOperand<6>(Inst, UImm6, Address, Decoder);
  (void)Result;
  assert(Result == MCDisassembler::Success && "Invalid immediate");
  return MCDisassembler::Success;
}

static DecodeStatus decodeRVCInstrRdRs2(MCInst &Inst, unsigned Insn,
                                        uint64_t Address, const void *Decoder) {
  unsigned Rd = fieldFromInstruction(Insn, 7, 5);
  unsigned Rs2 = fieldFromInstruction(Insn, 2, 5);
  DecodeGPRRegisterClass(Inst, Rd, Address, Decoder);
  DecodeGPRRegisterClass(Inst, Rs2, Address, Decoder);
  return MCDisassembler::Success;
}

static DecodeStatus decodeRVCInstrRdRs1Rs2(MCInst &Inst, unsigned Insn,
                                           uint64_t Address,
                                           const void *Decoder) {
  unsigned Rd = fieldFromInstruction(Insn, 7, 5);
  unsigned Rs2 = fieldFromInstruction(Insn, 2, 5);
  DecodeGPRRegisterClass(Inst, Rd, Address, Decoder);
  Inst.addOperand(Inst.getOperand(0));
  DecodeGPRRegisterClass(Inst, Rs2, Address, Decoder);
  return MCDisassembler::Success;
}

DecodeStatus RISCVDisassembler::getInstruction(MCInst &MI, uint64_t &Size,
                                               ArrayRef<uint8_t> Bytes,
                                               uint64_t Address,
                                               raw_ostream &OS,
                                               raw_ostream &CS) const {
  // TODO: This will need modification when supporting instruction set
  // extensions with instructions > 32-bits (up to 176 bits wide).
  uint32_t Insn;
  DecodeStatus Result;

  // It's a 32 bit instruction if bit 0 and 1 are 1.
  if ((Bytes[0] & 0x3) == 0x3) {
    if (Bytes.size() < 4) {
      Size = 0;
      return MCDisassembler::Fail;
    }
    Insn = support::endian::read32le(Bytes.data());
    LLVM_DEBUG(dbgs() << "Trying RISCV32 table :\n");
    Result = decodeInstruction(DecoderTable32, MI, Insn, Address, this, STI);
    Size = 4;
  } else {
    if (Bytes.size() < 2) {
      Size = 0;
      return MCDisassembler::Fail;
    }
    Insn = support::endian::read16le(Bytes.data());

    if (!STI.getFeatureBits()[RISCV::Feature64Bit]) {
      LLVM_DEBUG(
          dbgs() << "Trying RISCV32Only_16 table (16-bit Instruction):\n");
      // Calling the auto-generated decoder function.
      Result = decodeInstruction(DecoderTableRISCV32Only_16, MI, Insn, Address,
                                 this, STI);
      if (Result != MCDisassembler::Fail) {
        Size = 2;
        return Result;
      }
    }

    LLVM_DEBUG(dbgs() << "Trying RISCV_C table (16-bit Instruction):\n");
    // Calling the auto-generated decoder function.
    Result = decodeInstruction(DecoderTable16, MI, Insn, Address, this, STI);
    Size = 2;
  }

  return Result;
}
