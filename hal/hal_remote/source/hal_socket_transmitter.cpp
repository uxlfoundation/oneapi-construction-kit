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

#include <hal_remote/hal_socket_transmitter.h>
#include <hal_remote/hal_transmitter.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <cstring>
#include <iostream>
#include <string>

namespace hal {
hal_socket_transmitter::~hal_socket_transmitter() { shutdown(); }

hal_socket_transmitter::error_code hal_socket_transmitter::start_server(
    bool print_port) {
  if (!setup_connection_done) {
    const hal_socket_transmitter::error_code res = setup_connection(true);
    if (res != hal_socket_transmitter::success) {
      last_error = res;
      return res;
    }
    setup_connection_done = true;
  }
  if (!listen()) {
    last_error = hal_socket_transmitter::listen_failed;
    return last_error;
  }
  if (print_port) {
    std::cerr << "Listening on port " << get_port() << "\n";
  }
  if (!accept()) {
    last_error = hal_socket_transmitter::accept_failed;
    return last_error;
  }
  last_error = hal_socket_transmitter::success;
  return last_error;
}

/// @brief make a connection to a server
/// @return true if successful, otherwise will set last_error to errno
hal_socket_transmitter::error_code hal_socket_transmitter::make_connection() {
  if (!setup_connection_done) {
    last_error = setup_connection(false);
    if (last_error != hal_socket_transmitter::success) {
      std::cerr << "Failed to set up connection to remote server (port="
                << port_requested << " node=" << node << ")\n";
      return last_error;
    }
    setup_connection_done = true;
  }
  last_error = connect();
  if (last_error != hal_socket_transmitter::success) {
    std::cerr << "Failed to connect to server (port=" << port_requested
              << " node=" << node << ")\n";
  }
  return last_error;
}

bool hal_socket_transmitter::receive(void *data, uint32_t size) {
  uint32_t data_to_read = size;
  uint32_t offset = 0;

  // repeatedly recv until `size` bytes is read.
  do {
    const int res = recv(fd_to_use, ((char *)data) + offset, data_to_read, 0);
    // If recv returns 0, this indicates the connection has been dropped
    // It's not an error as such but we are not able to continue
    if (res == 0) {
      is_connected = false;
      fd_to_use = -1;
      last_error = hal_socket_transmitter::connection_closed;
      return false;
    }
    if (res == -1) {
      last_error = hal_socket_transmitter::recv_error;
      return false;
    }
    data_to_read = data_to_read - res;
    offset += res;
  } while (data_to_read);
  last_error = hal_socket_transmitter::success;
  return true;
}

/// @brief Send `size` bytes of `data` with an optional flush
bool hal_socket_transmitter::send(const void *data, uint32_t size, bool flush) {
  const int ret = ::send(fd_to_use, data, size, 0);
  if (ret == -1) {
    last_error = hal_socket_transmitter::send_error;
    return false;
  }
  last_error = hal_socket_transmitter::success;
  return true;
}

hal_socket_transmitter::error_code hal_socket_transmitter::connect() {
  const int res = ::connect(sock, (struct sockaddr *)&server_address,
                            sizeof(server_address));
  if (res != -1) {
    fd_to_use = sock;
    is_connected = true;
    return hal_socket_transmitter::success;
  } else {
    return hal_socket_transmitter::connect_failed;
  }
}

hal_socket_transmitter::error_code hal_socket_transmitter::setup_connection(
    bool server) {
  // don't support port 0
  if (port_requested == 0) {
    if (debug_enabled()) {
      std::cerr << "port requested must be specified as a non-zero value\n";
    }
    return hal_socket_transmitter::port_0_requested;
  }
  sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

  struct addrinfo hints;
  struct addrinfo *result;
  int sfd, s;
  std::memset(&hints, 0, sizeof(struct addrinfo));
  hints.ai_family = AF_UNSPEC; /* Allow IPv4 or IPv6 */
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_NUMERICSERV;
  hints.ai_protocol = IPPROTO_TCP;

  const std::string port_str = std::to_string(port_requested);
  s = getaddrinfo(node.c_str(), port_str.c_str(), &hints, &result);
  // Use getaddrinfo on the node and port. This may return one or
  // more entries we can bind to in priority order. We take the first
  // one that matches.
  if (s != 0) {
    if (debug_enabled()) {
      std::cerr << "getaddrinfo: " << gai_strerror(s) << "\n";
    }
    return hal_socket_transmitter::getaddrinfo_failed;
  } else {
    bool bind_failed = false;
    bool socket_failed = false;
    for (auto *rp = result; rp != NULL; rp = rp->ai_next) {
      bind_failed = false;
      socket_failed = false;
      sock = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
      if (sock == -1) {
        socket_failed = true;
        continue;
      }
      server_address = *rp->ai_addr;
      if (server) {
        if (bind(sock, rp->ai_addr, rp->ai_addrlen) != 0) {
          close(sock);
          sock = -1;
          bind_failed = true;
          continue;
        }

        struct sockaddr_in local_addr;
        socklen_t len = sizeof(local_addr);
        if (getsockname(sock, (struct sockaddr *)&local_addr, &len) != 0) {
          return hal_socket_transmitter::getsockname_failed;
        }
        current_port = ntohs(local_addr.sin_port);
      }
    }
    if (sock == -1) {
      if (bind_failed) {
        return hal_socket_transmitter::bind_failed;
      } else if (socket_failed) {
        return hal_socket_transmitter::socket_failed;
      }
    }
    if (!server) {
      current_port = port_requested;
    }
  }
  setup_connection_done = true;
  return hal_socket_transmitter::success;
}

bool hal_socket_transmitter::listen() {
  if (::listen(sock, 1) == 0) {
    return true;
  }
  return false;
}

bool hal_socket_transmitter::accept() {
  const int res = ::accept(sock, nullptr, nullptr);
  if (res != -1) {
    fd_to_use = res;
  }
  return (res != -1);
}

void hal_socket_transmitter::shutdown() {
  if (fd_to_use != -1) {
    close(fd_to_use);
  }
  if (sock != -1 && sock != fd_to_use) {
    close(sock);
  }
  fd_to_use = -1;
  sock = -1;
}

}  // namespace hal
