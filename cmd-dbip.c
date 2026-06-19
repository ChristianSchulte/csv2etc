
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

#include "cmd-dbip.h"
#include "csv2etc.h"
#include "heap.h"
#include "map.h"
#include "optparse.h"
#include "proc.h"
#include "string.h"

#include <errno.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

struct dbip_record {
  struct String *cc;
  struct String *asn;
  uint32_t mark;
  char comment[CELLVALUE_MAX + 1];
  char ip4_begin[CELLVALUE_MAX + 1];
  char ip4_end[CELLVALUE_MAX + 1];
  char ip6_begin[CELLVALUE_MAX + 1];
  char ip6_end[CELLVALUE_MAX + 1];
};

struct dbip_nftables {
  FILE *f_def;
  FILE *f_ip4;
  FILE *f_ip6;
  struct Map *restrict ids;
  const char *out_nm;
  const char *dbip_nm;
  char f_def_nm[FILENAME_MAX + 1];
  char f_ip4_nm[FILENAME_MAX + 1];
  char f_ip6_nm[FILENAME_MAX + 1];
  struct dbip_record r;
};

static void *dbip_asn_nftables_open(const struct csv2etc *restrict const,
                                    int argc, char *argv[]);
static int dbip_asn_nftables_read(const struct csv2etc *restrict const,
                                  void *restrict const,
                                  const struct cell *restrict const);
static int dbip_asn_nftables_write(const struct csv2etc *restrict const,
                                   void *restrict const);

static void *dbip_country_nftables_open(const struct csv2etc *restrict const,
                                        int argc, char *argv[]);
static int dbip_country_nftables_read(const struct csv2etc *restrict const,
                                      void *restrict const,
                                      const struct cell *restrict const);
static int dbip_country_nftables_write(const struct csv2etc *restrict const,
                                       void *restrict const);

static int dbip_nftables_close(const struct csv2etc *restrict const,
                               void *restrict const);

static const struct cmd_ops dbip_asn_ops[] = {
    {
        .nm = "nftables",
        .usage = "[-n name] [-o output-directory]",
        .open = dbip_asn_nftables_open,
        .close = dbip_nftables_close,
        .read = dbip_asn_nftables_read,
        .write = dbip_asn_nftables_write,
    },
    {NULL}};

static const struct cmd_ops dbip_country_ops[] = {
    {
        .nm = "nftables",
        .usage = "[-n name] [-o output-directory]",
        .open = dbip_country_nftables_open,
        .close = dbip_nftables_close,
        .read = dbip_country_nftables_read,
        .write = dbip_country_nftables_write,
    },
    {NULL}};

const struct cmd cmd_dbip[] = {{
                                   .nm = "dbip-asn",
                                   .ops = dbip_asn_ops,
                               },
                               {
                                   .nm = "dbip-country",
                                   .ops = dbip_country_ops,
                               },
                               {NULL}};

inline static FILE *fopn(const char *restrict const nm,
                         const char *restrict const mode) {
  FILE *restrict f;

  if ((f = fopen(nm, mode)) == NULL)
    werr("%s: %s\n", nm, strerror(errno));

  return f;
}

inline static int fcls(const char *restrict const nm, FILE *restrict const f) {
  if (f != NULL) {
    if (ferror(f)) {
      werr("%s: I/O error\n", nm);
      return -1;
    }

    if (fclose(f) == EOF) {
      werr("%s: %s\n", nm, strerror(errno));
      return -1;
    }
  }

  return 0;
}

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

