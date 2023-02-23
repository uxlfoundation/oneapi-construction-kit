// Copyright (C) Codeplay Software Limited. All Rights Reserved.

/// @file
///
/// @brief OpenCL specified limits.
///
/// @copyright Copyright (C) Codeplay Software Limited. All Rights Reserved.

#ifndef CL_LIMITS_H_INCLUDED
#define CL_LIMITS_H_INCLUDED

#include <cstddef>
#include <cstdint>

namespace cl {
/// @addtogroup cl
/// @{

namespace max {
/// @brief  Define the limits of the work item, work group.
enum limits : uint32_t {
  WORK_ITEM_DIM = 3,  ///< Maximum supported work item dimensions.
};
}  // namespace max

/// @}
}  // namespace cl

#endif  // CL_LIMITS_H_INCLUDED
