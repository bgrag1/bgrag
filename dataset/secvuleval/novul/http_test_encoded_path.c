static inline int http_test_encoded_path(const char *mem, size_t len) {
  const char *pos = NULL;
  const char *end = mem + len;
  while (mem < end && (pos = memchr(mem, '/', (size_t)len))) {
    len = end - pos;
    mem = pos + 1;
    if (pos[1] == '/')
      return -1;
    if (len > 3 && pos[1] == '.' && pos[2] == '.' && pos[3] == '/')
      return -1;
  }
  return 0;
}