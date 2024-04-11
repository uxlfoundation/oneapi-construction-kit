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
#include <sys/types.h>
#include <unistd.h>

#include <cstdio>

namespace hal {
hal_socket_transmitter::~hal_socket_transmitter() {
  if (sock) {
    close(sock);
  }
  if (fd_to_use && fd_to_use != sock) {
    close(fd_to_use);
  }
}

hal_socket_transmitter::error_code hal_socket_transmitter::start_server(
    bool print_port) {
  if (!setup_connection_done) {
    hal_socket_transmitter::error_code res = setup_connection(true);
    if (res != 0) {
      return res;
    }
    setup_connection_done = true;
  }
  if (listen() != 0) {
    last_error = hal_socket_transmitter::listen_failed;
    return last_error;
  }
  if (print_port) {
    printf("Listening on port %d\n", get_port());
    fflush(stdout);
  }
  if (accept() == -1) {
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
      return last_error;
    }
    setup_connection_done = true;
  }
  last_error = connect();
  return last_error;
}

bool hal_socket_transmitter::receive(void *data, uint32_t size) {
  uint32_t data_to_read = size;
  uint32_t offset = 0;

  // repeatedly recv until `size` bytes is read.
  do {
    int res = recv(fd_to_use, ((char *)data) + offset, data_to_read, 0);
    // If recv returns 0, this indicates the connection has been dropped
    // It's not an error as such but we are not able to continue
    if (res == 0) {
      is_connected = false;
      fd_to_use = 0;
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
  int res = ::connect(sock, (struct sockaddr *)&server_address,
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
  // @return 0 if success, otherwise -1 and errno set
  sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  server_address.sin_family = AF_INET;
  server_address.sin_port = htons(port_requested);
  server_address.sin_addr.s_addr = INADDR_ANY;
  if (server) {
    if (int res = bind(sock, (struct sockaddr *)&server_address,
                       sizeof(server_address)) != 0) {
      return hal_socket_transmitter::bind_failed;
    }

    struct sockaddr_in local_addr;
    socklen_t len = sizeof(local_addr);
    if (int res =
            getsockname(sock, (struct sockaddr *)&local_addr, &len) != 0) {
      return hal_socket_transmitter::getsockname_failed;
    }
    current_port = ntohs(local_addr.sin_port);
  } else {
    current_port = port_requested;
  }
  setup_connection_done = true;
  return hal_socket_transmitter::success;
}

int hal_socket_transmitter::listen() {
  if (int res = ::listen(sock, 1) != 0) {
    return res;
  }
  return 0;
}
int hal_socket_transmitter::accept() {
  int res = ::accept(sock, nullptr, nullptr);
  if (res != -1) {
    fd_to_use = res;
  }
  return res;
}

}  // namespace hal
