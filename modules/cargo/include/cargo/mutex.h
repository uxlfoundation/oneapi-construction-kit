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

/// @file
///
/// @brief Mutual exclusion utilities for threading support.

#ifndef CARGO_MUTEX_H_INCLUDED
#define CARGO_MUTEX_H_INCLUDED

#include <mutex>

#include "cargo/thread_safety.h"

namespace cargo {
/// @addtogroup cargo
/// @{

/// @brief A mutual exclusion synchronization primitive.
///
/// cargo::mutex is a wrapper around std::mutex with the addition of Clang's
/// Thread Safety Analysis attributes to provide static analysis of
/// multi-threaded code.
struct CARGO_TS_CAPABILITY("mutex") mutex {
  /// @brief Constructs the mutex.
  constexpr mutex() noexcept = default;

  mutex(const mutex &) = delete;
  mutex &operator=(const mutex &) = delete;

  /// @brief Destroys the mutex.
  ~mutex() {}

  /// @brief Lock the mutex, blocks if the mutex is not available.
  void lock() CARGO_TS_ACQUIRE() { Mutex.lock(); }

  /// @brief Tries to lock the mutex.
  ///
  /// @return Returns `false` if the mutex is not available, `true` otherwise.
  bool try_lock() CARGO_TS_TRY_ACQUIRE(true) { return Mutex.try_lock(); }

  /// @brief Unlocks the mutex.
  void unlock() CARGO_TS_RELEASE() { Mutex.unlock(); }

 private:
  /// @brief The wrapped std::mutex.
  std::mutex Mutex;
};

/// @brief An RAII mutex locking wrapper.
///
/// cargo::lock_guard is a wrapper around std::lock_guard with the addition of
/// Clang's Thread Safety Analysis attributes to provide static analysis of
/// multi-threaded code.
///
/// @tparam Mutex Type of the mutex to lock.
template <class Mutex>
struct CARGO_TS_SCOPED_CAPABILITY lock_guard {
  using mutex_type = Mutex;

  /// @brief Acquires ownership of mutex and locks it.
  ///
  /// Behaviour is undefined when `mutex` is not a recursive mutex and the
  /// current thread already owns the lock.
  ///
  /// @param mutex Mutex to acquire ownership of.
  explicit lock_guard(mutex_type &mutex) CARGO_TS_ACQUIRE(mutex)
      : Lock{mutex} {}

  /// @brief Acquires ownership of mutex without attempting to lock it.
  ///
  /// @param mutex Mutex to acquire ownership of.
  /// @param tag Tag used to select non-locking constructor.
  lock_guard(mutex_type &mutex, std::adopt_lock_t tag) CARGO_TS_REQUIRES(mutex)
      : Lock{mutex, tag} {}

  lock_guard(const lock_guard &) = delete;
  lock_guard &operator=(const lock_guard &) = delete;

  /// @brief Releases ownership of the owned mutex.
  ~lock_guard() CARGO_TS_RELEASE() {}

 private:
  /// @brief The wrapped std::lock_guard.
  std::lock_guard<mutex_type> Lock;
};

/// @brief A std::unique_lock wrapper with thread-safety annotations.
///
/// Implements a subset of std::unique_lock which can be completely and
/// correctly represented with thread-safety annotations:
///
/// * The default constructor, move constructor, and move assignement operator
///   are not present as ownership of the mutex can not be guaranteed. As a
///   result the thread-safety analysis can emit erroneous compile errors.
/// * All constructors which attempt to lock the mutex but may fail are not
///   present as the lock state of the mutex can not be guaranteed. The
///   thread-safety annotations are not capable of representing try acquire
///   operations which do not return a boolean.
///
/// @tparam Mutex Type of the mutex to be held.
///
/// @see https://en.cppreference.com/w/cpp/thread/unique_lock
template <class Mutex>
struct CARGO_TS_SCOPED_CAPABILITY unique_lock {
  using mutex_type = Mutex;

  /// @brief Construct with the given mutex and lock it.
  ///
  /// Locks the associated mutex by calling `mutex.lock()`. The behavior is
  /// undefined if the current thread already owns the mutex except when the
  /// mutex is recursive.
  ///
  /// @param mutex The mutex object to be locked.
  explicit unique_lock(mutex_type &mutex) CARGO_TS_ACQUIRE(mutex)
      : Lock{mutex} {}

  /// @brief Construct with the given mutex.
  ///
  /// Does not lock the associated mutex.
  ///
  /// @param mutex The mutex object not to be locked.
  /// @param tag Tag to select locking strategy.
  unique_lock(mutex_type &mutex, std::defer_lock_t tag) noexcept
      CARGO_TS_EXCLUDES(mutex)
      : Lock{mutex, tag} {}

  /// @brief Construct with an already locked mutex.
  ///
  /// Assumes the calling thread already owns `mutex`.
  ///
  /// @param mutex The mutex object to hold.
  /// @param tag Tag to select locking strategy.
  unique_lock(mutex_type &mutex, std::adopt_lock_t tag) CARGO_TS_REQUIRES(mutex)
      : Lock{mutex, tag} {}

  /// @brief Destroys the lock.
  ///
  /// If `Lock` has an associated mutex and has acquired ownership of it, the
  /// mutex is unlocked.
  ~unique_lock() CARGO_TS_RELEASE() {}

  /// @brief Lock the held mutex.
  ///
  /// Locks (i.e., takes ownership of) the associated mutex. Effectively calls
  /// `mutex()->lock()`.
  void lock() CARGO_TS_ACQUIRE() { Lock.lock(); };

