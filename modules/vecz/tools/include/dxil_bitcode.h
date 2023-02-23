// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#ifndef VECZ_DXIL_BITCODE_H
#define VECZ_DXIL_BITCODE_H

#include <llvm/Bitcode/BitcodeReader.h>
#include <llvm/Support/MemoryBuffer.h>

/// isDXILBitcode - Return true if the given bytes are the magic bytes for a
/// DXIL IR bitcode. This is based on LLVM's isRawBitcode, the only difference
/// being the pattern that is checked.
inline bool isDXILBitcode(const unsigned char *BufPtr,
                          const unsigned char *BufEnd) {
  // These bytes sort of have a hidden message, but it's not in
  // little-endian this time, and it's a little redundant.
  return BufPtr + 3 < BufEnd && BufPtr[0] == 'D' && BufPtr[1] == 'X' &&
         BufPtr[2] == 'B' && BufPtr[3] == 'C';
}

inline std::unique_ptr<llvm::Module> parseDXILModule(
    llvm::MemoryBuffer &inputFile, llvm::LLVMContext &context) {
  auto bufferStart =
      reinterpret_cast<const unsigned char *>(inputFile.getBufferStart());
  auto bufferEnd =
      reinterpret_cast<const unsigned char *>(inputFile.getBufferEnd());

  // If we are consuming a DXIL bitcode file, we need to jump ahead through
  // the DXIL header (which contains the root signature among other things)
  // and find the LLVM bitcode within.

  for (size_t i = 0, e = inputFile.getBufferSize(); i < e; i++) {
    if (llvm::isRawBitcode(bufferStart + i, bufferEnd)) {
      llvm::StringRef bufferData(inputFile.getBufferStart() + i, e - i);
      llvm::MemoryBufferRef ref(bufferData, inputFile.getBufferIdentifier());

      auto errorOrModule = llvm::parseBitcodeFile(ref, context);

      if (!errorOrModule) {
        llvm::errs() << "Error: parsing input DXIL bitcode file failed\n";
        return nullptr;
      }

      return std::move(errorOrModule.get());
    }
  }

  llvm::errs() << "Error: DXIL bitcode file was malformed\n";
  return nullptr;
}

#endif
