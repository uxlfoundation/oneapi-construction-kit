// Copyright (C) Codeplay Software Limited. All Rights Reserved.
#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <regex>
#include <string>

class SharedExecution {
 public:
  bool sharedTestsSupported() { return true; }

 protected:
  SharedExecution() : is_parameterized_(false) {}
  void Fail(const std::string& message) {
    GTEST_NONFATAL_FAILURE_(message.c_str());
  }

  void Fail(const std::string& message, int errorCode) {
    std::stringstream ss;
    ss << message << " (error: " << errorCode << ")";
    GTEST_NONFATAL_FAILURE_(ss.str().c_str());
  }

  /// @brief Determine the kernel path
  ///
  /// Extract the file prefix and kernel name from the current test name.
  /// Format for test names: 'TestSet_N_KernelName', where N is the number
  /// that identifies the test and where '_N_' is the last underscore wrapped
  /// number that occurs in the test name.
  ///
  /// Example name: 'Task_01_01_Copy_Stuff'.
  /// Kernel file to load: 'task_01.01_copy_stuff.cl'.
  ///
  /// @param[in] test_name The name of the test
  /// @param[out] file_prefix The prefix, e.g. Task_01.01
  /// @param[out] kernel_name The kernel name, e.g. copy_stuff
  /// @returns false if something went wrong, true otherwise
  bool GetKernelPrefixAndName(std::string test_name, std::string &file_prefix,
                              std::string &kernel_name) {
    // Strip the parameterization, everything after '/', from the test name.
    if (is_parameterized_) {
      auto slash = test_name.find('/');
      if (std::string::npos == slash) {
        std::cerr << "Warning: Test is parameterized but parameter could not "
                     "be removed from the kernel name.\n";
      } else {
        test_name.resize(slash);
      }
    }

    // Find where the test number is in `test_name`, we use this to split the
    // string into a test set prefix and test name.
    std::regex test_num_pattern("_([0-9]+_)+");
    std::smatch test_num_match;
    bool matched =
        std::regex_search(test_name, test_num_match, test_num_pattern);

    if (!matched) {
      Fail("Invalid test name \"" + test_name +
           "\" must be of the form \"TestSet_N_KernelName\" where \"N\" is a "
           "test number.");
    }

    // Get the position we should split the string at, add size of the match
    // because we want to include the match in the prefix, not the kernel name.
    auto split_index = test_num_match.position() + test_num_match.length() - 1;

    // Extract the file_prefix and kernel_name from the test name.
    file_prefix = test_name.substr(0, split_index);
    file_prefix[file_prefix.find_last_of('_')] = '.';
    kernel_name = test_num_match.suffix().str();

    // Make the file_prefix and kernel_name lower case.
    auto to_lower_case = [](unsigned char c) { return tolower(c); };
    std::transform(file_prefix.begin(), file_prefix.end(), file_prefix.begin(),
                   to_lower_case);
    std::transform(kernel_name.begin(), kernel_name.end(), kernel_name.begin(),
                   to_lower_case);

    return true;
  }

  bool is_parameterized_;
};

// Android compatibility
namespace stdcompat {
#ifdef __ANDROID__
// Note: This function accepts double only as its argument
using ::nan;
using ::nanf;
using ::nanl;
using ::isnan;
#else
using std::nan;
using std::nanf;
using std::nanl;
using std::isnan;
#endif  // __ANDROID__
}