static int dbip_nftables_open(const struct csv2etc *restrict const ctx,
                              struct dbip_nftables *restrict const cmd_ctx,
                              const char *restrict const dbip_nm, int argc,
                              char *argv[]) {
  int ch;
  struct optparse options = {0};
  optparse_init(&options, argv);
  options.permute = false;

  while ((ch = optparse(&options, "n:o:")) != -1) {
    switch (ch) {
    case 'n':
      cmd_ctx->dbip_nm = options.optarg;
      break;
    case 'o':
      cmd_ctx->out_nm = options.optarg;
      break;
    default:
      werr("%s: Illegal option: %c\n", argv[0], ch);
      break;
    }
  }

  if (cmd_ctx->out_nm != NULL) {
    sfmt(cmd_ctx->f_def_nm, sizeof(cmd_ctx->f_def_nm), "%s/%s-%s-def.nft",
         cmd_ctx->out_nm, cmd_ctx->dbip_nm ? cmd_ctx->dbip_nm : "dbip",
         dbip_nm);

    sfmt(cmd_ctx->f_ip4_nm, sizeof(cmd_ctx->f_ip4_nm), "%s/%s-%s-ipv4.nft",
         cmd_ctx->out_nm, cmd_ctx->dbip_nm ? cmd_ctx->dbip_nm : "dbip",
         dbip_nm);

    sfmt(cmd_ctx->f_ip6_nm, sizeof(cmd_ctx->f_ip6_nm), "%s/%s-%s-ipv6.nft",
         cmd_ctx->out_nm, cmd_ctx->dbip_nm ? cmd_ctx->dbip_nm : "dbip",
         dbip_nm);

  } else {
    sfmt(cmd_ctx->f_def_nm, sizeof(cmd_ctx->f_def_nm), "%s-%s-def.nft",
         cmd_ctx->dbip_nm ? cmd_ctx->dbip_nm : "dbip", dbip_nm);

    sfmt(cmd_ctx->f_ip4_nm, sizeof(cmd_ctx->f_ip4_nm), "%s-%s-ipv4.nft",
         cmd_ctx->dbip_nm ? cmd_ctx->dbip_nm : "dbip", dbip_nm);

    sfmt(cmd_ctx->f_ip6_nm, sizeof(cmd_ctx->f_ip6_nm), "%s-%s-ipv6.nft",
         cmd_ctx->dbip_nm ? cmd_ctx->dbip_nm : "dbip", dbip_nm);
  }

  if ((cmd_ctx->f_def = fopn(cmd_ctx->f_def_nm, "w")) == NULL)
    return -1;

  if ((cmd_ctx->f_ip4 = fopn(cmd_ctx->f_ip4_nm, "w")) == NULL)
    return -1;

  if ((cmd_ctx->f_ip6 = fopn(cmd_ctx->f_ip6_nm, "w")) == NULL)
    return -1;

  fprintf(cmd_ctx->f_ip4,
          "map %s-ipv4-%s {\n\ttype ipv4_addr : mark\n\tflags "
          "interval\n\telements = {\n",
          cmd_ctx->dbip_nm ? cmd_ctx->dbip_nm : "dbip", dbip_nm);

  fprintf(cmd_ctx->f_ip6,
          "map %s-ipv6-%s {\n\ttype ipv6_addr : mark\n\tflags "
          "interval\n\telements = {\n",
          cmd_ctx->dbip_nm ? cmd_ctx->dbip_nm : "dbip", dbip_nm);

  cmd_ctx->ids = Map_new(StringMapOps, 1048576);
  return 0;
}

static int dbip_nftables_close(const struct csv2etc *restrict const ctx,
                               void *restrict const cmd_ctx) {
  struct dbip_nftables *restrict const c_ctx = cmd_ctx;
  int r = 0;

  if (cmd_ctx == NULL)
    return -1;

  if (c_ctx->f_ip4 != NULL)
    fprintf(c_ctx->f_ip4, "\t}\n}\n");

  if (c_ctx->f_ip6 != NULL)
    fprintf(c_ctx->f_ip6, "\t}\n}\n");

  if (fcls(c_ctx->f_def_nm, c_ctx->f_def))
    r = -1;

  if (fcls(c_ctx->f_ip4_nm, c_ctx->f_ip4))
    r = -1;

  if (fcls(c_ctx->f_ip6_nm, c_ctx->f_ip6))
    r = -1;

  if (!ctx->quiet)
    fprintf(stdout, "%s\n%s\n%s\n", c_ctx->f_def_nm, c_ctx->f_ip4_nm,
            c_ctx->f_ip6_nm);

  Map_delete(c_ctx->ids, NULL);
  heap_free(cmd_ctx);
  return r;
}

