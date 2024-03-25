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
#include <metadata/detail/md_ctx.h>
#include <metadata/detail/md_stack.h>
#include <metadata/detail/md_value.h>
#include <metadata/metadata.h>

#include <algorithm>
#include <cstdarg>
#include <stack>

md_ctx md_init(md_hooks *hooks, void *userdata) {
  md::callback_allocator<md_ctx_> alloc(hooks, userdata);
  using AllocatorTraits = std::allocator_traits<decltype(alloc)>;

  md_ctx_ *ctx = AllocatorTraits::allocate(alloc, 1);
  ctx = new (ctx) md_ctx_(hooks, userdata);
  if (ctx->decode_binary() == md_err::MD_E_INVALID_BINARY) {
    md_release_ctx(ctx);
    return nullptr;
  }
  return ctx;
}

void md_release_ctx(md_ctx ctx) {
  md::callback_allocator<md_ctx_> alloc =
      ctx->get_alloc_helper().get_allocator<md_ctx_>();
  std::allocator_traits<decltype(alloc)>::destroy(alloc, ctx);
  std::allocator_traits<decltype(alloc)>::deallocate(alloc, ctx, 1);
}

md_value md_get_value(md_stack stack, size_t idx) {
  const auto top = stack->top();
  if (!top.has_value()) {
    return nullptr;
  }
  if (idx > top.value()) {
    return nullptr;
  }
  const auto it = std::next(stack->begin(), idx);
  return static_cast<md_value>(&*it);
}

md_value_type md_get_value_type(md_value val) { return val->get_type(); }

MD_ENDIAN md_get_endianness(md_ctx ctx) { return ctx->get_endianness(); }

int md_top(md_stack stack) {
  const auto top = stack->top();
  return top.value_or(top.error());
}

int md_pop(md_stack stack) {
  const auto popped = stack->pop();
  return popped.value_or(popped.error());
}

int md_push_uint(md_stack block, uint64_t val) {
  const auto result = block->push_unsigned(val);
  return result.value_or(result.error());
}

int md_push_sint(md_stack block, int64_t val) {
  const auto result = block->push_signed(val);
  return result.value_or(result.error());
}

int md_push_bytes(md_stack stack, const void *bytes, size_t len) {
  const auto result = stack->push_bytes((const uint8_t *)bytes, len);
  return result.value_or(result.error());
}

int md_push_zstr(md_stack stack, const char *str) {
  const auto result = stack->push_zstr(str);
  return result.value_or(result.error());
}

int md_push_real(md_stack stack, double val) {
  const auto result = stack->push_real(val);
  return result.value_or(result.error());
}

int md_push_array(md_stack stack, size_t n_elements_hint) {
  const auto result = stack->push_arr(n_elements_hint);
  return result.value_or(result.error());
}

int md_array_append(md_stack stack, int array_idx, int appendee_idx) {
  const auto arr_idx = stack->arr_append(array_idx, appendee_idx);
  return arr_idx.value_or(arr_idx.error());
}

int md_push_hashtable(md_stack stack, size_t n_elements_hint) {
  const auto ht_idx = stack->push_map(n_elements_hint);
  return ht_idx.value_or(ht_idx.error());
}

int md_hashtable_setkv(md_stack stack, int table_idx, int key_idx,
                       int val_idx) {
  const auto kv_idx = stack->hash_set_kv(table_idx, key_idx, val_idx);
  return kv_idx.value_or(kv_idx.error());
}

md_stack md_get_block(md_ctx ctx, const char *name) {
  const auto block = ctx->get_block(name);
  if (!block.has_value()) {
    return nullptr;
  }
  return static_cast<md_stack>(block.value());
}

md_stack md_create_block(md_ctx ctx, const char *name) {
  const auto block = ctx->create_block(name);
  if (!block.has_value()) {
    return nullptr;
  }
  return static_cast<md_stack>(block.value());
}

void md_release_val(md_value) {
  // Do nothing -> deallocating happens when stack is destroyed
}

int md_get_bytes(md_value val, char **s, size_t *len) {
  if (val->get_type() != md_value_type::MD_TYPE_BYTESTR) {
    return md_err::MD_E_TYPE_ERR;
  }
  const auto *const byte_arr = val->get<md_stack_::byte_arr_t>();
  *len = byte_arr->size();
  const auto &helper = val->get_alloc_helper();
  char *const buf = helper.get_allocator<char>().allocate(*len);
  std::memcpy(buf, byte_arr->data(), *len);
  *s = buf;
  return md_err::MD_SUCCESS;
}

