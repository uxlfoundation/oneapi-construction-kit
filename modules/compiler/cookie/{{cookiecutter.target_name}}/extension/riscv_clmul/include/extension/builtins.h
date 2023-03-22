// Copyright (C) Codeplay Software Limited. All Rights Reserved.
{% if cookiecutter.copyright_name != "" -%}
/// Additional changes Copyright (C) {{cookiecutter.copyright_name}}. All Rights
/// Reserved.
{% endif -%}

#ifndef {{cookiecutter.target_name_capitals}}_EXTENSION_BUILTINS_H_INCLUDED
#define {{cookiecutter.target_name_capitals}}_EXTENSION_BUILTINS_H_INCLUDED
 
 // Produce the lower half of the 128-bit carry-less product
 __attribute__((overloadable)) long clmul(long, long);
 
 // Produce the upper half of the 128-bit carry-less product
 __attribute__((overloadable)) long clmulh(long, long);
 
 // Produces bits 126-63 of the 128-bit carry-less product
 __attribute__((overloadable)) long clmulr(long, long);

#endif
