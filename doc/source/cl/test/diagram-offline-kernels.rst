.. graphviz::
  :align: center
  :caption: Compilation flow for ``UnitCL-offline-execution-kernels`` target (right-click, ``View Image`` for full size)

  digraph {
    fontname=Lato
    compound=true
    style="filled"

    subgraph cluster_target_kernels {
      label="List: ${${CORE_TARGET}_UNITCL_KERNEL_FILES}\n(<target>/kernels/)"
      fillcolor="#f2daf2"
      color="#ccb8cc"
      penwidth=3

      generated_kernels [label="Generated .bc32, .bc64 kernels\nGenerated .spvasm32, .spvasm64 kernels\n(kernels/)\n(<target>/kernels/)"
                         style=filled
                         fillcolor="#fff8a6"
                         color="#d7d18c"
                         penwidth=3]
      target_inputs [label="ComputeMux target kernels\n(<target>/kernels/)"
                     style=filled
                     fillcolor="#fff8a6"
                     color="#d7d18c"
                     penwidth=3]
      ca_kernels [label="Common kernels\n(kernels/)"
                  style=filled
                  fillcolor="#fff8a6"
                  color="#d7d18c"
                  penwidth=3]
    }

    filter_bits [label="Logic: Filter out unsupported bit widths\n(kernels/CMakeLists.txt)"
                 shape=box
                 style=filled
                 fillcolor="#72c2e3"
                 color="#5a99b3"
                 penwidth=3]

    cl_file [label="Logic: Find matching .cl file\n(kernels/CMakeLists.txt)"
             shape=box
             style=filled
             fillcolor="#72c2e3"
             color="#5a99b3"
             penwidth=3]

    join [shape=point
          color="#6a7880"]

    generated_kernels -> filter_bits [color="#6a7880" penwidth=3]
    filter_bits -> join [color="#6a7880" arrowhead=none penwidth=3]
    target_inputs -> join [color="#6a7880" arrowhead=none penwidth=3]
    ca_kernels -> join [color="#6a7880" arrowhead=none penwidth=3]
    join -> cl_file [color="#6a7880" penwidth=3]

    subgraph cluster_compile_bin {
      label="Script: CompileKernelToBin.cmake\n(kernels/CMakeLists.txt)"
      style=filled
      shape=box
      fillcolor="#b8daec"
      color="#97b9cc"
      penwidth=3

      input_file [label="${INPUT_FILE}"
                  style=filled
                  fillcolor="#f2daf2"
                  color="#ccb8cc"
                  penwidth=3]

      input_cl_file [label="${INPUT_CL_FILE}"
                     style=filled
                     fillcolor="#f2daf2"
                     color="#ccb8cc"
                     penwidth=3]

      reqs [label="${REQUIREMENTS_LIST}"
            style=filled
            fillcolor="#f2daf2"
            color="#ccb8cc"
            penwidth=3]

      opts [label="${OPTIONS_LIST}"
            style=filled
            fillcolor="#f2daf2"
            color="#ccb8cc"
            penwidth=3]

      check_skip [label="Must skip?"
                  shape=diamond
                  style=filled
                  fillcolor="#72c2e3"
                  color="#5a99b3"
                  penwidth=3]

      yes [label="Yes"
           shape=circle
           style=filled
           fillcolor="#72c2e3"
           color="#5a99b3"
           penwidth=3]

      no [label="No"
          shape=circle
          style=filled
          fillcolor="#72c2e3"
          color="#5a99b3"
          penwidth=3]

      stub [label="Create stub file"
            shape=box
            style=filled
            fillcolor="#72c2e3"
            color="#5a99b3"
            penwidth=3]

      clc [label="Logic: clc"
           shape=box
           style=filled
           fillcolor="#72c2e3"
           color="#5a99b3"
           penwidth=3]

      // Invisible edges for layout
      input_file -> input_cl_file [arrowhead=none penwidth=0]
      input_cl_file -> reqs [arrowhead=none penwidth=0]

      reqs -> check_skip [color="#6a7880" penwidth=3]
      check_skip -> yes [color="#6a7880" penwidth=3]
      check_skip -> no [color="#6a7880" penwidth=3]
      yes -> stub [color="#6a7880" penwidth=3]
      input_file -> clc [color="#6a7880" penwidth=3]
      no -> clc [color="#6a7880" penwidth=3]
      opts -> clc [color="#6a7880" penwidth=3]

    }

    join -> input_file [color="#6a7880" penwidth=3]
    cl_file -> input_cl_file [color="#6a7880" penwidth=3]

    subgraph cluster_get_opts {
      node [shape=box]
      label="Func: extract_reqs_opts()\n(cmake/ExtractReqsOpts.cmake)"
      fillcolor="#b8daec"
      color="#97b9cc"
      penwidth=3

      parse_reqs [label="Logic: Extract requirements"
                  style=filled
                  fillcolor="#72c2e3"
                  color="#5a99b3"
                  penwidth=3]
      parse_opts [label="Logic: Extract options"
                  style=filled
                  fillcolor="#72c2e3"
                  color="#5a99b3"
                  penwidth=3]
    }

    // Invisible node for layout
    cl_file -> reqs [arrowhead=none penwidth=0]

    input_cl_file -> parse_opts [lhead=cluster_get_opts color="#6a7880" penwidth=3]
    parse_reqs -> reqs [color="#6a7880" penwidth=3]
    parse_opts -> opts [color="#6a7880" penwidth=3]

    bin [label=".bin kernel\n(${PROJECT_BINARY_DIR}/share/kernels_offline/<device>/)\n(${PROJECT_BINARY_DIR}/share/kernels_offline/<device>/spir/)\n(${PROJECT_BINARY_DIR}/share/kernels_offline/<device>/spirv/)"
         style=filled
         fillcolor="#fff8a6"
         color="#d7d18c"
         penwidth=3]

    stub -> bin [color="#6a7880" penwidth=3]
    clc -> bin [color="#6a7880" penwidth=3]

    subgraph cluster_install {
      label="Target: install"
      fillcolor="#ffa6cb"
      color="#e695b7"
      penwidth=3
      subgraph cluster_install_kernels {
        label="Script: InstallKernels.cmake\n(CMakeLists.txt)"
        fillcolor="#b8daec"
        color="#97b9cc"
        penwidth=3
        filter_stub [label="Logic: Filter out stub files"
                     shape=box
                     style=filled
                     fillcolor="#72c2e3"
                     color="#5a99b3"
                     penwidth=3]
        copy [label="Logic: Copy"
              shape=box
              style=filled
              fillcolor="#72c2e3"
              color="#5a99b3"
              penwidth=3]

        filter_stub -> copy [color="#6a7880" penwidth=3]
      }
    }

    installed_kernels [label="Installed kernels\n(${CMAKE_INSTALL_PREFIX}/share/kernels_offline/<device>/)\n(${CMAKE_INSTALL_PREFIX}/share/kernels_offline/<device>/spir/)\n(${CMAKE_INSTALL_PREFIX}/share/kernels_offline/<device>/spirv/)"
                       style=filled
                       fillcolor="#fff8a6"
                       color="#d7d18c"
                       penwidth=3]

    bin -> filter_stub [color="#6a7880" penwidth=3]
    copy -> installed_kernels [color="#6a7880" penwidth=3]
  }

