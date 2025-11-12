static void fio_cli_set_arg(cstr_s arg, char const *value, char const *line,
                            fio_cli_parser_data_s *parser) {
  /* handle unnamed argument */
  if (!line || !arg.len) {
    if (!value) {
      goto print_help;
    }
    if (!strcmp(value, "-?") || !strcasecmp(value, "-h") ||
        !strcasecmp(value, "-help") || !strcasecmp(value, "--help")) {
      goto print_help;
    }
    cstr_s n = {.len = ++parser->unnamed_count};
    fio_cli_hash_insert(&fio_values, n.len, n, value, NULL);
    if (parser->unnamed_max >= 0 &&
        parser->unnamed_count > parser->unnamed_max) {
      arg.len = 0;
      goto error;
    }
    return;
  }

  /* validate data types */
  char const *type = fio_cli_get_line_type(parser, line);
  switch ((size_t)type) {
  case FIO_CLI_BOOL__TYPE_I:
    if (value && value != parser->argv[parser->pos + 1]) {
      goto error;
    }
    value = "1";
    break;
  case FIO_CLI_INT__TYPE_I:
    if (value) {
      char const *tmp = value;
      fio_atol((char **)&tmp);
      if (*tmp) {
        goto error;
      }
    }
    /* fallthrough */
  case FIO_CLI_STRING__TYPE_I:
    if (!value)
      goto error;
    if (!value[0])
      goto finish;
    break;
  }

  /* add values using all aliases possible */
  {
    cstr_s n = {.data = line};
    while (n.data[0] == '-') {
      while (n.data[n.len] && n.data[n.len] != ' ' && n.data[n.len] != ',') {
        ++n.len;
      }
      fio_cli_hash_insert(&fio_values, FIO_CLI_HASH_VAL(n), n, value, NULL);
      while (n.data[n.len] && (n.data[n.len] == ' ' || n.data[n.len] == ',')) {
        ++n.len;
      }
      n.data += n.len;
      n.len = 0;
    }
  }

finish:

  /* handle additional argv progress (if value is on separate argv) */
  if (value && parser->pos < parser->argc &&
      value == parser->argv[parser->pos + 1])
    ++parser->pos;
  return;

error: /* handle errors*/
  /* TODO! */
  fprintf(stderr, "\n\r\x1B[31mError:\x1B[0m unknown argument %.*s %s %s\n\n",
          (int)arg.len, arg.data, arg.len ? "with value" : "",
          value ? (value[0] ? value : "(empty)") : "(null)");
print_help:
  fprintf(stderr, "\n%s\n",
          parser->description ? parser->description
                              : "This application accepts any of the following "
                                "possible arguments:");
  /* print out each line's arguments */
  char const **pos = parser->names;
  while (*pos) {
    switch ((intptr_t)*pos) {
    case FIO_CLI_STRING__TYPE_I: /* fallthrough */
    case FIO_CLI_BOOL__TYPE_I:   /* fallthrough */
    case FIO_CLI_INT__TYPE_I:    /* fallthrough */
    case FIO_CLI_PRINT__TYPE_I:  /* fallthrough */
    case FIO_CLI_PRINT_HEADER__TYPE_I:
      ++pos;
      continue;
    }
    type = (char *)FIO_CLI_STRING__TYPE_I;
    switch ((intptr_t)pos[1]) {
    case FIO_CLI_PRINT__TYPE_I:
      fprintf(stderr, "%s\n", pos[0]);
      pos += 2;
      continue;
    case FIO_CLI_PRINT_HEADER__TYPE_I:
      fprintf(stderr, "\n\x1B[4m%s\x1B[0m\n", pos[0]);
      pos += 2;
      continue;

    case FIO_CLI_STRING__TYPE_I: /* fallthrough */
    case FIO_CLI_BOOL__TYPE_I:   /* fallthrough */
    case FIO_CLI_INT__TYPE_I:    /* fallthrough */
      type = pos[1];
    }
    /* print line @ pos, starting with main argument name */
    int alias_count = 0;
    int first_len = 0;
    size_t tmp = 0;
    char const *const p = *pos;
    while (p[tmp] == '-') {
      while (p[tmp] && p[tmp] != ' ' && p[tmp] != ',') {
        if (!alias_count)
          ++first_len;
        ++tmp;
      }
      ++alias_count;
      while (p[tmp] && (p[tmp] == ' ' || p[tmp] == ',')) {
        ++tmp;
      }
    }
    switch ((size_t)type) {
    case FIO_CLI_STRING__TYPE_I:
      fprintf(stderr, " \x1B[1m%.*s\x1B[0m\x1B[2m <>\x1B[0m\t%s\n", first_len,
              p, p + tmp);
      break;
    case FIO_CLI_BOOL__TYPE_I:
      fprintf(stderr, " \x1B[1m%.*s\x1B[0m   \t%s\n", first_len, p, p + tmp);
      break;
    case FIO_CLI_INT__TYPE_I:
      fprintf(stderr, " \x1B[1m%.*s\x1B[0m\x1B[2m ##\x1B[0m\t%s\n", first_len,
              p, p + tmp);
      break;
    }
    /* print aliase information */
    tmp = first_len;
    while (p[tmp] && (p[tmp] == ' ' || p[tmp] == ',')) {
      ++tmp;
    }
    while (p[tmp] == '-') {
      const size_t start = tmp;
      while (p[tmp] && p[tmp] != ' ' && p[tmp] != ',') {
        ++tmp;
      }
      int padding = first_len - (tmp - start);
      if (padding < 0)
        padding = 0;
      switch ((size_t)type) {
      case FIO_CLI_STRING__TYPE_I:
        fprintf(stderr,
                " \x1B[1m%.*s\x1B[0m\x1B[2m <>\x1B[0m%*s\t(same as "
                "\x1B[1m%.*s\x1B[0m)\n",
                (int)(tmp - start), p + start, padding, "", first_len, p);
        break;
      case FIO_CLI_BOOL__TYPE_I:
        fprintf(stderr,
                " \x1B[1m%.*s\x1B[0m   %*s\t(same as \x1B[1m%.*s\x1B[0m)\n",
                (int)(tmp - start), p + start, padding, "", first_len, p);
        break;
      case FIO_CLI_INT__TYPE_I:
        fprintf(stderr,
                " \x1B[1m%.*s\x1B[0m\x1B[2m ##\x1B[0m%*s\t(same as "
                "\x1B[1m%.*s\x1B[0m)\n",
                (int)(tmp - start), p + start, padding, "", first_len, p);
        break;
      }
    }

    ++pos;
  }
  fprintf(stderr, "\nUse any of the following input formats:\n"
                  "\t-arg <value>\t-arg=<value>\t-arg<value>\n"
                  "\n"
                  "Use the -h, -help or -? to get this information again.\n"
                  "\n");
  fio_cli_end();
  exit(0);
}