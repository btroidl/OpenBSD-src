//===-- X86Subtarget.h - Define Subtarget for the X86 ----------*- C++ -*--===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file declares the X86 specific subclass of TargetSubtargetInfo.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_X86_X86SUBTARGET_H
#define LLVM_LIB_TARGET_X86_X86SUBTARGET_H

#include "X86FrameLowering.h"
#include "X86ISelLowering.h"
#include "X86InstrInfo.h"
#include "X86SelectionDAGInfo.h"
#include "llvm/ADT/Triple.h"
#include "llvm/CodeGen/TargetSubtargetInfo.h"
#include "llvm/IR/CallingConv.h"
#include <climits>
#include <memory>

#define GET_SUBTARGETINFO_HEADER
#include "X86GenSubtargetInfo.inc"

namespace llvm {

class CallLowering;
class GlobalValue;
class InstructionSelector;
class LegalizerInfo;
class RegisterBankInfo;
class StringRef;
class TargetMachine;

/// The X86 backend supports a number of different styles of PIC.
///
namespace PICStyles {

enum class Style {
  StubPIC,          // Used on i386-darwin in pic mode.
  GOT,              // Used on 32 bit elf on when in pic mode.
  RIPRel,           // Used on X86-64 when in pic mode.
  None              // Set when not in pic mode.
};

} // end namespace PICStyles

class X86Subtarget final : public X86GenSubtargetInfo {
  enum X86SSEEnum {
    NoSSE, SSE1, SSE2, SSE3, SSSE3, SSE41, SSE42, AVX, AVX2, AVX512
  };

  enum X863DNowEnum {
    NoThreeDNow, MMX, ThreeDNow, ThreeDNowA
  };

  /// Which PIC style to use
  PICStyles::Style PICStyle;

  const TargetMachine &TM;

  /// SSE1, SSE2, SSE3, SSSE3, SSE41, SSE42, or none supported.
  X86SSEEnum X86SSELevel = NoSSE;

  /// MMX, 3DNow, 3DNow Athlon, or none supported.
  X863DNowEnum X863DNowLevel = NoThreeDNow;

#define GET_SUBTARGETINFO_MACRO(ATTRIBUTE, DEFAULT, GETTER)                    \
  bool ATTRIBUTE = DEFAULT;
#include "X86GenSubtargetInfo.inc"
  /// The minimum alignment known to hold of the stack frame on
  /// entry to the function and which must be maintained by every function.
  Align stackAlignment = Align(4);

  Align TileConfigAlignment = Align(4);

  /// Max. memset / memcpy size that is turned into rep/movs, rep/stos ops.
  ///
  // FIXME: this is a known good value for Yonah. How about others?
  unsigned MaxInlineSizeThreshold = 128;

  /// What processor and OS we're targeting.
  Triple TargetTriple;

  /// GlobalISel related APIs.
  std::unique_ptr<CallLowering> CallLoweringInfo;
  std::unique_ptr<LegalizerInfo> Legalizer;
  std::unique_ptr<RegisterBankInfo> RegBankInfo;
  std::unique_ptr<InstructionSelector> InstSelector;

  /// Override the stack alignment.
  MaybeAlign StackAlignOverride;

  /// Preferred vector width from function attribute.
  unsigned PreferVectorWidthOverride;

  /// Resolved preferred vector width from function attribute and subtarget
  /// features.
  unsigned PreferVectorWidth = UINT32_MAX;

  /// Required vector width from function attribute.
  unsigned RequiredVectorWidth;

  X86SelectionDAGInfo TSInfo;
  // Ordering here is important. X86InstrInfo initializes X86RegisterInfo which
  // X86TargetLowering needs.
  X86InstrInfo InstrInfo;
  X86TargetLowering TLInfo;
  X86FrameLowering FrameLowering;

public:
  /// This constructor initializes the data members to match that
  /// of the specified triple.
  ///
  X86Subtarget(const Triple &TT, StringRef CPU, StringRef TuneCPU, StringRef FS,
               const X86TargetMachine &TM, MaybeAlign StackAlignOverride,
               unsigned PreferVectorWidthOverride,
               unsigned RequiredVectorWidth);

  const X86TargetLowering *getTargetLowering() const override {
    return &TLInfo;
  }

  const X86InstrInfo *getInstrInfo() const override { return &InstrInfo; }

  const X86FrameLowering *getFrameLowering() const override {
    return &FrameLowering;
  }

