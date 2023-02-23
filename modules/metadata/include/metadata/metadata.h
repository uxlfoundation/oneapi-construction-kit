// Copyright (C) Codeplay Software Limited. All Rights Reserved.
#ifndef MD_METADATA_H_INCLUDED
#define MD_METADATA_H_INCLUDED

/// @file metadata.h
///
/// @brief Metadata API specification.
///
/// There are two distinct phases in metadata creation: creating, and
/// formatting the data; and writing it somewhere the target knows about.
/// Depending on the target and the binary format, this might be stored in a
/// binary section, in a separate file on disk. Historically this has been done
/// in an ad-hoc fashion, and the present header is an attempt at maintaining
/// the required flexibility without having targets implement their own handling
/// over and over again. In addition to targets handling their own, internal,
/// target-specific metadata, the runtimes also sometimes need a place to keep
/// this. For example, OpenCL needs to store metadata on `printf` calls among
/// other things. This was borne in mind in the design of the present API, which
/// can transparently handle *multiple* different metadata sections without the
/// target needing to understand the frontend. The design is split into 3 parts
///
///   1. Target mechanism: user callbacks provided by the target which are
///      tasked with writing a byte-stream wherever they like.
///
///   2. Metadata creation and querying. The model here is a re-targetable,
///      simple and flexible stack interface allowing the creation and
///      deserialization of the basic datatypes: strings, integers, real
///      numbers, arrays, and hash tables. It never exposes the serialization
///      format to the user. This allows some flexibility in how the internals
///      of the API do the actual nitty-gritty byte wrangling and avoids tying
///      to one project license.
///
///   3. Serialization mechanism. This is the byte-stream format, and internally
///      consists of a simple header, a list of serialized blocks, and their
///      name, size, and serialization format. This allows any number of named
///      blocks to be created, and later queried without the targets needing to
///      care about the format. It also conveniently allows the language
///      runtimes to transparently store their own metadata alongside the target
///
/// N.B. The API here is a *strict* C-compatable interface, enabling usage on
/// either side of mux. The internal implementation will be done in C++ as
/// usual.
///
/// @copyright Copyright (C) Codeplay Software Limited. All Rights Reserved.

