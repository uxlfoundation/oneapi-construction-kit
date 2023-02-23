// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include <UnitVK.h>

// https://www.khronos.org/registry/vulkan/specs/1.0/xhtml/vkspec.html#vkMapMemory

class MapMemory : public uvk::DeviceMemoryTest {
 public:
  MapMemory()
      : DeviceMemoryTest(false, 8 * sizeof(uint32_t)), dataList(8, 42) {}

  std::vector<uint32_t> dataList;
};

TEST_F(MapMemory, Default) {
  // to test the functionality of map memory, map the block of device memory,
  // fill it with data, un-map, re-map and check that the data is the same
  void *mappedMemory;

  // map the memory
  DeviceMemoryTest::mapMemory(0, dataList.size() * sizeof(uint32_t),
                              &mappedMemory);

  // fill it with data
  memcpy(mappedMemory, dataList.data(), dataList.size() * sizeof(uint32_t));

  // un-map and re-map
  DeviceMemoryTest::unmapMemory();
  DeviceMemoryTest::mapMemory(0, dataList.size() * sizeof(uint32_t),
                              (void **)&mappedMemory);

  // check the data
  for (int dataIndex = 0, dataEnd = dataList.size(); dataIndex < dataEnd;
       dataIndex++) {
    ASSERT_EQ(reinterpret_cast<uint32_t *>(mappedMemory)[dataIndex],
              dataList[dataIndex]);
  }

  DeviceMemoryTest::unmapMemory();
}

// VK_ERROR_OUT_OF_HOST_MEMORY
// Is a possible return from this function but is untestable
// as it doesn't take an allocator as a parameter.
//
// VK_ERROR_OUT_OF_DEVICE_MEMORY
// Is a possible return from this function, but is untestable
// due to the fact that we can't currently access device memory
// allocators to mess with.