  const X86SelectionDAGInfo *getSelectionDAGInfo() const override {
    return &TSInfo;
  }

  const X86RegisterInfo *getRegisterInfo() const override {
    return &getInstrInfo()->getRegisterInfo();
  }

  bool getSaveArgs() const { return SaveArgs; }

  unsigned getTileConfigSize() const { return 64; }
  Align getTileConfigAlignment() const { return TileConfigAlignment; }

  /// Returns the minimum alignment known to hold of the
  /// stack frame on entry to the function and which must be maintained by every
  /// function for this subtarget.
  Align getStackAlignment() const { return stackAlignment; }

  /// Returns the maximum memset / memcpy size
  /// that still makes it profitable to inline the call.
  unsigned getMaxInlineSizeThreshold() const { return MaxInlineSizeThreshold; }

  /// ParseSubtargetFeatures - Parses features string setting specified
  /// subtarget options.  Definition of function is auto generated by tblgen.
  void ParseSubtargetFeatures(StringRef CPU, StringRef TuneCPU, StringRef FS);

  /// Methods used by Global ISel
  const CallLowering *getCallLowering() const override;
  InstructionSelector *getInstructionSelector() const override;
  const LegalizerInfo *getLegalizerInfo() const override;
  const RegisterBankInfo *getRegBankInfo() const override;

private:
  /// Initialize the full set of dependencies so we can use an initializer
  /// list for X86Subtarget.
  X86Subtarget &initializeSubtargetDependencies(StringRef CPU,
                                                StringRef TuneCPU,
                                                StringRef FS);
  void initSubtargetFeatures(StringRef CPU, StringRef TuneCPU, StringRef FS);

public:

#define GET_SUBTARGETINFO_MACRO(ATTRIBUTE, DEFAULT, GETTER)                    \
  bool GETTER() const { return ATTRIBUTE; }
#include "X86GenSubtargetInfo.inc"

  /// Is this x86_64 with the ILP32 programming model (x32 ABI)?
  bool isTarget64BitILP32() const {
    return Is64Bit && (TargetTriple.isX32() || TargetTriple.isOSNaCl());
  }

  /// Is this x86_64 with the LP64 programming model (standard AMD64, no x32)?
  bool isTarget64BitLP64() const {
    return Is64Bit && (!TargetTriple.isX32() && !TargetTriple.isOSNaCl());
  }

  PICStyles::Style getPICStyle() const { return PICStyle; }
  void setPICStyle(PICStyles::Style Style)  { PICStyle = Style; }

  bool canUseCMPXCHG8B() const { return hasCX8(); }
  bool canUseCMPXCHG16B() const {
    // CX16 is just the CPUID bit, instruction requires 64-bit mode too.
    return hasCX16() && is64Bit();
  }
  // SSE codegen depends on cmovs, and all SSE1+ processors support them.
  // All 64-bit processors support cmov.
  bool canUseCMOV() const { return hasCMOV() || hasSSE1() || is64Bit(); }
  bool hasSSE1() const { return X86SSELevel >= SSE1; }
  bool hasSSE2() const { return X86SSELevel >= SSE2; }
  bool hasSSE3() const { return X86SSELevel >= SSE3; }
  bool hasSSSE3() const { return X86SSELevel >= SSSE3; }
  bool hasSSE41() const { return X86SSELevel >= SSE41; }
  bool hasSSE42() const { return X86SSELevel >= SSE42; }
  bool hasAVX() const { return X86SSELevel >= AVX; }
  bool hasAVX2() const { return X86SSELevel >= AVX2; }
  bool hasAVX512() const { return X86SSELevel >= AVX512; }
  bool hasInt256() const { return hasAVX2(); }
  bool hasMMX() const { return X863DNowLevel >= MMX; }
  bool hasThreeDNow() const { return X863DNowLevel >= ThreeDNow; }
  bool hasThreeDNowA() const { return X863DNowLevel >= ThreeDNowA; }
  bool hasAnyFMA() const { return hasFMA() || hasFMA4(); }
  bool hasPrefetchW() const {
    // The PREFETCHW instruction was added with 3DNow but later CPUs gave it
    // its own CPUID bit as part of deprecating 3DNow. Intel eventually added
    // it and KNL has another that prefetches to L2 cache. We assume the
    // L1 version exists if the L2 version does.
    return hasThreeDNow() || hasPRFCHW() || hasPREFETCHWT1();
  }
  bool hasSSEPrefetch() const {
    // We implicitly enable these when we have a write prefix supporting cache
    // level OR if we have prfchw, but don't already have a read prefetch from
    // 3dnow.
    return hasSSE1() || (hasPRFCHW() && !hasThreeDNow()) || hasPREFETCHWT1() ||
           hasPREFETCHI();
  }
  bool canUseLAHFSAHF() const { return hasLAHFSAHF64() || !is64Bit(); }
  // These are generic getters that OR together all of the thunk types
  // supported by the subtarget. Therefore useIndirectThunk*() will return true
  // if any respective thunk feature is enabled.
  bool useIndirectThunkCalls() const {
    return useRetpolineIndirectCalls() || useLVIControlFlowIntegrity();
  }
  bool useIndirectThunkBranches() const {
    return useRetpolineIndirectBranches() || useLVIControlFlowIntegrity();
  }

