// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#ifndef _REFSIDRV_IO_IMPL_H
#define _REFSIDRV_IO_IMPL_H

#include <stdbool.h>
#include <stdarg.h>

#include "device_if.h"
#include "host_io_regs.h"
#if defined(HAL_REFSI_TARGET_M1)
#include "dma_regs.h"
#endif

void* memcpy(void* dest, const void* src, size_t len);

void host_ioctl(int cmd, uintptr_t val) {
  asm volatile ("mv a7, %0\n\t"
                "mv a0, %1\n\t"
                "ecall\n\t"
                : : "r" (cmd), "r" (val) : "a0", "a7");
}

void shutdown(intptr_t code) {
  host_ioctl(HOST_IO_CMD_EXIT, (uint64_t)code);
  while (1) {}
}

// When this function is inlined by the compiler it is not possible to tell
// whether all threads are waiting at the same barrier or not by looking at the
// return address. Prevent inlining to avoid this issue.
void barrier(struct exec_state *state) __attribute__((noinline));

void barrier(struct exec_state *state) {
  uintptr_t link_address;
  asm volatile ("mv %0, ra" : "=r"(link_address));
  host_ioctl(HOST_IO_CMD_BARRIER, link_address);
}

int print(exec_state_t *e, const char *fmt, ...) {
  int ret = 0;
  va_list args;
  va_start(args, fmt);
  ret = vprintm(fmt, args);
  va_end(args);
  return ret;
}

void putstring(const char* s) {
  host_ioctl(HOST_IO_CMD_PUTSTRING, (uintptr_t)s);
}

#if defined(HAL_REFSI_TARGET_M1)
uintptr_t start_dma(void *dst, const void *src, size_t size_in_bytes,
                    struct exec_state *state) {
  volatile uintptr_t *dma_regs = (volatile uintptr_t *)REFSI_DMA_IO_ADDRESS;

  // Configure and start a 1D DMA transfer.
  uint64_t config = REFSI_DMA_1D | REFSI_DMA_STRIDE_NONE;
  dma_regs[REFSI_REG_DMASRCADDR] = (uintptr_t)src;
  dma_regs[REFSI_REG_DMADSTADDR] = (uintptr_t)dst;
  dma_regs[REFSI_REG_DMAXFERSIZE0] = size_in_bytes;
  dma_regs[REFSI_REG_DMACTRL] = config | REFSI_DMA_START;

  // Retrieve and return the transfer ID.
  return dma_regs[REFSI_REG_DMASTARTSEQ];
}

void wait_dma(uintptr_t xfer_id, struct exec_state *state) {
  volatile uintptr_t *dma_regs = (volatile uintptr_t *)REFSI_DMA_IO_ADDRESS;

  // Wait for the specified transfer to be complete. Attempting to wait for an
  // invalid transfer (ID of zero) is a no-op.
  dma_regs[REFSI_REG_DMADONESEQ] = xfer_id;
}
#else
uintptr_t start_dma(void *dst, const void *src, size_t size_in_bytes,
                    struct exec_state *state) {
  uintptr_t xfer_id = state->next_xfer_id++;
  memcpy(dst, src, size_in_bytes);
  return xfer_id;
}

void wait_dma(uintptr_t xfer_id, struct exec_state *state) { /* No-op */ }
#endif // defined(HAL_REFSI_TARGET_M1)

int vsnprint(char* out, size_t n, const char* s, va_list vl)
{
  bool format = false;
  bool longarg = false;
  size_t pos = 0;
  for( ; *s; s++)
  {
    if(format)
    {
      switch(*s)
      {
        case 'l':
          longarg = true;
          break;
        case 'p':
          longarg = true;
          if (++pos < n) out[pos-1] = '0';
          if (++pos < n) out[pos-1] = 'x';
        case 'x':
        {
          long num = longarg ? va_arg(vl, long) : va_arg(vl, int);
          for(int i = 2*(longarg ? sizeof(long) : sizeof(int))-1; i >= 0; i--) {
            int d = (num >> (4*i)) & 0xF;
            if (++pos < n) out[pos-1] = (d < 10 ? '0'+d : 'a'+d-10);
          }
          longarg = false;
          format = false;
          break;
        }
        case 'd':
        {
          long num = longarg ? va_arg(vl, long) : va_arg(vl, int);
          if (num < 0) {
            num = -num;
            if (++pos < n) out[pos-1] = '-';
          }
          long digits = 1;
          for (long nn = num; nn /= 10; digits++)
            ;
          for (int i = digits-1; i >= 0; i--) {
            if (pos + i + 1 < n) out[pos + i] = '0' + (num % 10);
            num /= 10;
          }
          pos += digits;
          longarg = false;
          format = false;
          break;
        }
        case 's':
        {
          const char* s2 = va_arg(vl, const char*);
          while (*s2) {
            if (++pos < n)
              out[pos-1] = *s2;
            s2++;
          }
          longarg = false;
          format = false;
          break;
        }
        case 'c':
        {
          if (++pos < n) out[pos-1] = (char)va_arg(vl,int);
          longarg = false;
          format = false;
          break;
        }
        default:
          break;
      }
    }
    else if(*s == '%')
      format = true;
    else
      if (++pos < n) out[pos-1] = *s;
  }
  if (pos < n)
    out[pos] = 0;
  else if (n)
    out[n-1] = 0;
  return pos;
}

int vprintm(const char* s, va_list vl)
{
  char buf[256];
  int ret = vsnprint(buf, sizeof buf, s, vl);
  putstring(buf);
  return ret;
}

int printm(const char* s, ...)
{
  va_list vl;

  va_start(vl, s);
  int ret = vprintm(s, vl);
  va_end(vl);
  return ret;
}

void* memcpy(void* dest, const void* src, size_t len)
{
  const char* s = src;
  char *d = dest;

  if ((((uintptr_t)dest | (uintptr_t)src) & (sizeof(uintptr_t)-1)) == 0) {
    while ((void*)d < (dest + len - (sizeof(uintptr_t)-1))) {
      *(uintptr_t*)d = *(const uintptr_t*)s;
      d += sizeof(uintptr_t);
      s += sizeof(uintptr_t);
    }
  }

  while (d < (char*)(dest + len))
    *d++ = *s++;

  return dest;
}

void* memset(void* dest, int byte, size_t len)
{
  if ((((uintptr_t)dest | len) & (sizeof(uintptr_t)-1)) == 0) {
    uintptr_t word = byte & 0xFF;
    word |= word << 8;
    word |= word << 16;
    word |= word << 16 << 16;

    uintptr_t *d = dest;
    while (d < (uintptr_t*)(dest + len))
      *d++ = word;
  } else {
    char *d = dest;
    while (d < (char*)(dest + len))
      *d++ = byte;
  }
  return dest;
}

#endif // _REFSIDRV_IO_IMPL_H

