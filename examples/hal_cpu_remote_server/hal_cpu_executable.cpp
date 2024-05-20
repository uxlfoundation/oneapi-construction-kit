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

#include <signal.h>
#include <unistd.h>

#include <iostream>

#include "hal_remote/hal_server.h"
#include "hal_remote/hal_socket_client.h"
#include "hal_remote/hal_socket_transmitter.h"

void print_usage(std::ostream &stream, const char *tool_name) {
  stream << "usage: " << tool_name << " [-h] [-n node] port\n";
  stream << "\tnote : node is an ip address or machine name e.g. \"127.0.0.1\" "
            "(default) or \"localhost\"\n";
  stream
      << "\t       port is an integer non-zero address which will be listened "
         "on\n";
}

hal::hal_socket_transmitter transmitter;

bool process_terminated = false;
void handle_sig(int signum) {
  // Attempt to close down gracefully
  transmitter.shutdown();
  process_terminated = true;
}

// Note this should not be run as root for security reasons.
// It will also run once per connection to avoid getting into a bad state
// It should be rerun if desired to make more than one consecutive connection.
int main(int argc, char **argv) {
  std::string node = "127.0.0.1";
  // Note this default will fail as we do not allow port 0
  uint16_t port = 0;
  struct sigaction action;
  memset(&action, 0, sizeof(action));
  action.sa_handler = handle_sig;
  sigaction(SIGTERM, &action, NULL);

  for (;;) {
    switch (auto opt = getopt(argc, argv, "n:h")) {
      case 'n':
        node = optarg;
        continue;

      case 'h':
        print_usage(std::cout, argv[0]);
        exit(0);
      default:
        std::cerr << "Error: Unexpected options '" << (char)opt << "'\n";
        print_usage(std::cerr, argv[0]);
        exit(1);

      case -1:
        break;
    }

    break;
  }

  if (optind >= argc) {
    std::cerr << "Error : requires port argument\n";
    print_usage(std::cerr, argv[0]);
    exit(1);
  }
  port = std::stoi(argv[optind]);

  uint32_t api_version;
  auto *hal = get_hal(api_version);

  // hal::hal_socket_transmitter transmitter(port, node.c_str());
  transmitter.set_node(node.c_str());
  transmitter.set_port(port);

  hal::hal_server::error_code last_error = hal::hal_server::status_success;
  auto res = transmitter.start_server(true);
  if (res != hal::hal_socket_transmitter::success) {
    std::cerr << "Unable to start server on requested port " << port
              << ", node " << node << "\n";
    return 1;
  }

  hal::hal_server server(&transmitter, hal);
  last_error = server.process_commands();

  if (process_terminated) {
    std::cerr << "Process Terminated\n";
    exit(1);
  }
  // Check if the failure was due the transmitter, if so check if connection
  // closed, otherwise we have a real error.
  if (last_error == hal::hal_server::status_transmitter_failed) {
    if (transmitter.get_last_error() !=
        hal::hal_socket_transmitter::connection_closed) {
      std::cerr << "Error with tcp/ip connection\n";
      return 1;
    } else {
      return 0;
    }
  }

  return 1;
}
