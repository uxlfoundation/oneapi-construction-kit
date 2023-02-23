// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include "simd_packet.h"

#define DEBUG_TYPE "vecz-simd"

using namespace llvm;
using namespace vecz;

SimdPacket::SimdPacket() : Mask(0) {}

llvm::Value *SimdPacket::at(unsigned Index) const {
  if (Index >= size()) {
    return nullptr;
  } else {
    return (*this)[Index];
  }
}

void SimdPacket::set(unsigned Index, Value *V) {
  if (Index < size()) {
    (*this)[Index] = V;
    Mask.enable(Index);
  }
}

SimdPacket &SimdPacket::update(const SimdPacket &Other) {
  for (unsigned i = 0; i < size(); i++) {
    if (Other.Mask.isEnabled(i)) {
      (*this)[i] = Other[i];
    }
  }
  Mask.Value |= Other.Mask.Value;
  return *this;
}

void PacketMask::enableAll(unsigned NumLanes) {
  for (unsigned i = 0; i < NumLanes; i++) {
    enable(i);
  }
}
