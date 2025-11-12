ares_status_t ares__read_line(FILE *fp, char **buf, size_t *bufsize)
{
  char  *newbuf;
  size_t offset = 0;
  size_t len;

  if (*buf == NULL) {
    *buf = ares_malloc(128);
    if (!*buf) {
      return ARES_ENOMEM;
    }
    *bufsize = 128;
  }

  for (;;) {
    int bytestoread = (int)(*bufsize - offset);

    if (!fgets(*buf + offset, bytestoread, fp)) {
      return (offset != 0) ? 0 : (ferror(fp)) ? ARES_EFILE : ARES_EOF;
    }
    len = offset + ares_strlen(*buf + offset);

    /* Probably means there was an embedded NULL as the first character in
     * the line, throw away line */
    if (len == 0) {
      offset = 0;
      continue;
    }

    if ((*buf)[len - 1] == '\n') {
      (*buf)[len - 1] = 0;
      break;
    }
    offset = len;
    if (len < *bufsize - 1) {
      continue;
    }

    /* Allocate more space. */
    newbuf = ares_realloc(*buf, *bufsize * 2);
    if (!newbuf) {
      ares_free(*buf);
      *buf = NULL;
      return ARES_ENOMEM;
    }
    *buf      = newbuf;
    *bufsize *= 2;
  }
  return ARES_SUCCESS;
}