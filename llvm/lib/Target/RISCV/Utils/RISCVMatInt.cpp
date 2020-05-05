//===- RISCVMatInt.cpp - Immediate materialisation -------------*- C++ -*--===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "RISCVMatInt.h"
#include "MCTargetDesc/RISCVMCTargetDesc.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/Support/MachineValueType.h"
#include "llvm/Support/MathExtras.h"
#include <cstdint>

namespace llvm {

namespace RISCVMatInt {
static int getInstSeqCost(InstSeq &Res, bool OptSize) {
    int Cost = 0;
    for(auto Instr : Res) {
        bool Compressed;
        switch(Instr.Opc) {
        case RISCV::SLLI:
        case RISCV::SRLI:
            Compressed = true;
            break;
        case RISCV::ADDI:
        case RISCV::ADDIW:
        case RISCV::LUI:
            Compressed = isInt<6>(Instr.Imm);
            break;
        default:
            Compressed = false;
            break;
        }
        // If we are optimizing for speed we consider two RVC instructions more
        // costly than one RVI instruction, even if the size is the same.
        if (Compressed)
          Cost += 2;
        else
          Cost += OptSize ? 4 : 3;
    }
    return Cost;
}

static void generateInstSeq32(int32_t Val, InstSeq &Res, bool OptSize) {
  int32_t Hi20 = ((Val + 0x800) >> 12) & 0xFFFFF;
  int32_t Lo12 = SignExtend32<12>(Val);

  // Does the general case require LUI+ADDI, and they aren't both compressed?
  // If so, first try to materialize the constant using shifts.
  if (Hi20 && Lo12 && (!isInt<6>(Hi20) || !isInt<6>(Lo12))) {
    unsigned FirstSet = findFirstSet((uint32_t)Val);

    // Try XXX0* -> ADDI(XXX)+SLLI.
    int ShiftAmount = FirstSet;
    if (ShiftAmount > 0) {
      int32_t AltVal = SignExtend32(Val >> ShiftAmount, 32 - ShiftAmount);
      if (isInt<12>(AltVal)) {
        Res.push_back(Inst(RISCV::ADDI, AltVal));
        Res.push_back(Inst(RISCV::SLLI, ShiftAmount));
        return;
      }
    }

    if (Val > 0) {
      // Try 0*XXX -> ADDI(XXX1*)+SRLI.
      int ShiftAmount = countLeadingZeros((uint32_t)Val);
      int32_t AltVal = (Val << ShiftAmount) | ((1 << ShiftAmount) - 1);
      if (isInt<12>(AltVal)) {
        Res.push_back(Inst(RISCV::ADDI, AltVal));
        Res.push_back(Inst(RISCV::SRLI, ShiftAmount));
        return;
      }

      // Try 0*XXX -> LUI(XXX)+SRLI.
      if (isUInt<20>(Val >> FirstSet)) {
        AltVal = uint32_t(Val << ShiftAmount) >> 12;
        Res.push_back(Inst(RISCV::LUI, AltVal));
        Res.push_back(Inst(RISCV::SRLI, ShiftAmount));
        return;
      }
    }
  }

  // Depending on the active bits in the constant, the following
  // instruction sequences are emitted in the general case:
  //
  // Val == 0                          : ADDI
  // Val[0,12) != 0 && Val[12,32) == 0 : ADDI
  // Val[0,12) == 0 && Val[12,32) != 0 : LUI
  // Val[0,32) != 0                    : LUI+ADDI(W)

  if (Hi20)
    Res.push_back(Inst(RISCV::LUI, Hi20));

  if (Lo12 || Hi20 == 0)
    Res.push_back(Inst(RISCV::ADDI, Lo12));
}

static void generateInstSeq64Base(int64_t Val, InstSeq &Res, bool OptSize) {
  // In the worst case, for a full 64-bit constant, a sequence of 8 instructions
  // (i.e., LUI+ADDIW+SLLI+ADDI+SLLI+ADDI+SLLI+ADDI) has to be emmitted. Note
  // that the first two instructions (LUI+ADDIW) can contribute up to 32 bits
  // while the following ADDI instructions contribute up to 12 bits each.
  //
  // On the first glance, implementing this seems to be possible by simply
  // emitting the most significant 32 bits (LUI+ADDIW) followed by as many left
  // shift (SLLI) and immediate additions (ADDI) as needed. However, due to the
  // fact that ADDI performs a sign extended addition, doing it like that would
  // only be possible when at most 11 bits of the ADDI instructions are used.
  // Using all 12 bits of the ADDI instructions, like done by GAS, actually
  // requires that the constant is processed starting with the least significant
  // bit.
  //
  // In the following, constants are processed from LSB to MSB but instruction
  // emission is performed from MSB to LSB by recursively calling
  // generateInstSeq. In each recursion, first the lowest 12 bits are removed
  // from the constant and the optimal shift amount, which can be greater than
  // 12 bits if the constant is sparse, is determined. Then, the shifted
  // remaining constant is processed recursively and gets emitted as soon as it
  // fits into 32 bits. The emission of the shifts and additions is subsequently
  // performed when the recursion returns.

  int64_t Lo12 = SignExtend64<12>(Val);

  if (isInt<32>(Val)) {
    // Depending on the active bits in the immediate Value v, the following
    // instruction sequences are emitted:
    //
    // v == 0                        : ADDI
    // v[0,12) != 0 && v[12,32) == 0 : ADDI
    // v[0,12) == 0 && v[12,32) != 0 : LUI
    // v[0,32) != 0                  : LUI+ADDI(W)
    int64_t Hi20 = ((Val + 0x800) >> 12) & 0xFFFFF;

    if (Hi20)
      Res.push_back(Inst(RISCV::LUI, Hi20));

    if (Lo12 || Hi20 == 0) {
      unsigned AddiOpc = Hi20 ? RISCV::ADDIW : RISCV::ADDI;
      Res.push_back(Inst(AddiOpc, Lo12));
    }

    return;
  }

  int64_t Hi52 = ((uint64_t)Val + 0x800ull) >> 12;
  int ShiftAmount = 12 + findFirstSet((uint64_t)Hi52);
  Hi52 = SignExtend64(Hi52 >> (ShiftAmount - 12), 64 - ShiftAmount);

  generateInstSeq64Base(Hi52, Res, OptSize);

  Res.push_back(Inst(RISCV::SLLI, ShiftAmount));
  if (Lo12)
    Res.push_back(Inst(RISCV::ADDI, Lo12));
}

static void generateInstSeq64(int64_t Val, InstSeq &Res, bool OptSize) {
  generateInstSeq64Base(Val, Res, OptSize);

  int64_t Lo12 = SignExtend64<12>(Val);
  int64_t Hi52 = ((uint64_t)Val + 0x800ull) >> 12;
  int ShiftAmount = 12 + findFirstSet((uint64_t)Hi52);

  // Now that we handled the general case, let's check if we can improve on it
  // in various ways.

  if (Res.size() == 1)
    return;

  // Try InstSeq(Val) -> InstSeq(Val - Lo12)+ADDI(Lo12).
  if (Lo12 != 0) {
    int64_t AltVal = Val - Lo12;
    InstSeq AltRes;
    generateInstSeq64(AltVal, AltRes, OptSize);
    AltRes.push_back(Inst(RISCV::ADDI, Lo12));
    if (getInstSeqCost(AltRes, OptSize) < getInstSeqCost(Res, OptSize)) {
      Res = AltRes;
    }
  }

  // Try InstSeq(XXX0*) -> InstSeq(XXX)+SLLI.
  if ((Val & 1) == 0) {
    ShiftAmount = findFirstSet((uint64_t)Val);
    int64_t AltVal = SignExtend64(Val >> ShiftAmount, 64 - ShiftAmount);
    if (isInt<20>(AltVal) && !isInt<12>(AltVal) && ShiftAmount > 12) {
      AltVal = AltVal << 12;
      ShiftAmount -= 12;
    }
    InstSeq AltRes;
    generateInstSeq64(AltVal, AltRes, OptSize);
    AltRes.push_back(Inst(RISCV::SLLI, ShiftAmount));
    if (getInstSeqCost(AltRes, OptSize) < getInstSeqCost(Res, OptSize)) {
      Res = AltRes;
    }
  }

  if (Val > 0) {
    // Try InstSeq(0*XXX) -> InstSeq(XXX1*)+SRLI.
    ShiftAmount = countLeadingZeros((uint64_t)Val);
    int64_t AltVal = (Val << ShiftAmount) | ((1L << ShiftAmount) - 1);
    InstSeq AltRes;
    generateInstSeq64(AltVal, AltRes, OptSize);
    AltRes.push_back(Inst(RISCV::SRLI, ShiftAmount));
    if (getInstSeqCost(AltRes, OptSize) < getInstSeqCost(Res, OptSize)) {
      Res = AltRes;
    }

    // Try InstSeq(0*XXX) -> InstSeq(XXX0*)+SRLI.
    AltVal = (Val << ShiftAmount);
    AltRes.clear();
    generateInstSeq64(AltVal, AltRes, OptSize);
    AltRes.push_back(Inst(RISCV::SRLI, ShiftAmount));
    if (getInstSeqCost(AltRes, OptSize) < getInstSeqCost(Res, OptSize)) {
      Res = AltRes;
    }
  }
}

void generateInstSeq(int64_t Val, InstSeq &Res, bool IsRV64, bool OptSize) {
  if (!IsRV64) {
    assert(isInt<32>(Val));
    generateInstSeq32(Val, Res, OptSize);
  }
  else
    generateInstSeq64(Val, Res, OptSize);
}

int getIntMatCost(const APInt &Val, unsigned Size, bool IsRV64, bool OptSize) {
  int PlatRegSize = IsRV64 ? 64 : 32;

  // Split the constant into platform register sized chunks, and calculate cost
  // of each chunk.
  int Cost = 0;
  for (unsigned ShiftVal = 0; ShiftVal < Size; ShiftVal += PlatRegSize) {
    APInt Chunk = Val.ashr(ShiftVal).sextOrTrunc(PlatRegSize);
    InstSeq MatSeq;
    generateInstSeq(Chunk.getSExtValue(), MatSeq, IsRV64, OptSize);
    Cost += getInstSeqCost(MatSeq, OptSize);
  }
  return std::max(1, Cost);
}
} // namespace RISCVMatInt
} // namespace llvm
