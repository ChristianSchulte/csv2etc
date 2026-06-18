
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
#include "proc.h"
#include "string.h"

#define OPTPARSE_IMPLEMENTATION
#include "optparse.h"

#include <errno.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

const char *restrict progname;
_Atomic bool terminated;

static size_t scan_quoted(char *restrict out, size_t *out_len,
                          const struct csv2etc *restrict const ctx,
                          const struct cell *restrict const cell,
                          const char *restrict const in) {
  const char *in_p = in;
  char *out_p = out;
  bool unmatched = true;

  while ((*out_len)-- != 0 && *in_p) {
    if (*in_p == '\"') {
      if (*(in_p + 1) == '\"') {
        *out_p++ = *in_p++;
        in_p++;
        continue;
      }
      in_p++;
      unmatched = false;
      break;
    } else
      *out_p++ = *in_p++;
  }

  if (*out_len == SIZE_MAX) {
    werr("%s: %zu: %zu: Value too long: %s\n", ctx->in_nm, cell->row, cell->col,
         in);
    return 0;
  }

  *out_p = '\0';
  *out_len = out_p - out;

  if (unmatched) {
    werr("%s: %zu: %zu: Umatched quotes: %s\n", ctx->in_nm, cell->row,
         cell->col, in);
    return 0;
  }

  return in_p - in;
}

static int scan_line(const struct csv2etc *restrict const ctx,
                     void *restrict const cmd_ctx,
                     const struct cmd_ops *restrict const cmd_ops) {
  char val[CELLVALUE_MAX + 1] = {0};
  char *start = ctx->line, *end = NULL;
  struct cell cell = {0};
  cell.row = ctx->row;

  while (start != NULL) {
    if (*start == '\"') {
      size_t v_len = sizeof(val) - 1;
      size_t i_len = scan_quoted(val, &v_len, ctx, &cell, ++start);

      if (i_len == 0 && v_len != 0)
        return -1;

      cell.val = val;
      cell.len = v_len;

      if (cmd_ops->read(ctx, cmd_ctx, &cell))
        return -1;

      cell.col++;
      end = start + i_len;
      start = *end ? end + 1 : NULL;
      continue;
    } else
      end = strchr(start, ctx->separator);

    cell.val = start;

    if (end != NULL) {
      *end = '\0';
      cell.len = end - start;
    } else
      cell.len = strlen(start);

    if (cell.len > sizeof(val) - 1) {
      werr("%s: %zu: %zu: Value too long: %s\n", ctx->in_nm, cell.row, cell.col,
           start);
      return -1;
    }

    if (cmd_ops->read(ctx, cmd_ctx, &cell))
      return -1;

    cell.col++;
    start = end != NULL ? end + 1 : NULL;
  }

  return 0;
}

inline static const struct cmd *cmd_match(const struct cmd *restrict cmd,
                                          const char *restrict const nm) {
  while (cmd->nm != NULL) {
    if (!strcmp(cmd->nm, nm))
      return cmd;
    cmd++;
  }

  return NULL;
}

inline static const struct cmd_ops *
cmd_ops_match(const struct cmd_ops *restrict cmd_ops,
              const char *restrict const nm) {
  while (cmd_ops->nm != NULL) {
    if (!strcmp(cmd_ops->nm, nm))
      return cmd_ops;
    cmd_ops++;
  }

  return NULL;
}

static void cmd_usage(const struct cmd *restrict cmd) {
  while (cmd->nm != NULL) {
    const struct cmd_ops *restrict cmd_ops = cmd->ops;

    werr("\t%s\n", cmd->nm);

    while (cmd_ops->nm != NULL) {
      werr("\t\t%s", cmd_ops->nm);
      if (cmd_ops->usage)
        werr(" %s", cmd_ops->usage);
      werr("\n");
      cmd_ops++;
    }

    cmd++;
  }
}

static _Noreturn void usage(void) {
  werr("Usage: %s command [-q] [-i input-file] [-s separator]\n", progname);
  cmd_usage(cmd_dbip);
  exit(EXIT_FAILURE);
}

static int csv2etc(int argc, char *argv[]) {
  int ch, r = EXIT_FAILURE;
  struct optparse options = {0};
  struct csv2etc ctx = {0};
  const struct cmd *restrict cmd = NULL;
  void *restrict cmd_ctx = NULL;
  const struct cmd_ops *restrict cmd_ops = NULL;
  FILE *restrict f_csv = stdin;
  char buf[BUFSIZ];

  ctx.separator = ',';

  optparse_init(&options, argv);
  options.permute = false;

  if (argv[0] == NULL)
    usage();

  cmd = cmd_match(cmd_dbip, argv[0]);

  if (cmd == NULL)
    usage();

  while ((ch = optparse(&options, "i:qs:")) != -1) {
    switch (ch) {
    case 'i':
      ctx.in_nm = options.optarg;
      break;
    case 'q':
      ctx.quiet = true;
      break;
    case 's':
      ctx.separator = *options.optarg;
      break;
    default:
      usage();
    }
  }

  argc -= options.optind;
  argv += options.optind;

  if (argv[0] == NULL)
    usage();

  cmd_ops = cmd_ops_match(cmd->ops, argv[0]);

  if (cmd_ops == NULL)
    usage();

  if (ctx.in_nm != NULL) {
    f_csv = fopen(ctx.in_nm, "r");

    if (f_csv == NULL) {
      werr("%s: %s\n", ctx.in_nm, strerror(errno));
      goto err;
    }
  } else
    ctx.in_nm = "-";

  cmd_ctx = cmd_ops->open(&ctx, argc, argv);
  if (cmd_ctx == NULL)
    goto err;

  ctx.row = 0;
  while (fgets(buf, sizeof(buf), f_csv) != NULL && !terminated) {
    buf[strcspn(buf, "\r\n")] = '\0';

    ctx.line = buf;
    ctx.row++;

    if (scan_line(&ctx, cmd_ctx, cmd_ops))
      goto err;

    if (cmd_ops->write(&ctx, cmd_ctx))
      goto err;
  }

  r = EXIT_SUCCESS;
err:
  if (cmd_ops->close(&ctx, cmd_ctx))
    r = EXIT_FAILURE;

  if (f_csv != NULL) {
    if (ferror(f_csv)) {
      werr("%s: I/O error\n", ctx.in_nm);
      r = EXIT_FAILURE;
    }

    if (fclose(f_csv) == EOF) {
      werr("%s: %s\n", ctx.in_nm, strerror(errno));
      r = EXIT_FAILURE;
    }
  }

  return r;
}

static void terminate(int signum) { terminated = true; }

int main(int argc, char *argv[]) {
  if (signal(SIGTERM, terminate) == SIG_ERR)
    fatal("%s", strerror(errno));

  if (signal(SIGINT, terminate) == SIG_ERR)
    fatal("%s", strerror(errno));

  if (argv[0] != NULL) {
    char *p_nm = strrchr(argv[0], '/');

    if (p_nm == NULL)
      p_nm = strrchr(argv[0], '\\');

    progname = p_nm != NULL ? p_nm + 1 : argv[0];
  } else
    progname = ".";

  return csv2etc(argc - 1, argv + 1);
}
