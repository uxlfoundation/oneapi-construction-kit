.. graphviz::
  :align: center
  :caption: Compilation flow for the ``regenerate-spirv`` target (right-click, ``View Image`` for full size)

  digraph {
    fontname=Lato
    compound=true
    style="filled"

    subgraph cluster_target_kernels {
      label="List: ${${CORE_TARGET}_UNITCL_KERNEL_FILES}\n(<target>/kernels/)"
      fillcolor="#f2daf2"
      color="#ccb8cc"
      penwidth=3
      generated_kernels [label="Generated .spvasm32, .spvasm64 kernels\n(kernels/)\n(<target>/kernels/)"
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

    filter_cl [label="Logic: Filter for only .cl kernels\n(kernels/CMakeLists.txt)"
               shape=box
               style=filled
               fillcolor="#72c2e3"
               color="#5a99b3"
               penwidth=3]

    target_inputs -> filter_cl [ltail=cluster_target_kernels color="#6a7880" penwidth=3]

    filter_cl -> input_file1 [ltail=cluster_target_kernels color="#6a7880" penwidth=3]

    subgraph cluster_build_spirv {
      label="Script: CompileKernelToSPIRVAsm.cmake\n(kernels/CMakeLists.txt)"
      style=filled
      shape=box
      fillcolor="#b8daec"
      color="#97b9cc"
      penwidth=3

      input_file2 [label="${INPUT_FILE}"
                   style=filled
                   fillcolor="#f2daf2"
                   color="#ccb8cc"
                   penwidth=3]

      reqs2 [label="${REQUIREMENTS_LIST}"
             style=filled
             fillcolor="#f2daf2"
             color="#ccb8cc"
             penwidth=3]

      opts2 [label="${OPTIONS_LIST}"
             style=filled
             fillcolor="#f2daf2"
             color="#ccb8cc"
             penwidth=3]

      check_skip2 [label="Must skip?"
                   shape=diamond
                   style=filled
                   fillcolor="#72c2e3"
                   color="#5a99b3"
                   penwidth=3]

      yes2 [label="Yes"
            shape=circle
            style=filled
            fillcolor="#72c2e3"
            color="#5a99b3"
            penwidth=3]

      no2 [label="No"
           shape=circle
           style=filled
           fillcolor="#72c2e3"
           color="#5a99b3"
           penwidth=3]

      make_spvasm_stub32 [label="Create .spvasm32 stub file"
                          shape=box
                          style=filled
                          fillcolor="#72c2e3"
                          color="#5a99b3"
                          penwidth=3]

      make_spvasm_stub64 [label="Create .spvasm64 stub file"
                          shape=box
                          style=filled
                          fillcolor="#72c2e3"
                          color="#5a99b3"
                          penwidth=3]


      clang32 [label="Logic: Modern clang\n(32-bit options)"
               shape=box
               style=filled
               fillcolor="#72c2e3"
               color="#5a99b3"
               penwidth=3]

      clang64 [label="Logic: Modern clang\n(64-bit options)"
               shape=box
               style=filled
               fillcolor="#72c2e3"
               color="#5a99b3"
               penwidth=3]

      temp_bc32_kernel [label=".bc32-temp kernel\n(${CMAKE_CURRENT_BINARY_DIR}/<...>/kernels/)"
                        style=filled fillcolor="#fff8a6"
                        color="#d7d18c"
                        penwidth=3]

      temp_bc64_kernel [label=".bc64-temp kernel\n(${CMAKE_CURRENT_BINARY_DIR}/<...>/kernels/)"
                        style=filled fillcolor="#fff8a6"
                        color="#d7d18c"
                        penwidth=3]

      llvm_spirv [label="Logic: llvm-spirv"
                  shape=box
                  style=filled
                  fillcolor="#72c2e3"
                  color="#5a99b3"
                  penwidth=3]

      spv32_kernel [label=".spv32 kernel\n(${CMAKE_CURRENT_BINARY_DIR}/<...>/kernels/)"
                    style=filled fillcolor="#fff8a6"
                    color="#d7d18c"
                    penwidth=3]

      spv64_kernel [label=".spv64 kernel\n(${CMAKE_CURRENT_BINARY_DIR}/<...>/kernels/)"
                    style=filled fillcolor="#fff8a6"
                    color="#d7d18c"
                    penwidth=3]

      spirv_dis [label="Logic: spirv-dis"
                 shape=box
                 style=filled
                 fillcolor="#72c2e3"
                 color="#5a99b3"
                 penwidth=3]

      delete [label="Logic: Delete temporary files"
              shape=box
              style=filled
              fillcolor="#72c2e3"
              color="#5a99b3"
              penwidth=3]

      // Invisible arrows to aid layout
      input_file2 -> reqs2 [arrowhead=none weight=2 penwidth=0]
      reqs2 -> opts2 [arrowhead=none weight=2 penwidth=0]

      check_skip2 -> yes2 [color="#6a7880" penwidth=3]
      check_skip2 -> no2 [color="#6a7880" penwidth=3]
      yes2 -> make_spvasm_stub32 [color="#6a7880" penwidth=3]
      yes2 -> make_spvasm_stub64 [color="#6a7880" penwidth=3]
      no2 -> clang32 [color="#6a7880" penwidth=3]
      no2 -> clang64 [color="#6a7880" penwidth=3]
      reqs2 -> check_skip2 [color="#6a7880" penwidth=3]
      input_file2 -> clang64 [ltail=cluster_target_kernels color="#6a7880" penwidth=3]
      input_file2 -> clang32 [ltail=cluster_target_kernels color="#6a7880" penwidth=3 weight=2]
      input_file2 -> check_skip2 [penwidth=0 arrowhead=none]
      opts2 -> clang32 [color="#6a7880" penwidth=3]
      opts2 -> clang64 [color="#6a7880" penwidth=3]
      clang32 -> temp_bc32_kernel [color="#6a7880" penwidth=3]
      clang64 -> temp_bc64_kernel [color="#6a7880" penwidth=3]
      temp_bc32_kernel -> llvm_spirv [color="#6a7880" penwidth=3]
      temp_bc64_kernel -> llvm_spirv [color="#6a7880" penwidth=3 style=dotted]
      llvm_spirv -> spv32_kernel [color="#6a7880" penwidth=3]
      llvm_spirv -> spv64_kernel [color="#6a7880" penwidth=3 style=dotted]
      spv32_kernel -> spirv_dis [color="#6a7880" penwidth=3]
      spv64_kernel -> spirv_dis [color="#6a7880" penwidth=3 style=dotted]
      temp_bc32_kernel -> delete [color="#6a7880" penwidth=3 weight=0.1]
      temp_bc64_kernel -> delete [color="#6a7880" penwidth=3 weight=0.1]
      spv32_kernel -> delete [color="#6a7880" penwidth=3 weight=0.1]
      spv64_kernel -> delete [color="#6a7880" penwidth=3 weight=0.1]
    }

    spvasm32_kernel [label=".spvasm32 kernel\n(same dir as source .cl kernel)"
                     style=filled fillcolor="#fff8a6"
                     color="#d7d18c"
                     penwidth=3]

    spvasm64_kernel [label=".spvasm64 kernel\n(same dir as source .cl kernel)"
                     style=filled fillcolor="#fff8a6"
                     color="#d7d18c"
                     penwidth=3]

    spirv_dis -> spvasm32_kernel [color="#6a7880" penwidth=3]
    spirv_dis -> spvasm64_kernel [color="#6a7880" penwidth=3 style=dotted]
    make_spvasm_stub32 -> spvasm32_kernel [color="#6a7880" penwidth=3]
    make_spvasm_stub64 -> spvasm64_kernel [color="#6a7880" penwidth=3]

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

    input_file1 -> parse_reqs [lhead=cluster_get_opts color="#6a7880" penwidth=3 ]
    input_file2 -> parse_opts [lhead=cluster_get_opts color="#6a7880" style=dotted penwidth=3 ]
    filter_cl -> input_file2 [ltail=cluster_target_kernels color="#6a7880" penwidth=3 arrowhead=none arrowtail=none]
    parse_reqs -> reqs1 [color="#6a7880" penwidth=3]
    parse_reqs -> reqs2 [color="#6a7880" style=dotted penwidth=3]
    parse_opts -> opts1 [color="#6a7880" penwidth=3]
    parse_opts -> opts2 [color="#6a7880" penwidth=3]

  }

