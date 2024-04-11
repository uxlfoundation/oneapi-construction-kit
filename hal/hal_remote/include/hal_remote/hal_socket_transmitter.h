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

#ifndef _HAL_SOCKET_TRANSMITTER_H
#define _HAL_SOCKET_TRANSMITTER_H

#include <hal_remote/hal_transmitter.h>
#include <netinet/in.h>

namespace hal {

/// @brief A very simple socket based version of a hal_transmitter
/// @note This supports both client and server mode, and the sure should use
/// start_server() or make_connection() as appropriate. This supports the
/// required port being 0 for a server allows it to find a free port. For
/// some operations error_code enum will be used, but for `send` and `receive`
/// these are derived functions, so get_last_error() can be used.
class hal_socket_transmitter : public hal_transmitter {
 public:
  hal_socket_transmitter(uint16_t port = 0) : port_requested(port) {}
  ~hal_socket_transmitter();

  enum error_code {
    success,
    bind_failed,
    connect_failed,
    connection_closed,
    listen_failed,
    accept_failed,
    send_error,
    recv_error,
    getsockname_failed
  };

  /// @brief set port we wish to request on. This must be done before any calls
  /// to start_server or make_connection.
  /// @param port we wish to make request on
  /// @note This duplicates the constructor argument but makes it easier
  ///       in some cases to default it initially and then set it later.
  void set_port(uint16_t port) { port_requested = port; }

  /// @brief Start the server end
  /// @param print_port optionally print out that we are listening on a
  /// particular port
  /// @return success if successful. If unsuccessful last_error also will
  /// contain error
  error_code start_server(bool print_port);

  /// @brief make a connection to a server
  /// @return success if successful. If unsuccessful last_error also will
  /// contain error
  error_code make_connection();

  /// @brief Indicates that a connection is live (either as server or as a
  /// client)
  /// @note it may still be the case that the last send/receive was in error
  ///       but we keep the connected flag live and report false in that case.
  ///       If the connection was dropped, recv can receive 0 bytes which is
  ///       what is used to set this to false.
  /// @return true if connected.
  bool connected() const { return is_connected; }

  /// @brief returns the last error from socket operations
  /// @return last error reported from the socket
  int get_last_error() const { return last_error; }

  ///  @brief Receive `size` bytes of data into `data`
  ///  @return true if was able to receive the data. Note that false may
  ///  indicate that the connection was dropped or there were errors. In this
  ///  case check last_error for information (connection_closed if dropped)
  bool receive(void *data, uint32_t size) override;

  /// @brief Send `size` bytes of `data` with an optional flush
  bool send(const void *data, uint32_t size, bool flush);

 private:
  /// @brief connect to the remote server
  /// @return error returned by connect socket function
  error_code connect();

  /// @brief set up a connection basic ready for connect() or listen/accept
  ///        optionally adding a bind if being used as server and getting the
  ///        port number bound to.
  /// @param server in server mode, requiring a bind() call
  /// @return 0 for success, anything else represents the error from bind or
  ///         getsockname
  error_code setup_connection(bool server);

  int listen();
  int accept();

  int get_port() { return current_port; }

  uint16_t port_requested;
  uint16_t current_port = 0;
  sockaddr_in server_address;
  int sock = 0;
  int fd_to_use = 0;
  bool setup_connection_done = false;
  error_code last_error = error_code::success;
  bool is_connected = false;
};
}  // namespace hal
#endif