  unsigned getPreferVectorWidth() const { return PreferVectorWidth; }
  unsigned getRequiredVectorWidth() const { return RequiredVectorWidth; }

  // Helper functions to determine when we should allow widening to 512-bit
  // during codegen.
  // TODO: Currently we're always allowing widening on CPUs without VLX,
  // because for many cases we don't have a better option.
  bool canExtendTo512DQ() const {
    return hasAVX512() && (!hasVLX() || getPreferVectorWidth() >= 512);
  }
  bool canExtendTo512BW() const  {
    return hasBWI() && canExtendTo512DQ();
  }

  // If there are no 512-bit vectors and we prefer not to use 512-bit registers,
  // disable them in the legalizer.
  bool useAVX512Regs() const {
    return hasAVX512() && (canExtendTo512DQ() || RequiredVectorWidth > 256);
  }

  bool useLight256BitInstructions() const {
    return getPreferVectorWidth() >= 256 || AllowLight256Bit;
  }

  bool useBWIRegs() const {
    return hasBWI() && useAVX512Regs();
  }

  bool isXRaySupported() const override { return is64Bit(); }

  /// Use clflush if we have SSE2 or we're on x86-64 (even if we asked for
  /// no-sse2). There isn't any reason to disable it if the target processor
  /// supports it.
  bool hasCLFLUSH() const { return hasSSE2() || is64Bit(); }

  /// Use mfence if we have SSE2 or we're on x86-64 (even if we asked for
  /// no-sse2). There isn't any reason to disable it if the target processor
  /// supports it.
  bool hasMFence() const { return hasSSE2() || is64Bit(); }

  const Triple &getTargetTriple() const { return TargetTriple; }

  bool isTargetDarwin() const { return TargetTriple.isOSDarwin(); }
  bool isTargetFreeBSD() const { return TargetTriple.isOSFreeBSD(); }
  bool isTargetOpenBSD() const { return TargetTriple.isOSOpenBSD(); }
  bool isTargetDragonFly() const { return TargetTriple.isOSDragonFly(); }
  bool isTargetSolaris() const { return TargetTriple.isOSSolaris(); }
  bool isTargetPS() const { return TargetTriple.isPS(); }

  bool isTargetELF() const { return TargetTriple.isOSBinFormatELF(); }
  bool isTargetCOFF() const { return TargetTriple.isOSBinFormatCOFF(); }
  bool isTargetMachO() const { return TargetTriple.isOSBinFormatMachO(); }

  bool isTargetLinux() const { return TargetTriple.isOSLinux(); }
  bool isTargetKFreeBSD() const { return TargetTriple.isOSKFreeBSD(); }
  bool isTargetGlibc() const { return TargetTriple.isOSGlibc(); }
  bool isTargetAndroid() const { return TargetTriple.isAndroid(); }
  bool isTargetNaCl() const { return TargetTriple.isOSNaCl(); }
  bool isTargetNaCl32() const { return isTargetNaCl() && !is64Bit(); }
  bool isTargetNaCl64() const { return isTargetNaCl() && is64Bit(); }
  bool isTargetMCU() const { return TargetTriple.isOSIAMCU(); }
  bool isTargetFuchsia() const { return TargetTriple.isOSFuchsia(); }

  bool isTargetWindowsMSVC() const {
    return TargetTriple.isWindowsMSVCEnvironment();
  }

