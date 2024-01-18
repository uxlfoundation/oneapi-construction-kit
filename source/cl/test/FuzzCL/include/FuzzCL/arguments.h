// Copyright (C) Codeplay Software Limited
//
// Licensed under the Apache License, Version 2.0 (the "License") with LLVM
// Exceptions; you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     https://github.com/codeplaysoftware/oneapi-construction-kit/blob/main/LICENSE.txt
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
// WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
// License for the specific language governing permissions and limitations
// under the License.
//
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#ifndef FUZZCL_ARGUMENTS_H_INCLUDED
#define FUZZCL_ARGUMENTS_H_INCLUDED

#include <cargo/argument_parser.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <fstream>
#include <iostream>

#ifdef _MSC_VER
#include <strsafe.h>
#include <windows.h>
#else
#include <dirent.h>
#endif

#define KERNEL_SOURCE_DIR "FuzzCL_kernels/"

namespace fuzzcl {
/// @brief List files in a directory
///
/// @param[in] name Directory name
/// @param[out] files Array to write filenames to
void list_dir(const std::string &name, std::vector<std::string> &files) {
  // return if the folder doesn't exists
  struct stat info;
  if (stat(name.c_str(), &info) != 0) {
    return;
  }

#ifdef _MSC_VER
  if (name.size() > (MAX_PATH - 3)) {
    std::cerr << "Directory path is too long: " << name << ".\n";
    exit(1);
  }

  TCHAR dirPath[MAX_PATH];
  StringCchCopy(dirPath, MAX_PATH, name.c_str());
  StringCchCat(dirPath, MAX_PATH, TEXT("\\*"));

  WIN32_FIND_DATA findData;
  HANDLE findHandle = FindFirstFile(dirPath, &findData);
  if (findHandle == INVALID_HANDLE_VALUE) {
    std::cerr << "Failed finding first file in directory: " << name << ".\n";
    exit(1);
  }

  do {
    if (!(findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
      files.push_back(findData.cFileName);
    }
  } while (FindNextFile(findHandle, &findData) != 0);

  if (GetLastError() != ERROR_NO_MORE_FILES) {
    std::cerr << "Failed to read directory: " << name << ".\n";
    exit(1);
  }

  FindClose(findHandle);
#else
  DIR *dir = opendir(name.c_str());
  if (dir == nullptr) {
    std::cerr << "Failed opening " << name << ".\n";
    exit(1);
  }

  struct dirent *d;
  while ((d = readdir(dir)) != nullptr) {
    if (strcmp(d->d_name, ".") != 0 && strcmp(d->d_name, "..") != 0) {
      files.push_back(d->d_name);
    }
  }
  closedir(dir);
#endif
}

/// @brief Read the content of a file
///
/// @param[in] filepath Path of the file to read
///
/// @tparam T The type of the read data
///
/// @return Returns an array of bytes
template <class T>
std::vector<T> read_file(const std::string &filepath) {
  std::ifstream fs;
  // std::ios::ate moves the cursor at the end of the file
  fs.open(filepath, std::ios::binary | std::ios::in | std::ios::ate);
  if (!fs.is_open()) {
    std::cerr << "Failed opening " << filepath << ".\n";
    exit(1);
  }

  // tellg() returns the position of the cursor in bytes
  const size_t size = fs.tellg();
  // move the cursor at the beginning
  fs.seekg(0, std::ios::beg);

  std::vector<T> data;
  char value;
  for (size_t i = 0; i < size; i++) {
    fs.read(&value, sizeof(T));
    data.push_back(static_cast<T>(value));
  }

  return data;
}

/// @brief Add an argument to the parser
///
/// @param[in,out] parser A parser to add the argument to
/// @param[out] storage A `cargo::string_view` to store the argument
/// @param[in] name Name of the argument
/// @param[in] secondaryName Secondary name of the argument
void add_argument(cargo::argument_parser<1> &parser,
                  cargo::string_view &storage, cargo::string_view name,
                  cargo::string_view secondaryName) {
  if (auto error = parser.add_argument({name, storage})) {
    std::cerr << error << '\n';
    exit(1);
  }
  if (auto error = parser.add_argument({secondaryName, storage})) {
    std::cerr << error << '\n';
    exit(1);
  }
}

/// @brief Add a bool argument to the parser
///
/// @param[in,out] parser A parser to add the argument to
/// @param[out] storage A bool to store the argument
/// @param[in] name Name of the argument
/// @param[in] secondaryName Secondary name of the argument
void add_argument(cargo::argument_parser<1> &parser, bool &storage,
                  cargo::string_view name, cargo::string_view secondaryName) {
  if (auto error = parser.add_argument({name, storage})) {
    std::cerr << error << '\n';
    exit(1);
  }
  if (auto error = parser.add_argument({secondaryName, storage})) {
    std::cerr << error << '\n';
    exit(1);
  }
}

/// @brief Add a single name argument to the parser
///
/// @param[in,out] parser A parser to add the argument to
/// @param[out] storage A `cargo::string_view` to store the argument
/// @param[in] name Name of the argument
void add_argument(cargo::argument_parser<1> &parser, bool &storage,
                  cargo::string_view name) {
  if (auto error = parser.add_argument({name, storage})) {
    std::cerr << error << '\n';
    exit(1);
  }
}
}  // namespace fuzzcl

#endif