int md_get_zstr(md_value val, char **s, size_t *len) {
  if (val->get_type() != md_value_type::MD_TYPE_ZSTR) {
    return md_err::MD_E_TYPE_ERR;
  }
  const auto *const str = val->get<md_stack_::string_t>();
  *len = str->length() + 1;  // additional null terminator
  const auto &helper = val->get_alloc_helper();
  char *const buf = helper.get_allocator<char>().allocate(*len);
  *s = std::strncpy(buf, str->c_str(), *len);
  return md_err::MD_SUCCESS;
}

int md_get_real(md_value val, double *f) {
  if (val->get_type() != md_value_type::MD_TYPE_REAL) {
    return md_err::MD_E_TYPE_ERR;
  }
  const auto *const real = val->get<md_stack_::real_t>();
  *f = *real;
  return md_err::MD_SUCCESS;
}

int md_get_sint(md_value val, int64_t *i) {
  if (val->get_type() != md_value_type::MD_TYPE_SINT) {
    return md_err::MD_E_TYPE_ERR;
  }
  const auto *const sint = val->get<md_stack_::signed_t>();
  *i = *sint;
  return md_err::MD_SUCCESS;
}

int md_get_uint(md_value val, uint64_t *i) {
  if (val->get_type() != md_value_type::MD_TYPE_UINT) {
    return md_err::MD_E_TYPE_ERR;
  }
  const auto *const uint_val = val->get<md_stack_::unsigned_t>();
  *i = *uint_val;
  return md_err::MD_SUCCESS;
}

int md_get_array_idx(md_value array, size_t idx, md_value *val) {
  if (array->get_type() != md_value_type::MD_TYPE_ARRAY) {
    return md_err::MD_E_TYPE_ERR;
  }
  auto *const arr = array->get<md_stack_::array_t>();
  if (idx >= arr->size()) {
    return md_err::MD_E_INDEX_ERR;
  }
  *val = static_cast<md_value>(&arr->at(idx));
  return md_err::MD_SUCCESS;
}

int md_get_array_size(md_value array) {
  if (array->get_type() != md_value_type::MD_TYPE_ARRAY) {
    return md_err::MD_E_TYPE_ERR;
  }
  auto *const arr = array->get<md_stack_::array_t>();
  return arr->size();
}

int md_get_hashtable_key(md_value ht, md_value key, md_value *val) {
  if (ht->get_type() != md_value_type::MD_TYPE_HASH) {
    return md_err::MD_E_TYPE_ERR;
  }
  auto *const map = ht->get<md_stack_::map_t>();
  const auto it = map->find(*key);
  if (it == map->end()) {
    return md_err::MD_E_INVALID_KEY;
  }
  *val = static_cast<md_value>(&(it->second));
  return md_err::MD_SUCCESS;
}