#ifdef __cplusplus
extern "C" {
#endif

#if __cplusplus >= 201103 || __STDC_VERSION__ >= 201112
#define MD_STATIC_ASSERT(...) static_assert(__VA_ARGS__)
#elif __STDC_VERSION__ >= 201112
_Static_assert(__VA_ARGS__)
#else
#define MD_STATIC_ASSERT(expr, msg) \
  extern int md_static_assert_failed_if_negative_array_size[(expr) ? 1 : -1]
#endif

#include <limits.h>
#include <stddef.h>
#include <stdint.h>

enum md_err {
  MD_E_OOM = INT_MIN,
  MD_E_TYPE_ERR,
  MD_E_RANGE_ERR,
  MD_E_INDEX_ERR,
  MD_E_KEY_ERR,
  MD_E_DUPLICATE_KEY,
  MD_E_PRECISION_ERR,
  MD_E_EMPTY_STACK,
  MD_E_STACK_CORRUPT,
  MD_E_INVALID_FMT_STR,
  MD_E_INVALID_BINARY,
  MD_E_INVALID_KEY,
  MD_E_NO_HOOKS,
  MD_E_INVALID_FLAGS,
  MD_E_STACK_NOT_REGISTERED,
  MD_E_STACK_ALREADY_REGISTERED,
  MD_E_STACK_FINALIZED,
  MD_SUCCESS,
  MD_E_MAX_ /* end canary. Not a real error enumerator */
};

/// @brief Enumeration of supported block binary encodings.
enum md_enc : uint8_t {
  MD_NO_ENC,
  MD_ENC_ZLIB,
  MD_ENC_BROTLI,
  MD_ENC_LZMA,
  // MD_ENC_ ...
  MD_ENC_MAX_ /* end canary. Not a real error enumerator */
};

MD_STATIC_ASSERT(md_enc::MD_ENC_MAX_ < UCHAR_MAX, "Max encoding size reached");

/// @brief Enumeration of supported block binary formats.
enum md_fmt : uint8_t {
  MD_FMT_RAW_BYTES,
  MD_FMT_MSGPACK,
  MD_FMT_JSON,
  MD_FMT_LLVM_BC_MD,
  MD_FMT_LLVM_TEXT_MD,
  // MD_FMT_ ...
  MD_FMT_MAX_ /* end canary. Not a real error enumerator */
};

MD_STATIC_ASSERT(md_fmt::MD_FMT_MAX_ < UCHAR_MAX, "Max format size reached");

/// @brief Check if an integer value is an md_err.
///
/// @param ERR An integer error code.
/// @return true if is an error, false otherwise.
#define MD_CHECK_ERR(ERR) (ERR < 0 && ERR != md_err::MD_SUCCESS)

MD_STATIC_ASSERT(MD_E_MAX_ < 0, "errors must be negative");

/* Opaque handle types. All data manipulations for serialization and
 deserialization go through the API's helpers */
typedef struct md_ctx_ *md_ctx;
typedef struct md_stack_ *md_stack;
typedef struct md_value_ *md_value;

/// @brief IO and allocation handlers for targets.
///
/// Since the metadata can be written and read from anywhere, all IO is done via
/// user-provided callback. This allows us to read and write metadata into e.g.
/// an ELF section, or store it on disk in a separate, encrypted file if
/// necessary, but not care about the specifics. If only reading, only `map` and
/// `finalize` are required to be non-null; if writing, `write` and `finalize`.
/// The target will then be responsible for cleaning up after itself on the
/// library's invocation of
///`finalize`
struct md_hooks {
  /// @brief Get a pointer to the beginning of a previously serialized metadata
  /// section.
  ///
  /// If the target cannot map directly, it should read into an internal buffer
  /// and return a pointer to that buffer. `n` should be filled in to the total
  /// length of the deserialized buffer. The target should keep the returned
  /// pointer alive until `finalize` is called.
  void *(*map)(void *userdata, size_t *n);

  /// @brief Write a block of data into the target's datastore.
  ///
  /// The block of data provided in `src` should be copied up to `n` bytes and
  /// appended to the target's output as this function can be called
  /// multiple times. The call should return a valid md_err enumeration as an
  /// error/success, which will be returned from a call to `md_finalize_ctx`.
  ///
  /// @param userdata The user supplied data.
  /// @param src A pointer to the serialized data.
  /// @param n The length (in-bytes) of the serialized data.
  /// @return md_err enumeration to signal success/failure of writing.
  md_err (*write)(void *userdata, const void *src, size_t n);

  /// @brief Finalize and close the target's storage for this context. No more
  /// calls to `write` or `map` will be made.
  ///
  /// @param userdata The user supplied data.
  void (*finalize)(void *userdata);

  /// @brief A callback for custom allocator.
  ///
  /// If no user-provided allocate() hook is present, then malloc() will be
  /// used, otherwise all heap allocations will be handled by this user-provided
  /// allocator. Its behaviour should match the semantics of the standard
  /// `malloc` function, but align according to the `align` parameter, or
  /// default if zero.
  ///
  /// @param size Size (in bytes) to be allocated.
  /// @param align The alignment requirement.
  /// @param userdata User supplied data.
  /// @return Pointer to the allocated object, NULL if allocation failed.
  void *(*allocate)(size_t size, size_t align, void *userdata);

  /// @brief A callback for custom deallocator.
  ///
  /// Deallocate memory previously allocated with `allocate()` hook. This
  /// callback will default to `free()` if no user implementation is
  /// provided.
  ///
  /// @param ptr The pointer to be deallocated.
  /// @param userdata User supplied data.
  void (*deallocate)(void *ptr, void *userdata);
};

/// @brief Initialize a new context, which can handle multiple blocks.
///
/// @param io A pointer to the user provided callback functions.
/// @param userdata Custom user supplied data which is passed to all
/// `md_hooks` callbacks to their corresponding `userdata`
/// argument.
/// @return Opaque metadata context.
md_ctx md_init(struct md_hooks *io, void *userdata);

/// @brief Create a raw normal, formatted, metadata block named by the
/// zero-terminated `name`.
///
/// @param ctx A handle to the current context.
/// @param name The name of the block.
/// @return A handle to the newly created stack.
md_stack md_create_block(md_ctx ctx, const char *name);

enum md_value_type {
  MD_TYPE_SINT,
  MD_TYPE_UINT,
  MD_TYPE_REAL,
  MD_TYPE_BYTESTR,
  MD_TYPE_ZSTR,
  MD_TYPE_ARRAY,
  MD_TYPE_HASH,
};

/// @brief Represents valid endian encoding in the metadata binary format.
enum MD_ENDIAN : uint8_t { LITTLE = 0x01, BIG = 0x02 };

/// @brief Get the tag type of value.
///
/// @param val The value to be queried.
/// @return The corresponding value type.
enum md_value_type md_get_value_type(md_value val);

/// @brief Get a pointer to the value at block index `idx`.
///
/// @param idx The index of the value.
/// @return md_value
md_value md_get_value(md_stack, size_t idx);

/// @brief Get the encoding used by a metadata context.
///
/// @param ctx The context to query.
/// @return MD_ENDIAN
MD_ENDIAN md_get_endianness(md_ctx ctx);

/// @brief Get the size of the stack.
///
/// @param stack The stack to be queried.
/// @return An index to the top of the stack, otherwise
/// md_err::MD_E_EMPTY_STACK.
int md_top(md_stack stack);

/// @brief Get the size of the stack after removing an element.
///
/// @param stack The stack to be queried.
/// @return An index to the top of the stack, md_err otherwise.
int md_pop(md_stack stack);

/// @brief Push an unsigned integer to the stack.
///
/// @param stack The stack to which the value will be pushed.
/// @param val The value.
/// @return Index of the pushed element, md_err otherwise.
int md_push_uint(md_stack stack, uint64_t val);

/// @brief Push an signed integer to the stack.
///
/// @param stack The stack to which the value will be pushed.
/// @param val The value.
/// @return Index of the pushed element, md_err otherwise.
int md_push_sint(md_stack stack, int64_t val);

/// @brief Push a byte-array onto the stack.
///
/// @param stack The stack to which the value will be pushed.
/// @param bytes Pointer to the start of the byte-array.
/// @param len The length of bytes.
/// @return Index of the pushed byte-array, md_err otherwise.
int md_push_bytes(md_stack stack, const void *bytes, size_t len);

/// @brief Push a zero-terminated string (including the zero terminator, which
/// must be present) onto the stack
///
/// @param stack The stack to which the value will be pushed.
/// @param str A zero-terminated string.
/// @return Index of the pushed string, md_err otherwise.
int md_push_zstr(md_stack, const char *str);

/// @brief Push a double-precision floating real value `val` onto the stack.
///
/// @param stack The stack to which the value will be pushed.
/// @param val The value to be pushed.
/// @return Index of the pushed value, md_err otherwise.
int md_push_real(md_stack, double val);

/// @brief Push a new array to the stack. If `n_elements_hint` is present, then
/// that number of values is pre-reserved.
///
/// @param stack The stack to which the value will be pushed.
/// @param n_elements_hint The amount of space to pre-reserve (optional).
/// @return Index of the pushed array, md_err otherwise.
int md_push_array(md_stack stack, size_t n_elements_hint = 0);

/// @brief Append a value to the end an array.
///
/// The value which is appended must strictly be above the array on the stack.
/// I.e. `appendee_idx` must be strictly greater than `array_idx`
///
/// @param stack The stack on which the values are present.
/// @param array_idx The index of the array of the stack.
/// @param appendee_idx The index of the value to be appended on the stack.
/// @return Index of the element within the array, md_err otherwise.
int md_array_append(md_stack stack, int array_idx, int appendee_idx);

/// @brief Create a new hashtable/associative array and push it to the stack.
///
/// Keys are required to be any scalar datatype & values can be any datatype. If
/// `n_elements_hint` is present, then that number of space will be pre-reserved
/// in the hashtable.
///
/// @param stack The stack on which the values are present.
/// @param n_elements_hint The amount of space to pre-reserve.
/// @return Index of the the hashtable on the stack, md_err otherwise.
int md_push_hashtable(md_stack stack, size_t n_elements_hint = 0);

/// @brief Insert a new key-value pair into a hashtable.
///
/// @param stack The stack on which the values are present.
/// @param table_idx The index of the hashtable (on the stack).
/// @param key_idx The index of the key (on the stack).
/// @param val_idx The index of the value (on the stack).
/// @return Index of the element within the hashtable, md_err otherwise.
int md_hashtable_setkv(md_stack stack, int table_idx, int key_idx, int val_idx);

/// @brief Push values to a stack according to a printf like format string.
///
/// e.g.
///```
/// md_pushf(block, "{s:s}", "hello", "world");
///```
/// pushes a hashtable containing a single entry with key of "hello", and value
/// of "world" to the top of the stack '{' introduces a hashtable; '}' closes
/// it. Such braces must be balanced '[' introduces an array; ']' closes it.
/// Such brackets must be balanced ':' signifies a key/value pair. Only valid in
/// a hash-table elements are comma delimited.
///
/// The format specifiers are as follows (somewhat inspired by printf) with the
/// correspondingly consumed arguments and their types. Where multiple arguments
/// are listed, they are required in that order e.g. md_pushf(block, "s", 4,
///"hello") results in the non-null-terminated "hello" being pushed to the top
/// of the stack.
///
/// * z: [`const char *`] zero-terminated byte-string
/// * s: [`size_t len`, `const void *`] byte string
/// * f: [`double`] real number
/// * u: [`uint64_t`] unsigned integer
/// * i: [`int64_t`] signed integer
///
/// @param stack The stack to which the value will be pushed.
/// @param fmt A valid format string.
/// @param ... VA_ARGS list of arguments to push according to the format.
/// @return Index to the top of the stack, md_err otherwise.
int md_pushf(md_stack stack, const char *fmt, ...);

/// @brief "Free" the given value. Deallocation may be internally deferred if
/// the value is still live (e.g. present somewhere on the stack).
///
/// @param val The value to be released.
void md_release_val(md_value val);

/// @brief Get a block from the stack.
///
/// @param ctx The context from which to find the block.
/// @param name The name of the block.
/// @return md_stack handle, or nullptr if a stack with that name doesn't
/// exist.
md_stack md_get_block(md_ctx ctx, const char *name);

/// @brief Get a pointer and length of the give MD_TYPE_BYTESTR value.
///
/// @param val The MD_TYPE_BYTESTR value.
/// @param s A pointer to the byte-string data.
/// @param len A pointer to the stored length.
/// @return MD_SUCCESS, or md_err if failure.
int md_get_bytes(md_value val, char **s, size_t *len);

/// @brief Get a pointer and length of the given MD_TYPE_ZSTR.
///
/// @param val The MD_TYPE_ZSTR value.
/// @param s A pointer to the zero-terminated string.
/// @param len A pointer to the stored length.
/// @return MD_SUCCESS, or md_err if failure.
int md_get_zstr(md_value val, char **s, size_t *len);

/// @brief Read back a floating point value.
///
/// @param val The value to read from.
/// @param f A pointer to the stored float.
/// @return MD_SUCCESS, or md_err if failure.
int md_get_real(md_value val, double *f);

/// @brief Read back a signed integer value.
///
/// @param val The value to read from.
/// @param i A pointer to the stored value.
/// @return MD_SUCCESS, or md_err if failure.
int md_get_sint(md_value val, int64_t *i);

/// @brief Read back an unsigned integer value.
///
/// @param val The value to read from.
/// @param i A pointer to the stored value.
/// @return MD_SUCCESS, or md_err if failure.
int md_get_uint(md_value val, uint64_t *i);

/// @brief Read a value from an array at a specific index.
///
/// @param array The array value to read from.
/// @param idx The index within the array.
/// @param val Pointer to the specific value.
/// @return MD_SUCCESS, or md_err if failure.
int md_get_array_idx(md_value array, size_t idx, md_value *val);

/// @brief Get the length of an array value.
///
/// @param array The array to query.
/// @return The length of the array, md_err otherwise.
int md_get_array_size(md_value array);

/// @brief Get a value from a hashtable specific by a specific key.
///
/// @param ht The hashtable in which to look.
/// @param key The key to look for.
/// @param val Pointer to the specific value.
/// @return MD_SUCCESS, or md_err if failure.
int md_get_hashtable_key(md_value ht, md_value key, md_value *val);

/// @brief Finalize a given stack.
///
/// After this function is called on a stack, the stack is considered "frozen"
/// and no more push or read operations can be performed on it.
///
/// @param stack The stack to finalize.
/// @return MD_SUCCESS, or md_err if failure.
enum md_err md_finalize_block(md_stack stack);

/// @brief Set the desired serialization format used by the stack.
///
/// @param stack The stack to update.
/// @param fmt The desired Format
/// @return md_err
md_err md_set_out_fmt(md_stack stack, md_fmt fmt);

/// @brief Finalize an `md_ctx`.
///
/// All stacks will be considered finalized and no more stacks can be added to
/// the context.
///
/// @param ctx The context to finalize.
/// @return MD_SUCCESS, or md_err if failure.
enum md_err md_finalize_ctx(md_ctx ctx);

/// @brief Cleanup and deallocation of all internally held resources within
/// `md_ctx`. The context will be closed and the handle will no longer be
/// valid.
///
/// @param ctx The context to be released.
void md_release_ctx(md_ctx ctx);

/// @brief Read values from a stack according to a printf like format string.
///
/// Placeholders are *pointers* to the same datatypes used in `md_pushf`, where
/// the value is stored.
///
/// * z: [`const char **`] zero-terminated byte-string.
/// * s: [`size_t *len`, `const void **`] byte string.
/// * f: [`double *`] real number
/// * u: [`uint64_t*`] unsigned integer
/// * i: [`int64_t *`] signed integer
///
/// For of non-value types (i.e. zstr and byte-string) the data from the stack
/// will be copied into newly allocated memory, which will persist beyond the
/// lifetime or `md_ctx`. This memory is allocated using the user provided
/// custom allocator, and is therefore the caller's responsibility to deallocate
/// this memory with the allocator, or free() if none was provided.
///
/// @param stack The stack from which to load the values.
/// @param fmt A valid format string
/// @param ... VAR_ARGS to which values will be stored.
/// @return MD_SUCCESS, or md_err if failure.
int md_loadf(md_stack stack, const char *fmt, ...);

#ifdef __cplusplus
}  // extern "C"

#endif
#endif  // MD_METADATA_H_INCLUDED
