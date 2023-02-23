// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include <llvm/ADT/Statistic.h>
#include <llvm/Bitcode/BitcodeWriter.h>
#include <llvm/Support/CommandLine.h>
#include <llvm/Support/FileSystem.h>

#include "dxil_bitcode.h"

static llvm::cl::opt<std::string> InputFilename(
    llvm::cl::Positional, llvm::cl::desc("<input DXIL bitcode file>"),
    llvm::cl::init("-"));

static llvm::cl::opt<std::string> OutputFilename(
    "o", llvm::cl::desc("Override output filename"),
    llvm::cl::value_desc("filename"));

int main(const int argc, const char *const argv[]) {
  llvm::cl::ParseCommandLineOptions(argc, argv);

  if (OutputFilename.empty()) {
    llvm::errs() << "Error: no output filename was given (use -o <file>)\n";
    return 1;
  }

  auto errorOrInputFile =
      llvm::MemoryBuffer::getFileOrSTDIN(InputFilename.getValue());

  // If there was an error in getting the input file.
  if (!errorOrInputFile) {
    llvm::errs() << "Error: " << errorOrInputFile.getError().message() << " '"
                 << InputFilename.getValue() << "'\n";
    return 1;
  }

  auto inputFile = std::move(errorOrInputFile.get());

  auto bufferStart =
      reinterpret_cast<const unsigned char *>(inputFile->getBufferStart());
  auto bufferEnd =
      reinterpret_cast<const unsigned char *>(inputFile->getBufferEnd());

  if (!isDXILBitcode(bufferStart, bufferEnd)) {
    llvm::errs() << "Error: DXIL bitcode file was malformed\n";
    return 1;
  }

  llvm::LLVMContext context;
  auto module = parseDXILModule(*inputFile, context);

  if (!module) {
    return 1;
  }

  // Write the resulting binary.
  std::error_code error;
  llvm::raw_fd_ostream outStream(
      OutputFilename, error, llvm::sys::fs::FA_Read | llvm::sys::fs::FA_Write);

  if (error) {
    llvm::errs() << "Error: Unable to open output file '" << OutputFilename
                 << "': " << error.message() << '\n';
    return 1;
  }

  llvm::WriteBitcodeToFile(*module, outStream);

  if (llvm::AreStatisticsEnabled()) {
    llvm::PrintStatistics();
  }
  return 0;
}