  bool isTargetWindowsCoreCLR() const {
    return TargetTriple.isWindowsCoreCLREnvironment();
  }

  bool isTargetWindowsCygwin() const {
    return TargetTriple.isWindowsCygwinEnvironment();
  }

  bool isTargetWindowsGNU() const {
    return TargetTriple.isWindowsGNUEnvironment();
  }

  bool isTargetWindowsItanium() const {
    return TargetTriple.isWindowsItaniumEnvironment();
  }

  bool isTargetCygMing() const { return TargetTriple.isOSCygMing(); }

  bool isOSWindows() const { return TargetTriple.isOSWindows(); }

  bool isTargetWin64() const { return Is64Bit && isOSWindows(); }

  bool isTargetWin32() const { return !Is64Bit && isOSWindows(); }

  bool isPICStyleGOT() const { return PICStyle == PICStyles::Style::GOT; }
  bool isPICStyleRIPRel() const { return PICStyle == PICStyles::Style::RIPRel; }

  bool isPICStyleStubPIC() const {
    return PICStyle == PICStyles::Style::StubPIC;
  }

  bool isPositionIndependent() const;

  bool isCallingConvWin64(CallingConv::ID CC) const {
    switch (CC) {
    // On Win64, all these conventions just use the default convention.
    case CallingConv::C:
    case CallingConv::Fast:
    case CallingConv::Tail:
    case CallingConv::Swift:
    case CallingConv::SwiftTail:
    case CallingConv::X86_FastCall:
    case CallingConv::X86_StdCall:
    case CallingConv::X86_ThisCall:
    case CallingConv::X86_VectorCall:
    case CallingConv::Intel_OCL_BI:
      return isTargetWin64();
    // This convention allows using the Win64 convention on other targets.
    case CallingConv::Win64:
      return true;
    // This convention allows using the SysV convention on Windows targets.
    case CallingConv::X86_64_SysV:
      return false;
    // Otherwise, who knows what this is.
    default:
      return false;
    }
  }

  /// Classify a global variable reference for the current subtarget according
  /// to how we should reference it in a non-pcrel context.
  unsigned char classifyLocalReference(const GlobalValue *GV) const;

  unsigned char classifyGlobalReference(const GlobalValue *GV,
                                        const Module &M) const;
  unsigned char classifyGlobalReference(const GlobalValue *GV) const;

  /// Classify a global function reference for the current subtarget.
  unsigned char classifyGlobalFunctionReference(const GlobalValue *GV,
                                                const Module &M) const;
  unsigned char
  classifyGlobalFunctionReference(const GlobalValue *GV) const override;

  /// Classify a blockaddress reference for the current subtarget according to
  /// how we should reference it in a non-pcrel context.
  unsigned char classifyBlockAddressReference() const;

  /// Return true if the subtarget allows calls to immediate address.
  bool isLegalToCallImmediateAddr() const;

  /// Return whether FrameLowering should always set the "extended frame
  /// present" bit in FP, or set it based on a symbol in the runtime.
  bool swiftAsyncContextIsDynamicallySet() const {
    // Older OS versions (particularly system unwinders) are confused by the
    // Swift extended frame, so when building code that might be run on them we
    // must dynamically query the concurrency library to determine whether
    // extended frames should be flagged as present.
    const Triple &TT = getTargetTriple();

    unsigned Major = TT.getOSVersion().getMajor();
    switch(TT.getOS()) {
    default:
      return false;
    case Triple::IOS:
    case Triple::TvOS:
      return Major < 15;
    case Triple::WatchOS:
      return Major < 8;
    case Triple::MacOSX:
    case Triple::Darwin:
      return Major < 12;
    }
  }

  /// If we are using indirect thunks, we need to expand indirectbr to avoid it
  /// lowering to an actual indirect jump.
  bool enableIndirectBrExpand() const override {
    return useIndirectThunkBranches();
  }

  /// Enable the MachineScheduler pass for all X86 subtargets.
  bool enableMachineScheduler() const override { return true; }

  bool enableEarlyIfConversion() const override;

  void getPostRAMutations(std::vector<std::unique_ptr<ScheduleDAGMutation>>
                              &Mutations) const override;

  AntiDepBreakMode getAntiDepBreakMode() const override {
    return TargetSubtargetInfo::ANTIDEP_CRITICAL;
  }
};

} // end namespace llvm

#endif // LLVM_LIB_TARGET_X86_X86SUBTARGET_H
