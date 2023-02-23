.. graphviz::
  :align: center
  :caption: Compilation flow for ``copy-kernels`` target (right-click, ``View Image`` for full size)

  digraph {
    fontname=Lato
    compound=true
    style="filled"

    subgraph cluster_ca_kernels {
      label="List: ${KERNEL_FILES}\n(kernels/kernel_source_list.cmake)"
      fillcolor="#f2daf2"
      color="#ccb8cc"
      penwidth=3
      bc_inputs [label=".bc32 kernels\n.bc64 kernels\n(kernels/)"
                 style="filled,dashed"
                 fillcolor="#fff8a6"
                 color="#d7d18c"
                 penwidth=5]
      spvasm_inputs [label=".spvasm32 kernels\n.spvasm64 kernels\n(kernels/)"
                     style=filled
                     fillcolor="#fff8a6"
                     color="#d7d18c"
                     penwidth=3]
      cl_inputs [label=".cl kernels\n(kernels/)"
                 style=filled fillcolor="#fff8a6"
                 color="#d7d18c"
                 penwidth=3]
    }

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

    subgraph cluster_add_ir {
      node [shape=box]
      label="Func: add_spir_spirv()\n(kernels/kernel_source_list.cmake)"
      fillcolor="#b8daec"
      color="#97b9cc"
      penwidth=3
      filter_cl [label="Logic: Filter for only .cl kernels"
                 style=filled
                 fillcolor="#72c2e3"
                 color="#5a99b3"
                 penwidth=3]

      subgraph cluster_glob {
        label="Logic: Glob for matching files"
        fillcolor="#72c2e3"
        color="#5a99b3"
        penwidth=3
        glob [label=".bc32, .bc64 kernels\n.spvasm32, spvasm64 kernels"
              shape=ellipse
              style=filled
              fillcolor="#fff8a6"
              color="#d7d18c"
              penwidth=3]
      }

      filter_cl -> glob [lhead=cluster_glob color="#6a7880" penwidth=3]
      filter_cl -> glob [lhead=cluster_glob style=dotted color="#6a7880" penwidth=3]
    }

    subgraph cluster_all_kernels {
      label="List: ${ALL_KERNEL_FILES}\n(kernels/CMakeLists.txt)"
      fillcolor="#f2daf2"
      color="#ccb8cc"
      penwidth=3
      common_kernels [label="Common kernels\n(kernels/)"
                      style=filled
                      fillcolor="#fff8a6"
                      color="#d7d18c"
                      penwidth=3]
      target_kernels [label="Target kernels\n(<target>/kernels/"
                      style=filled
                      fillcolor="#fff8a6"
                      color="#d7d18c"
                      penwidth=3]
      all_generated_kernels [label="Generated .bc32, .bc64 kernels\nGenerated .spvasm32, .spvasm64 kernels"
                             style=filled
                             fillcolor="#fff8a6"
                             color="#d7d18c"
                             penwidth=3]

    }

    spvasm_inputs -> ca_kernels [ltail=cluster_ca_kernels color="#6a7880" penwidth=3]
    generated_kernels -> filter_cl [ltail=cluster_target_kernels color="#6a7880" penwidth=3]
    glob -> generated_kernels [color="#6a7880" penwidth=3]
    spvasm_inputs -> common_kernels [ltail=cluster_ca_kernels color="#6a7880" penwidth=3]
    target_inputs -> target_kernels [weight=5 color="#6a7880" penwidth=3]
    all_generated_kernels -> filter_cl [ltail=cluster_all_kernels style=dotted color="#6a7880" penwidth=3]
    glob -> all_generated_kernels [style=dotted color="#6a7880" penwidth=3]

    subgraph cluster_copy_assemble {
      label="Script: CopyOrAssembleKernel.cmake\n(kernels/CMakeLists.txt)"
      fillcolor="#b8daec"
      color="#97b9cc"
      penwidth=3
      all_kernels [label="All kernels"
                   style=filled
                   fillcolor="#fff8a6"
                   color="#d7d18c"
                   penwidth=3]
      assemble [label="Logic: Assemble:\n.spvasm32 -> .spv32\n.spvasm64 -> .spv64"
                shape=box
                style=filled
                fillcolor="#72c2e3"
                color="#5a99b3"
                penwidth=3]
      all_kernels -> assemble [color="#6a7880" penwidth=3]
    }

    all_bindir_kernels [label="All kernels\n(${PROJECT_BINARY_DIR}/share/kernels/)"
                        style=filled
                        fillcolor="#fff8a6"
                        color="#d7d18c"
                        penwidth=3]

    target_kernels -> all_kernels [ltail=cluster_all_kernels color="#6a7880" penwidth=3]
    assemble -> all_bindir_kernels [color="#6a7880" penwidth=3]

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

    installed_kernels [label="Installed kernels\n(${CMAKE_INSTALL_PREFIX}/share/kernels/)"
                       style=filled
                       fillcolor="#fff8a6"
                       color="#d7d18c"
                       penwidth=3]

    all_bindir_kernels -> filter_stub [color="#6a7880" penwidth=3]
    copy -> installed_kernels [color="#6a7880" penwidth=3]
  }