static void *dbip_asn_nftables_open(const struct csv2etc *restrict const ctx,
                                    int argc, char *argv[]) {
  struct dbip_nftables *restrict const cmd_ctx =
      heap_calloc(1, sizeof(struct dbip_nftables));

  if (dbip_nftables_open(ctx, cmd_ctx, "asn", argc, argv)) {
    heap_free(cmd_ctx);
    return NULL;
  }

  return cmd_ctx;
}

static void *
dbip_country_nftables_open(const struct csv2etc *restrict const ctx, int argc,
                           char *argv[]) {
  struct dbip_nftables *restrict const cmd_ctx =
      heap_calloc(1, sizeof(struct dbip_nftables));

  if (dbip_nftables_open(ctx, cmd_ctx, "country", argc, argv)) {
    heap_free(cmd_ctx);
    return NULL;
  }

  return cmd_ctx;
}

static int dbip_asn_nftables_read(const struct csv2etc *restrict const ctx,
                                  void *restrict const cmd_ctx,
                                  const struct cell *restrict const cell) {
  struct dbip_record *restrict const r = &((struct dbip_nftables *)cmd_ctx)->r;

  switch (cell->col) {
  case 0:
    if (strchr(cell->val, ':') != NULL)
      scopy(r->ip6_begin, cell->val, sizeof(r->ip6_begin) - 1, cell->len);
    else
      scopy(r->ip4_begin, cell->val, sizeof(r->ip4_begin) - 1, cell->len);
    break;
  case 1:
    if (strchr(cell->val, ':') != NULL)
      scopy(r->ip6_end, cell->val, sizeof(r->ip6_end) - 1, cell->len);
    else
      scopy(r->ip4_end, cell->val, sizeof(r->ip4_end) - 1, cell->len);
    break;
  case 2:
    r->asn = String_cnew(cell->val);
    break;
  case 3:
    scopy(r->comment, cell->val, sizeof(r->comment) - 1, cell->len);
    break;
  default:
    String_delete(r->asn);
    werr("%s: %zu: %zu: Too many columns: %s\n", ctx->in_nm, cell->row,
         cell->col, cell->val);
    return -1;
  }

  return 0;
}

static int dbip_asn_nftables_write(const struct csv2etc *restrict const ctx,
                                   void *restrict const cmd_ctx) {
  struct dbip_nftables *restrict const c_ctx = cmd_ctx;
  int r = -1;
  if (c_ctx->r.asn == NULL) {
    werr("%s: %zu: No system number: %s\n", ctx->in_nm, ctx->row, ctx->line);
    goto err;
  }

  if (Map_get(c_ctx->ids, c_ctx->r.asn) == NULL) {
    fprintf(c_ctx->f_def, "# %s\ndefine AS%s = %s\n", c_ctx->r.comment,
            String_chars(c_ctx->r.asn), String_chars(c_ctx->r.asn));

    Map_put(c_ctx->ids, c_ctx->r.asn, c_ctx->r.asn);
  }

  if (*c_ctx->r.ip4_begin && *c_ctx->r.ip4_end) {
    fprintf(c_ctx->f_ip4, "\t\t%s-%s\t:\t$AS%s,\n", c_ctx->r.ip4_begin,
            c_ctx->r.ip4_end, String_chars(c_ctx->r.asn));
  } else if (*c_ctx->r.ip6_begin && *c_ctx->r.ip6_end) {
    fprintf(c_ctx->f_ip6, "\t\t%s-%s\t:\t$AS%s,\n", c_ctx->r.ip6_begin,
            c_ctx->r.ip6_end, String_chars(c_ctx->r.asn));
  } else {
    werr("%s: :%zu: No IP range: %s\n", ctx->in_nm, ctx->row, ctx->line);
    goto err;
  }

  r = 0;
err:
  c_ctx->r.mark = 0;
  *c_ctx->r.comment = '\0';
  *c_ctx->r.ip4_begin = '\0';
  *c_ctx->r.ip4_end = '\0';
  *c_ctx->r.ip6_begin = '\0';
  *c_ctx->r.ip6_end = '\0';
  String_delete(c_ctx->r.asn);
  c_ctx->r.asn = NULL;
  return r;
}

