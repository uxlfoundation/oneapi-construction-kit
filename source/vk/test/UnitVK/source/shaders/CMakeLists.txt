# Copyright (C) Codeplay Software Limited. All Rights Reserved.

include(Bin2H)
include(GLSL)

foreach(shader_name ${UVK_SHARED_SHADER_NAMES})
  string(REPLACE "." "_" shader_name_esc ${shader_name})
  list(APPEND UVK_SHARED_SHADER_NAMES_ESC ${shader_name_esc})
endforeach()

foreach(name ${UVK_SHADER_NAMES})
  set(comp ${CMAKE_CURRENT_SOURCE_DIR}/${name}.comp)
  set(spvasm ${CMAKE_CURRENT_SOURCE_DIR}/${name}.spvasm)
  set(header ${CMAKE_CURRENT_BINARY_DIR}/${name}.h)
  if(NOT EXISTS ${comp})
    if(NOT EXISTS ${spvasm})
      if(${Clspv_FOUND} AND ${name} IN_LIST UVK_SHARED_SHADER_NAMES_ESC)
        add_dependencies(UnitVK kts-spvasm)
      else()
        message(FATAL_ERROR
          "No files exist for requested shader: ${name}\n"
          "${comp} ${spvasm}")
      endif()
    endif()
    add_spvasm_bin2h_command(${name}_spv ${spvasm} ${header})
  else()
    add_glsl_bin2h_command(${name}_spv ${comp} ${header})
  endif()
  list(APPEND UVK_SHADER_CODE_HEADERS ${header})
endforeach()

add_custom_target(UnitVKShaderCode DEPENDS ${UVK_SHADER_CODE_HEADERS})

set(UVK_SHADER_CODE_HEADER ${CMAKE_CURRENT_BINARY_DIR}/ShaderCode.h)
set(UVK_SHADER_CODE_SOURCE ${CMAKE_CURRENT_BINARY_DIR}/ShaderCode.cpp)

# Write ShaderCode.h which enumerates all shader names and provides the
# getShader() function which can be called to fetch a particular shader binary.
file(WRITE ${UVK_SHADER_CODE_HEADER}
  "// Copyright (C) Codeplay Software Limited. All Rights Reserved.\n"
  "\n"
  "#ifndef UNITVK_SHADER_SOURCE_H_INCLUDED\n"
  "#define UNITVK_SHADER_SOURCE_H_INCLUDED\n"
  "\n"
  "#include <cstddef>\n"
  "#include <string> \n"
  "#include <map> \n"

  "\n"
  "namespace uvk {\n"
  "enum class Shader {\n"
  "none, \n")

foreach(name ${UVK_SHADER_NAMES})
  file(APPEND ${UVK_SHADER_CODE_HEADER} "  ${name},\n")
endforeach()
file(APPEND ${UVK_SHADER_CODE_HEADER}
  "};\n"
  "\n"
  "struct ShaderCode {\n"
  "  const void *code;\n"
  "  const size_t size;\n"
  "};\n"
  "\n"
  "ShaderCode getShader(Shader id);\n"
  "extern std::map<std::string, Shader> ShaderMap; \n"
  "}  // uvk\n"
  "\n"
  "#endif  // UNITVK_SHADER_SOURCE_H_INCLUDED\n")

# Write ShaderCode.cpp which includes each shader binary header and implements
# the getShader() function defined in ShaderCode.h to return the specified
# shader binary.
file(WRITE ${UVK_SHADER_CODE_SOURCE}
  "// Copyright (C) Codeplay Software Limited. All Rights Reserved.\n"
  "\n"
  "#include \"ShaderCode.h\"\n")
foreach(name ${UVK_SHADER_NAMES})
  file(APPEND ${UVK_SHADER_CODE_SOURCE}
    "#include \"${name}.h\"\n")
endforeach()
file(APPEND ${UVK_SHADER_CODE_SOURCE} "\n"
  "uvk::ShaderCode uvk::getShader(uvk::Shader id) {\n"
  "  switch (id) {\n"
  "    case uvk::Shader::none:\n"
  "      return ShaderCode{nullptr, 0};\n")

foreach(name ${UVK_SHADER_NAMES})
  file(APPEND ${UVK_SHADER_CODE_SOURCE}
    "    case uvk::Shader::${name}:\n"
    "      return ShaderCode{${name}_spv, sizeof(${name}_spv)};\n")
endforeach()
file(APPEND ${UVK_SHADER_CODE_SOURCE}
  "  }\n"
  "  return ShaderCode{nullptr, 0};\n"
  "}\n"
  "std::map<std::string, uvk::Shader> uvk::ShaderMap { \n")
foreach(name ${UVK_SHARED_SHADER_NAMES})
  string(REGEX REPLACE "\\." "_" name_esc ${name})
  file(APPEND ${UVK_SHADER_CODE_SOURCE}
    "  {\"${name}\", uvk::Shader::${name_esc}}, \n")
endforeach()
file(APPEND ${UVK_SHADER_CODE_SOURCE}
  "};")


# Set these to parent scope so they can be used by UnitVK.
set(UVK_SHADER_CODE_HEADER ${UVK_SHADER_CODE_HEADER} PARENT_SCOPE)
set(UVK_SHADER_CODE_SOURCE ${UVK_SHADER_CODE_SOURCE} PARENT_SCOPE)
