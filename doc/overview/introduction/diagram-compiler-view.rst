.. graphviz::
  :align: center
  :caption: Compilation flow using ``compiler::Module`` and ``compiler::Kernel`` (dashed components are optional)

  digraph {
    node [fontname=Lato];
    fontname=Lato
    compound=true
    style="filled"
    splines=false
    ranksep=0.9

    subgraph cluster_module {
      label="Module"
      labeljust=l
      fillcolor="#f2daf2"
      color="#ccb8cc"
      penwidth=3

      clc [
        label="Module::compileOpenCLC"
        style="filled"
        shape=box
        fillcolor="#fff8a6"
        color="#d7d18c"
        penwidth=3]
      spir [
        label="Module::compileSPIR"
        style="filled"
        shape=box
        fillcolor="#fff8a6"
        color="#d7d18c"
        penwidth=3]
      spirv [
        label="Module::compileSPIRV"
        style="filled"
        shape=box
        fillcolor="#fff8a6"
        color="#d7d18c"
        penwidth=3]

        subgraph cluster_module_finalize {
          labeljust=c
          label="Module::finalize"
          fillcolor="#fff8a6"
          color="#d7d18c"
          penwidth=3
          module_backend [
            label="BaseModule::getLateTargetPasses"
            style="filled"
            shape=box
            fillcolor="#aaddff"
            color="#88aadd"
            penwidth=3]

        subgraph cluster_module_passes {
          label="Passes"
          style="dashed"
          color="#d7d18c"
          penwidth=3
          labeljust=l
          passes_vecz [
            label="Vecz"
            style="filled"
            shape=box
            fillcolor="#fff8a6"
            color="#d7d18c"
            penwidth=3]

          passes_builtins [
            label="Builtins"
            style="filled"
            shape=box
            fillcolor="#fff8a6"
            color="#d7d18c"
            penwidth=3]

          passes_others [
            label="..."
            style="filled"
            shape=box
            fillcolor="#fff8a6"
            color="#d7d18c"
            penwidth=3]
        }

        module_backend->passes_vecz [color="#6a7880" style=dashed penwidth=1]
        module_backend->passes_builtins [color="#6a7880" style=dashed penwidth=1]
        module_backend->passes_others [color="#6a7880" style=dashed penwidth=1]
      }

      module_create_binary [
        label="Module::createBinary"
        style="filled"
        shape=box
        fillcolor="#aaddff"
        color="#88aadd"
        penwidth=3]

      subgraph cluster_module_get_kernel {
        labeljust=c
        label="Module::getKernel"
        fillcolor="#fff8a6"
        color="#d7d18c"
        penwidth=3
        module_create_kernel [
          label="Module::createKernel"
          style="filled,dashed"
          shape=box
          fillcolor="#aaddff"
          color="#88aadd"
          penwidth=3]
      }

        clc -> module_backend [lhead=cluster_module_finalize color="#6a7880" penwidth=3]
        spir -> module_backend [lhead=cluster_module_finalize color="#6a7880" penwidth=3]
        spirv -> module_backend [lhead=cluster_module_finalize color="#6a7880" penwidth=3]
        passes_vecz -> module_create_binary [style=invis]
        passes_others -> module_create_kernel [style=invis]
        passes_builtins -> module_create_binary [ltail=cluster_module_finalize color="#6a7880" penwidth=3]
        passes_builtins -> module_create_kernel [ltail=cluster_module_finalize lhead=cluster_module_get_kernel color="#6a7880" penwidth=3]
    }

    subgraph cluster_kernel {
      labeljust=l
      label="Kernel"
      fillcolor="#f2daf2"
      color="#ccb8cc"
      style="filled,dashed"
      penwidth=3
      kernel_create_specialized_kernel [
        label="Kernel::createSpecializedKernel"
        style="filled,dashed"
        shape=box
        fillcolor="#aaddff"
        color="#88aadd"
        penwidth=3]
    }

    module_create_kernel -> kernel_create_specialized_kernel [color="#6a7880" penwidth=3]
  }