static int dbip_country_nftables_read(const struct csv2etc *restrict const ctx,
                                      void *restrict const cmd_ctx,
                                      const struct cell *restrict const cell) {
  struct dbip_record *restrict const r = &((struct dbip_nftables *)cmd_ctx)->r;

  switch (cell->col) {
  case 0:
    if (strchr(cell->val, ':') != NULL)
      scopy(r->ip6_begin, cell->val, sizeof(r->ip6_begin) - 1, cell->len);
    else
      scopy(r->ip4_begin, cell->val, sizeof(r->ip4_begin) - 1, cell->len);
    break;
  case 1:
    if (strchr(cell->val, ':') != NULL)
      scopy(r->ip6_end, cell->val, sizeof(r->ip6_end) - 1, cell->len);
    else
      scopy(r->ip4_end, cell->val, sizeof(r->ip4_end) - 1, cell->len);
    break;
  case 2:
    if (cell->len > 4) {
      werr("%s: %zu: %zu: Country code too long: %s\n", ctx->in_nm, cell->row,
           cell->col, cell->val);
      return -1;
    }

    r->cc = String_cnew(cell->val);
    r->mark = 0;

    const char *restrict v_p = cell->val;
    while (*v_p)
      r->mark = (r->mark << 8) | (*v_p++ & 0xff);

    break;
  default:
    String_delete(r->cc);
    werr("%s: %zu: %zu: Too many columns: %s\n", ctx->in_nm, cell->row,
         cell->col, cell->val);
    return -1;
  }

  return 0;
}

static int dbip_country_nftables_write(const struct csv2etc *restrict const ctx,
                                       void *restrict const cmd_ctx) {
  struct dbip_nftables *restrict const c_ctx = cmd_ctx;
  int r = -1;

  if (c_ctx->r.cc == NULL) {
    werr("%s: %zu: No country code: %s\n", ctx->in_nm, ctx->row, ctx->line);
    goto err;
  }

  if (Map_get(c_ctx->ids, c_ctx->r.cc) == NULL) {
    fprintf(c_ctx->f_def, "define %s = %u\n", String_chars(c_ctx->r.cc),
            c_ctx->r.mark);
    Map_put(c_ctx->ids, c_ctx->r.cc, c_ctx->r.cc);
  }

  if (*c_ctx->r.ip4_begin && *c_ctx->r.ip4_end) {
    fprintf(c_ctx->f_ip4, "\t\t%s-%s\t:\t$%s,\n", c_ctx->r.ip4_begin,
            c_ctx->r.ip4_end, String_chars(c_ctx->r.cc));
  } else if (*c_ctx->r.ip6_begin && *c_ctx->r.ip6_end) {
    fprintf(c_ctx->f_ip6, "\t\t%s-%s\t:\t$%s,\n", c_ctx->r.ip6_begin,
            c_ctx->r.ip6_end, String_chars(c_ctx->r.cc));
  } else {
    werr("%s: :%zu: No IP range: %s\n", ctx->in_nm, ctx->row, ctx->line);
    goto err;
  }

  r = 0;
err:
  c_ctx->r.mark = 0;
  *c_ctx->r.comment = '\0';
  *c_ctx->r.ip4_begin = '\0';
  *c_ctx->r.ip4_end = '\0';
  *c_ctx->r.ip6_begin = '\0';
  *c_ctx->r.ip6_end = '\0';
  String_delete(c_ctx->r.cc);
  c_ctx->r.cc = NULL;
  return r;
}