int md_pushf(md_stack stack, const char *fmt, ...) {
  // Initialize C-style variable number arguments
  va_list argp;
  va_start(argp, fmt);

  // -- Bookkeeping -- //
  // * The array_map_stack allows us to keep track of the level of nesting as
  //   well as what data structure we are currently dealing with i.e. hashtable
  //   or array.
  // * The v_idx_stack is used to store temporary indexes to values on the stack
  //   which will be used to append to and array or set a hashtable key-value
  //   pair
  struct fmt_stack_element {
    md_value_type type;
    size_t index;
  };
  std::stack<fmt_stack_element> array_map_stack;
  std::stack<size_t> v_idx_stack;

  // If for any reason we cannot complete the `md_pushf` operation, we want to
  // release the arguments and empty the stack - leaving it in its original
  // state
  auto cleanup = [&]() -> void {
    // cleanup arg list
    va_end(argp);

    // cleanup the stack
    while (stack->top()) {
      md_pop(stack);
    }
  };

  // Sets a key-value pair in a hashtable or appends a value to an array, based
  // on the the state of the array_map_stack.
  auto push_to_hash_or_array = [&]() -> int {
    if (array_map_stack.empty()) {
      return md_err::MD_E_INVALID_FMT_STR;
    }
    if (array_map_stack.top().type == md_value_type::MD_TYPE_HASH) {
      // must be at least two values to set a key/value pair
      if (v_idx_stack.size() < 2) {
        return md_err::MD_E_INVALID_FMT_STR;
      }
      const size_t val_idx = v_idx_stack.top();
      v_idx_stack.pop();
      const size_t key_idx = v_idx_stack.top();
      v_idx_stack.pop();
      const size_t hashtable_idx = array_map_stack.top().index;
      int err = md_hashtable_setkv(stack, hashtable_idx, key_idx, val_idx);
      if (MD_CHECK_ERR(err)) {
        return err;
      }
      // once we set key/value we pop the remaining values off the metadata
      // stack
      err = md_pop(stack);
      if (MD_CHECK_ERR(err)) {
        return err;
      }
      err = md_pop(stack);
      if (MD_CHECK_ERR(err)) {
        return err;
      }
      return md_err::MD_SUCCESS;
    }
    if (array_map_stack.top().type == md_value_type::MD_TYPE_ARRAY) {
      // At least one value must exist on the stack to append to the array
      if (v_idx_stack.size() < 1) {
        return md_err::MD_E_INVALID_FMT_STR;
      }
      const size_t val_idx = v_idx_stack.top();
      v_idx_stack.pop();
      const size_t arr_idx = array_map_stack.top().index;
      int err = md_array_append(stack, arr_idx, val_idx);
      if (MD_CHECK_ERR(err)) {
        return err;
      }
      err = md_pop(stack);
      if (MD_CHECK_ERR(err)) {
        return err;
      }
      return md_err::MD_SUCCESS;
    }
    return md_err::MD_E_INVALID_FMT_STR;
  };

  while (*fmt != '\0') {
    switch (*fmt) {
      case '[': {
        const int arr_idx_or_err = md_push_array(stack, 0);
        if (MD_CHECK_ERR(arr_idx_or_err)) {
          return cleanup(), arr_idx_or_err;
        }
        if (!array_map_stack.empty()) {
          v_idx_stack.push(arr_idx_or_err);
        }
        array_map_stack.push(fmt_stack_element{
            md_value_type::MD_TYPE_ARRAY, static_cast<size_t>(arr_idx_or_err)});
      } break;
      case ']': {
        if (array_map_stack.top().type != md_value_type::MD_TYPE_ARRAY) {
          return cleanup(), md_err::MD_E_INVALID_FMT_STR;
        }
        if (!v_idx_stack.empty()) {
          const int err = push_to_hash_or_array();
          if (MD_CHECK_ERR(err)) {
            return cleanup(), err;
          }
        }
        array_map_stack.pop();
      } break;
      case '{': {
        const int map_idx_or_err = md_push_hashtable(stack, 0);
        if (MD_CHECK_ERR(map_idx_or_err)) {
          return cleanup(), map_idx_or_err;
        }
        if (!array_map_stack.empty()) {
          v_idx_stack.push(map_idx_or_err);
        }
        array_map_stack.push(fmt_stack_element{
            md_value_type::MD_TYPE_HASH, static_cast<size_t>(map_idx_or_err)});
      } break;
      case '}': {
        if (array_map_stack.top().type != md_value_type::MD_TYPE_HASH) {
          return cleanup(), md_err::MD_E_INVALID_FMT_STR;
        }
        if (!v_idx_stack.empty()) {
          const int err = push_to_hash_or_array();
          if (MD_CHECK_ERR(err)) {
            return cleanup(), err;
          }
        }
        array_map_stack.pop();
      } break;
      case 'z': {
        const char *str_to_push = va_arg(argp, char *);
        const int zstr_idx_or_err = md_push_zstr(stack, str_to_push);
        if (MD_CHECK_ERR(zstr_idx_or_err)) {
          return cleanup(), zstr_idx_or_err;
        }
        if (!array_map_stack.empty()) {
          v_idx_stack.push(zstr_idx_or_err);
        }
      } break;
      case 's': {
        const size_t byte_str_len = va_arg(argp, size_t);
        const void *byte_str = va_arg(argp, void *);
        const int byte_str_idx_or_err =
            md_push_bytes(stack, byte_str, byte_str_len);
        if (MD_CHECK_ERR(byte_str_idx_or_err)) {
          return cleanup(), byte_str_idx_or_err;
        }
        if (!array_map_stack.empty()) {
          v_idx_stack.push(byte_str_idx_or_err);
        }
      } break;
      case 'f': {
        const double double_to_push = va_arg(argp, double);
        const int real_idx = md_push_real(stack, double_to_push);
        if (MD_CHECK_ERR(real_idx)) {
          return cleanup(), real_idx;
        }
        if (!array_map_stack.empty()) {
          v_idx_stack.push(real_idx);
        }
      } break;
      case 'u': {
        const uint64_t int_to_push = va_arg(argp, uint64_t);
        const int uint_idx_or_err = md_push_uint(stack, int_to_push);
        if (MD_CHECK_ERR(uint_idx_or_err)) {
          return cleanup(), uint_idx_or_err;
        }
        if (!array_map_stack.empty()) {
          v_idx_stack.push(uint_idx_or_err);
        }
      } break;
      case 'i': {
        const int64_t int_to_push = va_arg(argp, int64_t);
        const int sint_idx_or_err = md_push_sint(stack, int_to_push);
        if (MD_CHECK_ERR(sint_idx_or_err)) {
          return cleanup(), sint_idx_or_err;
        }
        if (!array_map_stack.empty()) {
          v_idx_stack.push(sint_idx_or_err);
        }
      } break;
      case ':':
        if (array_map_stack.top().type != md_value_type::MD_TYPE_HASH) {
          return cleanup(), md_err::MD_E_INVALID_FMT_STR;
        }
        if (v_idx_stack.size() < 1) {
          return cleanup(), md_err::MD_E_INVALID_FMT_STR;
        }
        break;
      case ',': {
        const int err = push_to_hash_or_array();
        if (MD_CHECK_ERR(err)) {
          return cleanup(), err;
        }
      } break;
      case ' ':
        break;
      default:
        return cleanup(), md_err::MD_E_INVALID_FMT_STR;  // NOLINT
        break;
    }
    ++fmt;
  }

  // If the array_map_stack or v_idx_stack is non empty it suggests that there
  // is a unterminated array or hashtable.
  if (!(array_map_stack.empty() && v_idx_stack.empty())) {
    return cleanup(), md_err::MD_E_INVALID_FMT_STR;  // NOLINT
  }

  va_end(argp);
  return md_top(stack);
}

