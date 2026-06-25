
/*
 * Copyright (c) 2026 Christian Schulte <cs@schulte.it>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#ifdef HAVE_HOST_H
#include "host.h"
#endif

#include "cmd-ip2location.h"
#include "csv2etc.h"
#include "heap.h"
#include "optparse.h"
#include "proc.h"

#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <math.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define nitems(a) (sizeof((a)) / sizeof((a)[0]))

enum ip2loc_type { IPV4 = 1, IPV6 };

struct ip2loc_record {
  char id[CELLVALUE_MAX + 1];
  char ip_begin[CELLVALUE_MAX + 1];
  char ip_end[CELLVALUE_MAX + 1];
};

struct ip2loc_dbip {
  FILE *f_out;
  const char *out_nm;
  enum ip2loc_type type;
  struct ip2loc_record r;
};

static void *ip2loc_dbip_ipv4_open(const struct csv2etc *restrict const,
                                   int argc, char *argv[]);
static void *ip2loc_dbip_ipv6_open(const struct csv2etc *restrict const,
                                   int argc, char *argv[]);
static int ip2loc_dbip_close(const struct csv2etc *restrict const,
                             void *restrict const);

static int ip2loc_asn_dbip_read(const struct csv2etc *restrict const,
                                void *restrict const,
                                const struct cell *restrict const);
static int ip2loc_country_dbip_read(const struct csv2etc *restrict const,
                                    void *restrict const,
                                    const struct cell *restrict const);
static int ip2loc_dbip_write(const struct csv2etc *restrict const,
                             void *restrict const);

static const struct cmd_ops ip2loc_asn_ipv4_ops[] = {
    {
        .nm = "dbip-asn",
        .usage = "[-o output-file]",
        .open = ip2loc_dbip_ipv4_open,
        .close = ip2loc_dbip_close,
        .read = ip2loc_asn_dbip_read,
        .write = ip2loc_dbip_write,
    },
    {NULL}};

static const struct cmd_ops ip2loc_asn_ipv6_ops[] = {
    {
        .nm = "dbip-asn",
        .usage = "[-o output-file]",
        .open = ip2loc_dbip_ipv6_open,
        .close = ip2loc_dbip_close,
        .read = ip2loc_asn_dbip_read,
        .write = ip2loc_dbip_write,
    },
    {NULL}};

static const struct cmd_ops ip2loc_country_ipv4_ops[] = {
    {
        .nm = "dbip-country",
        .usage = "[-o output-file]",
        .open = ip2loc_dbip_ipv4_open,
        .close = ip2loc_dbip_close,
        .read = ip2loc_country_dbip_read,
        .write = ip2loc_dbip_write,
    },
    {NULL}};

static const struct cmd_ops ip2loc_country_ipv6_ops[] = {
    {
        .nm = "dbip-country",
        .usage = "[-o output-file]",
        .open = ip2loc_dbip_ipv6_open,
        .close = ip2loc_dbip_close,
        .read = ip2loc_country_dbip_read,
        .write = ip2loc_dbip_write,
    },
    {NULL}};

const struct cmd cmd_ip2location[] = {{
                                          .nm = "ip2location-asn-ipv4",
                                          .ops = ip2loc_asn_ipv4_ops,
                                      },
                                      {
                                          .nm = "ip2location-asn-ipv6",
                                          .ops = ip2loc_asn_ipv6_ops,
                                      },
                                      {
                                          .nm = "ip2location-ipv4",
                                          .ops = ip2loc_country_ipv4_ops,
                                      },
                                      {
                                          .nm = "ip2location-ipv6",
                                          .ops = ip2loc_country_ipv6_ops,
                                      },
                                      {NULL}};

__attribute__((__format__(printf, 3, 4))) inline static void
sfmt(char *restrict const str, const size_t size, const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  int r = vsnprintf(str, size, fmt, ap);
  va_end(ap);
  if (r < 0 || (size_t)r >= size)
    panic();
}

inline static void *scopy(char *restrict dst, const char *restrict src,
                          size_t dst_len, size_t src_len) {
  if (src_len > dst_len)
    panic();

  memcpy(dst, src, src_len);
  dst[src_len] = '\0';
  return dst;
}

static int stoipv4(unsigned long *restrict ip, const char *restrict s) {
  char *ep;

  errno = 0;
  const unsigned long long ip64 = strtoull(s, &ep, 10);

  if (s[0] == '\0' || *ep != '\0')
    return -1;
  if (errno == ERANGE && ip64 == ULLONG_MAX)
    return -1;
  if (ip64 > UINT32_MAX)
    return -1;

  *ip = (unsigned long)ip64;
  return 0;
}

inline static int mul10add(uintptr_t *restrict n, const size_t items,
                           uintptr_t c) {
#define BITS (sizeof(uintptr_t) * 8)
  for (size_t i = items; i-- > 0;) {
    const uintptr_t x2_msb = n[i] >> (BITS - 1);
    const uintptr_t x8_msb = n[i] >> (BITS - 3);
    const uintptr_t x2_lsb = n[i] << 1;
    const uintptr_t x8_lsb = n[i] << 3;
    const uintptr_t x10_lsb = x2_lsb + x8_lsb;
    const uintptr_t x10_msb = x2_msb + x8_msb + (x10_lsb < x2_lsb);
    n[i] = x10_lsb + c;
    c = x10_msb + (n[i] < c);
  }

  return c ? -1 : 0;
}

static int stoipv6(uint8_t *restrict ip, size_t items, const char *restrict s,
                   size_t len) {
  const char *restrict p = s;
  uintptr_t n[sizeof(uint8_t) * items / sizeof(uintptr_t)];
  const size_t n_items = nitems(n);

  memset(n, 0, sizeof(n));

  while (len-- != 0) {
    uintptr_t d = *p++ - '0';

    if (d > 9)
      return -1;

    if (mul10add(n, n_items, d))
      return -1;
  }

  for (size_t i = 0; i < n_items; i++)
    for (size_t b = sizeof(uintptr_t); b-- > 0;)
      *ip++ = (uint8_t)(n[i] >> (b * 8));

  return 0;
}

static int ip2loc_dbip_open(const struct csv2etc *restrict const ctx,
                            struct ip2loc_dbip *restrict const cmd_ctx,
                            int argc, char *argv[]) {
  int ch;
  struct optparse options = {0};
  optparse_init(&options, argv);
  options.permute = false;

  while ((ch = optparse(&options, "o:")) != -1) {
    switch (ch) {
    case 'o':
      cmd_ctx->out_nm = options.optarg;
      break;
    default:
      werr("%s: Illegal option: %c\n", argv[0], ch);
      break;
    }
  }

  if (cmd_ctx->out_nm != NULL) {
    if ((cmd_ctx->f_out = fopen(cmd_ctx->out_nm, "w")) == NULL) {
      werr("%s: %s\n", cmd_ctx->out_nm, strerror(errno));
      return -1;
    }
  } else {
    cmd_ctx->f_out = stdout;
    cmd_ctx->out_nm = "-";
  }

  return 0;
}

static int ip2loc_dbip_close(const struct csv2etc *restrict const ctx,
                             void *restrict const cmd_ctx) {
  struct ip2loc_dbip *restrict const c_ctx = cmd_ctx;
  int r = 0;

  if (cmd_ctx == NULL)
    return -1;

  if (c_ctx->f_out != NULL) {
    if (ferror(c_ctx->f_out)) {
      werr("%s: I/O error\n", c_ctx->out_nm);
      return -1;
    }

    if (fclose(c_ctx->f_out) == EOF) {
      werr("%s: %s\n", c_ctx->out_nm, strerror(errno));
      return -1;
    }
  }

  heap_free(cmd_ctx);
  return r;
}

static void *ip2loc_dbip_ipv4_open(const struct csv2etc *restrict const ctx,
                                   int argc, char *argv[]) {
  struct ip2loc_dbip *restrict const cmd_ctx =
      heap_calloc(1, sizeof(struct ip2loc_dbip));

  cmd_ctx->type = IPV4;

  if (ip2loc_dbip_open(ctx, cmd_ctx, argc, argv)) {
    heap_free(cmd_ctx);
    return NULL;
  }

  return cmd_ctx;
}

static void *ip2loc_dbip_ipv6_open(const struct csv2etc *restrict const ctx,
                                   int argc, char *argv[]) {
  struct ip2loc_dbip *restrict const cmd_ctx =
      heap_calloc(1, sizeof(struct ip2loc_dbip));

  cmd_ctx->type = IPV6;

  if (ip2loc_dbip_open(ctx, cmd_ctx, argc, argv)) {
    heap_free(cmd_ctx);
    return NULL;
  }

  return cmd_ctx;
}

static int ip2loc_dbip_read(const struct csv2etc *restrict const ctx,
                            void *restrict const cmd_ctx,
                            const struct cell *restrict const cell) {
  unsigned long ip4;
  uint8_t ip6[16] = {0};
  struct ip2loc_dbip *restrict const c_ctx = cmd_ctx;

  switch (cell->col) {
  case 0:
    switch (c_ctx->type) {
    case IPV4:
      if (stoipv4(&ip4, cell->val)) {
        werr("%s: %zu: %zu: Not a 32bit IPv4 address: %s\n", ctx->in_nm,
             cell->row, cell->col, cell->val);
        return -1;
      }
      sfmt(c_ctx->r.ip_begin, sizeof(c_ctx->r.ip_begin) - 1, "%lu.%lu.%lu.%lu",
           (ip4 & 0xff000000) >> 24, (ip4 & 0x00ff0000) >> 16,
           (ip4 & 0x0000ff00) >> 8, ip4 & 0xff);
      break;
    case IPV6:
      if (stoipv6(ip6, nitems(ip6), cell->val, cell->len)) {
        werr("%s: %zu: %zu: Not a 128bit IPv6 address: %s\n", ctx->in_nm,
             cell->row, cell->col, cell->val);
        return -1;
      }
      sfmt(c_ctx->r.ip_begin, sizeof(c_ctx->r.ip_begin) - 1,
           "%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%"
           "02x%02x",
           ip6[0], ip6[1], ip6[2], ip6[3], ip6[4], ip6[5], ip6[6], ip6[7],
           ip6[8], ip6[9], ip6[10], ip6[11], ip6[12], ip6[13], ip6[14],
           ip6[15]);
      break;
    default:
      panic();
    }
    break;
  case 1:
    switch (c_ctx->type) {
    case IPV4:
      if (stoipv4(&ip4, cell->val)) {
        werr("%s: %zu: %zu: Not a 32bit IPv4 address: %s\n", ctx->in_nm,
             cell->row, cell->col, cell->val);
        return -1;
      }
      sfmt(c_ctx->r.ip_end, sizeof(c_ctx->r.ip_end) - 1, "%lu.%lu.%lu.%lu",
           (ip4 & 0xff000000) >> 24, (ip4 & 0x00ff0000) >> 16,
           (ip4 & 0x0000ff00) >> 8, ip4 & 0xff);
      break;
    case IPV6:
      if (stoipv6(ip6, nitems(ip6), cell->val, cell->len)) {
        werr("%s: %zu: %zu: Not a 128bit IPv6 address: %s\n", ctx->in_nm,
             cell->row, cell->col, cell->val);
        return -1;
      }
      sfmt(c_ctx->r.ip_end, sizeof(c_ctx->r.ip_end) - 1,
           "%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%"
           "02x%02x",
           ip6[0], ip6[1], ip6[2], ip6[3], ip6[4], ip6[5], ip6[6], ip6[7],
           ip6[8], ip6[9], ip6[10], ip6[11], ip6[12], ip6[13], ip6[14],
           ip6[15]);
      break;
    default:
      panic();
    }
    break;
  default:
    break;
  }

  return 0;
}

static int ip2loc_asn_dbip_read(const struct csv2etc *restrict const ctx,
                                void *restrict const cmd_ctx,
                                const struct cell *restrict const cell) {
  struct ip2loc_dbip *restrict const c_ctx = cmd_ctx;

  if (ip2loc_dbip_read(ctx, cmd_ctx, cell))
    return -1;

  switch (cell->col) {
  case 3:
    if (isdigit(*cell->val))
      scopy(c_ctx->r.id, cell->val, sizeof(c_ctx->r.id) - 1, cell->len);
    break;
  default:
    break;
  }

  return 0;
}

static int ip2loc_country_dbip_read(const struct csv2etc *restrict const ctx,
                                    void *restrict const cmd_ctx,
                                    const struct cell *restrict const cell) {
  struct ip2loc_dbip *restrict const c_ctx = cmd_ctx;

  if (ip2loc_dbip_read(ctx, cmd_ctx, cell))
    return -1;

  switch (cell->col) {
  case 2:
    if (isalpha(*cell->val))
      scopy(c_ctx->r.id, cell->val, sizeof(c_ctx->r.id) - 1, cell->len);
    break;
  default:
    break;
  }

  return 0;
}

static int ip2loc_dbip_write(const struct csv2etc *restrict const ctx,
                             void *restrict const cmd_ctx) {
  struct ip2loc_dbip *restrict const c_ctx = cmd_ctx;
  int r = -1;

  if (*c_ctx->r.id) {
    if (*c_ctx->r.ip_begin && *c_ctx->r.ip_end) {
      fprintf(c_ctx->f_out, "%s,%s,%s\n", c_ctx->r.ip_begin, c_ctx->r.ip_end,
              c_ctx->r.id);
    } else {
      werr("%s: :%zu: No IP range: %s\n", ctx->in_nm, ctx->row, ctx->line);
      goto err;
    }
  }

  r = 0;
err:
  *c_ctx->r.id = '\0';
  *c_ctx->r.ip_begin = '\0';
  *c_ctx->r.ip_end = '\0';
  return r;
}