  /// @brief Attempt to lock the held mutex.
  ///
  /// Tries to lock (i.e., takes ownership of) the associated mutex without
  /// blocking. Effectively calls `mutex()->try_lock()`.
  bool try_lock() CARGO_TS_TRY_ACQUIRE(true) { return Lock.try_lock(); };

  /// @brief Attempt to lock the held mutex for the specified duration.
  ///
  /// Tries to lock (i.e., takes ownership of) the associated mutex. Blocks
  /// until specified timeout_duration has elapsed or the lock is acquired,
  /// whichever comes first. On successful lock acquisition returns true,
  /// otherwise returns false. Effectively calls
  /// `mutex()->try_lock_for(timeout_duration)`.
  ///
  /// @param timeout_duration Duration of timeout before giving up.
  ///
  /// @return Returns `true` if the lock is acquired, `false` otherwise.
  template <class Rep, class Period>
  bool try_lock_for(const std::chrono::duration<Rep, Period> &timeout_duration)
      CARGO_TS_TRY_ACQUIRE(true) {
    return Lock.try_lock_for(timeout_duration);
  }

  /// @brief Attempt to lock the held mutex until the specified time.
  ///
  /// Tries to lock (i.e., takes ownership of) the associated mutex. Blocks
  /// until specified timeout_time has been reached or the lock is acquired,
  /// whichever comes first. On successful lock acquisition returns true,
  /// otherwise returns false. May block for longer than until timeout_time has
  /// been reached.
  ///
  /// @param timeout_time The time to stop attempting to lock the mutex at.
  ///
  /// @return Returns `true` if the lock is acquired, `false` otherwise.
  template <class Clock, class Duration>
  bool try_lock_until(
      const std::chrono::time_point<Clock, Duration> &timeout_time)
      CARGO_TS_TRY_ACQUIRE(true) {
    return Lock.try_lock_until(timeout_time);
  }

  /// @brief Unlock the held mutex.
  ///
  /// Unlocks (i.e., releases ownership of) the associated mutex and releases
  /// ownership.
  void unlock() CARGO_TS_RELEASE() { Lock.unlock(); }

  /// @brief Breaks the association of the associated mutex, if any, and *this.
  mutex_type *release() noexcept { return Lock.release(); }

  /// @brief Returns a pointer to the associated mutex, or a null pointer if
  /// there is no associated mutex.
  mutex_type *mutex() const noexcept { return Lock.mutex(); }

  /// @brief Checks whether *this owns a locked mutex or not.
  bool owns_lock() const noexcept { return Lock.owns_lock(); }

  /// @brief Checks whether *this owns a locked mutex or not. Effectively calls
  /// owns_lock().
  explicit operator bool() const noexcept { return bool(Lock); }

 private:
  std::unique_lock<mutex_type> Lock;
};

/// @brief Acquires ownership of a given mutex with an output stream operator.
///
/// Ensure that output streams do not suffer from data races by guarding it
/// with a mutex yet keep the convenience and simplicity of the output stream
/// syntax.
///
/// Example usage:
///
/// ```cpp
/// #include <cargo/mutex.h>
/// #include <iostream>
///
/// struct holder {
///   holder(std::ostream &stream) : stream(stream) {}
///
///   cargo::ostream_lock_guard<std::ostream> out() {
///     return {stream, mutex};
///   }
///
///  private:
///   std::ostream &stream;
///   std::mutex mutex;
/// };
///
/// int main() {
///   holder h(std::cout);
///
///   // aquire the lock for a single statement
///   h.out() << "hello, world\n";
///
///   {
///     // or aquire the lock for the duration of the containing scope
///     auto out = h.out();
///     out << "testing\n";
///     for (int i = 0; i < 3; ++i) {
///       out << i << "\n";
///     }
///   }
/// }
/// ```
///
/// @tparam OStream Type of the output stream.
/// @tparam Mutex Type of the mutex.
template <class OStream, class Mutex = std::mutex>
class ostream_lock_guard {
 public:
  using ostream_type = OStream;
  using mutex_type = Mutex;

  /// @brief Construct the output stream lock guard.
  ///
  /// @param stream Reference to the output stream to be guarded.
  /// @param mutex Mutex to lock.
  ostream_lock_guard(ostream_type &stream, mutex_type &mutex)
      : stream(&stream), lock(mutex) {}

  /// @brief Move constructor transfers ownership of the lock.
  ///
  /// @param other Other output stream lock guard to move from.
  ostream_lock_guard(ostream_lock_guard &&other)
      : stream(other.stream), lock(std::move(other.lock)) {}

  /// @brief Move assignment operator transfers ownership of the lock.
  ///
  /// @param other Other output stream lock guard to move from.
  ///
  /// @return Returns a reference to the newly assigned lock guard.
  ostream_lock_guard &operator=(ostream_lock_guard &&other) {
    stream = other.stream;
    lock = std::move(other.lock);
    return *this;
  }

  /// @brief Output operator is forwarded to the output stream.
  ///
  /// @tparam T Type of the object being passed to the output stream.
  /// @param value Constant reference to the object being passed.
  ///
  /// @return Returns a reference to the output stream.
  template <class T>
  ostream_type &operator<<(const T &value) {
    *stream << value;
    return *stream;
  }

 private:
  ostream_type *stream;
  std::unique_lock<mutex_type> lock;
};

/// @}
}  // namespace cargo

#endif  // CARGO_MUTEX_H_INCLUDED