int md_loadf(md_stack stack, const char *fmt, ...) {
  va_list argp;
  va_start(argp, fmt);

  // -- Bookkeeping -- //
  // * The array_map_stack is used to identify the current level of nesting as
  //   well as storing values of hashtables or arrays.
  // * dangling_pointers is used to store a pointer to a value which requires an
  //   allocation (byte-string or strings). If parsing fails, we need to make
  //   sure we deallocate their memory before returning the appropriate error
  //   code.
  // * cur_stack_pointer is an index to the current stack item we are parsing.
  // * is_key is used when parsing a key-value pair in a hashtable to correctly
  //   identify which item is the key and which is the value.
  struct stack_element {
    md_value value;
    size_t index;
  };
  std::stack<stack_element> array_map_stack;
  std::vector<char *> dangling_pointers;
  size_t cur_stack_pointer = 0;
  bool is_key = true;

  auto cleanup = [&]() -> void {
    // cleanup arg list
    va_end(argp);

    // cleanup any dangling pointers
    for (char *ptr : dangling_pointers) {
      stack->get_alloc_helper().get_allocator<char>().deallocate(ptr, 0);
    }
  };

  // Get a next md_value based on the current context.
  // If we are within an array, the next element in the array is returned.
  // If we are within a hashtable, the next key or value is returned.
  // Otherwise the next stack value is returned.
  auto get_value = [&]() -> cargo::expected<md_value, int> {
    // If nothing on the array_map_stack => just get the element at the current
    // stack pointer
    if (array_map_stack.empty()) {
      auto val = md_get_value(stack, cur_stack_pointer);
      if (!val) {
        return cargo::make_unexpected(md_err::MD_E_INDEX_ERR);
      }
      return val;
    }

    md_value value = array_map_stack.top().value;
    if (md_get_value_type(value) == md_value_type::MD_TYPE_ARRAY) {
      auto *const arr = value->get<md_stack_::array_t>();
      if (arr->size() <= array_map_stack.top().index) {
        return cargo::make_unexpected(md_err::MD_E_INDEX_ERR);
      }
      return static_cast<md_value>(&arr->at(array_map_stack.top().index));
    }

    if (md_get_value_type(value) == md_value_type::MD_TYPE_HASH) {
      auto *const hash_tbl = value->get<md_stack_::map_t>();
      if (hash_tbl->size() <= array_map_stack.top().index) {
        return cargo::make_unexpected(md_err::MD_E_INDEX_ERR);
      }
      auto &key_val = hash_tbl->at(array_map_stack.top().index);
      return is_key ? static_cast<md_value>(&key_val.first)
                    : static_cast<md_value>(&key_val.second);
    }
    return cargo::make_unexpected(md_err::MD_E_INVALID_FMT_STR);
  };

  while (*fmt != '\0') {
    switch (*fmt) {
      case '[': {
        const auto arr = get_value();
        if (!arr.has_value()) {
          return cleanup(), arr.error();
        }
        if (arr.value()->get_type() != md_value_type::MD_TYPE_ARRAY) {
          return cleanup(), md_err::MD_E_TYPE_ERR;
        }
        array_map_stack.push({arr.value(), 0});
      } break;
      case ']':
        array_map_stack.pop();
        break;
      case '{': {
        const auto hash = get_value();
        if (!hash.has_value()) {
          return cleanup(), hash.error();
        }
        if (hash.value()->get_type() != md_value_type::MD_TYPE_HASH) {
          return cleanup(), md_err::MD_E_TYPE_ERR;
        }
        array_map_stack.push({hash.value(), 0});
        is_key = true;
      } break;
      case '}':
        array_map_stack.pop();
        break;
      case ',': {
        array_map_stack.top().index += 1;
        is_key = true;
      } break;
      case ':':
        is_key = false;
        break;
      case 'z': {
        const char **str_to_store = va_arg(argp, const char **);
        const auto value_or_err = get_value();
        if (!value_or_err.has_value()) {
          return cleanup(), value_or_err.error();
        }
        md_value value = value_or_err.value();
        size_t str_len;
        char *str_data;
        const int err = md_get_zstr(value, &str_data, &str_len);
        dangling_pointers.push_back(str_data);
        if (MD_CHECK_ERR(err)) {
          return cleanup(), err;
        }
        *str_to_store = str_data;
      } break;
      case 'u': {
        uint64_t *val_to_store = va_arg(argp, uint64_t *);
        const auto value_or_err = get_value();
        if (!value_or_err.has_value()) {
          return cleanup(), value_or_err.error();
        }
        md_value value = value_or_err.value();
        const int err = md_get_uint(value, val_to_store);
        if (MD_CHECK_ERR(err)) {
          return cleanup(), err;
        }
      } break;
      case 'i': {
        int64_t *val_to_store = va_arg(argp, int64_t *);
        const auto value_or_err = get_value();
        if (!value_or_err.has_value()) {
          return cleanup(), value_or_err.error();
        }
        md_value value = value_or_err.value();
        const int err = md_get_sint(value, val_to_store);
        if (MD_CHECK_ERR(err)) {
          return cleanup(), err;
        }
      } break;
      case 'f': {
        double *val_to_store = va_arg(argp, double *);
        const auto value_or_err = get_value();
        if (!value_or_err.has_value()) {
          return cleanup(), value_or_err.error();
        }
        md_value value = value_or_err.value();
        const int err = md_get_real(value, val_to_store);
        if (MD_CHECK_ERR(err)) {
          return cleanup(), err;
        }
      } break;
      case 's': {
        size_t *len_to_store = va_arg(argp, size_t *);
        char **bytes_to_store = va_arg(argp, char **);
        const auto value_or_err = get_value();
        if (!value_or_err.has_value()) {
          return cleanup(), value_or_err.error();
        }
        md_value value = value_or_err.value();
        char *bytes_data;
        size_t bytes_len;
        const int err = md_get_bytes(value, &bytes_data, &bytes_len);
        dangling_pointers.push_back(bytes_data);
        if (MD_CHECK_ERR(err)) {
          return cleanup(), err;
        }
        *bytes_to_store = bytes_data;
        *len_to_store = bytes_len;
      } break;
      default:
        break;
    }
    ++fmt;
    if (array_map_stack.empty()) {
      ++cur_stack_pointer;
    }
  }

  if (!array_map_stack.empty()) {
    return cleanup(), md_err::MD_E_INVALID_FMT_STR;  // NOLINT
  }
  va_end(argp);
  return md_top(stack);
}

md_err md_finalize_block(md_stack stack) { return stack->freeze_stack(); }

md_err md_finalize_ctx(md_ctx ctx) { return ctx->finalize(); }

md_err md_set_out_fmt(md_stack stack, md_fmt fmt) {
  stack->set_out_fmt(fmt);
  return md_err::MD_SUCCESS;
}
