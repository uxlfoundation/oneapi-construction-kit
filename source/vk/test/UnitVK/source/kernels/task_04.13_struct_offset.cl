// Copyright (C) Codeplay Software Limited. All Rights Reserved.

struct SampleBuffer {
  float samples[16];
};

typedef global struct SampleBuffer *SamplePtr;

void kernel struct_offset(global struct SampleBuffer *channels, int channelID) {
  size_t id = get_global_id(0);
  SamplePtr channel = (SamplePtr)&channels[channelID];
  channel->samples[id] = id * (1.0f / 16.0f);
}
