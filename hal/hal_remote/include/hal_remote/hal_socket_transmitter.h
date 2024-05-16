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

#include <string>

namespace hal {

/// @brief A very simple socket based version of a hal_transmitter
/// @note This supports both client and server mode, and the sure should use
/// start_server() or make_connection() as appropriate. This does not support
/// the required port being 0 to allow a server to find a free port. For some
/// operations the `error_code` enum will be used, but for `send` and `receive`
/// these are derived functions, so get_last_error() can be used.
///
/// We highly recommend using port forwarding and a user process with this to
/// reduce any security risk.
class hal_socket_transmitter : public hal_transmitter {
 public:
  /// @note the default port allows us to create the
  hal_socket_transmitter(uint16_t port = 0, const char *node = "127.0.0.1")
      : port_requested(port), node(node) {}
  ~hal_socket_transmitter();

  enum error_code {
    success,
    socket_failed,
    port_0_requested,
    bind_failed,
    connect_failed,
    connection_closed,
    listen_failed,
    accept_failed,
    send_error,
    recv_error,
    getsockname_failed,
    getaddrinfo_failed
  };

  /// @brief set port we wish to request on. This must be done before any calls
  /// to start_server or make_connection.
  /// @param port we wish to make request on
  /// @note This duplicates the constructor argument but makes it easier
  ///       in some cases to default it initially and then set it later.
  void set_port(uint16_t port) { port_requested = port; }

  /// @brief set node we wish to limit connections from.
  /// @param node_in we wish to limit connections from
  /// @note This duplicates the constructor argument but makes it easier
  ///       in some cases to default it initially and then set it later.
  void set_node(const char *node_in) { node = node_in; }

  /// @brief Start the server end
  /// @param print_port optionally print out that we are listening on a
  /// particular port
  /// @return error_code::success if successful. If unsuccessful last_error
  /// will also contain error
  error_code start_server(bool print_port);

  /// @brief make a connection to a server
  /// @return error_code::success if successful. If unsuccessful last_error
  /// will also contain the error
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
  /// @return last error
  int get_last_error() const { return last_error; }

  ///  @brief Receive `size` bytes of data into `data`
  ///  @return true if was able to receive the data. Note that false may
  ///  indicate that the connection was dropped or there were errors. In this
  ///  case check last_error for information (connection_closed if dropped)
  bool receive(void *data, uint32_t size) override;

  /// @brief Send `size` bytes of `data` with an optional flush
  bool send(const void *data, uint32_t size, bool flush) override;

  /// @brief Attempt to shut the connection down gracefully.
  void shutdown();

 private:
  /// @brief connect to the remote server
  /// @return error_code::success if successful
  error_code connect();

  /// @brief set up a connection basic ready for connect() or listen/accept
  ///        optionally adding a bind if being used as server and getting the
  ///        port number bound to.
  /// @param server in server mode, requiring a bind() call
  /// @return error_code::success for success
  error_code setup_connection(bool server);

  /// @brief call listen
  /// @return true if listen succeeded
  bool listen();

  /// @brief Call accept
  /// @return true if accept succeeded
  bool accept();

  int get_port() { return current_port; }

  uint16_t port_requested;
  uint16_t current_port = 0;
  sockaddr server_address;
  int sock = -1;
  int fd_to_use = -1;
  bool setup_connection_done = false;
  error_code last_error = error_code::success;
  bool is_connected = false;
  std::string node;
};
}  // namespace hal
#endif