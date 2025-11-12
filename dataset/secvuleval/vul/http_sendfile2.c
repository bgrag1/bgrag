int http_sendfile2(http_s *h, const char *prefix, size_t prefix_len,
                   const char *encoded, size_t encoded_len) {
  if (HTTP_INVALID_HANDLE(h))
    return -1;
  struct stat file_data = {.st_size = 0};
  static uint64_t accept_enc_hash = 0;
  if (!accept_enc_hash)
    accept_enc_hash = fiobj_hash_string("accept-encoding", 15);
  static uint64_t range_hash = 0;
  if (!range_hash)
    range_hash = fiobj_hash_string("range", 5);

  /* create filename string */
  FIOBJ filename = fiobj_str_tmp();
  if (prefix && prefix_len) {
    /* start with prefix path */
    if (encoded && prefix[prefix_len - 1] == '/' && encoded[0] == '/')
      --prefix_len;
    fiobj_str_capa_assert(filename, prefix_len + encoded_len + 4);
    fiobj_str_write(filename, prefix, prefix_len);
  }
  {
    /* decode filename in cases where it's URL encoded */
    fio_str_info_s tmp = fiobj_obj2cstr(filename);
    if (encoded) {
      char *pos = (char *)encoded;
      const char *end = encoded + encoded_len;
      while (pos < end) {
        /* test for path manipulations while decoding */
        if (*pos == '/' && (pos[1] == '/' ||
                            (((uintptr_t)end - (uintptr_t)pos >= 4) &&
                             pos[1] == '.' && pos[2] == '.' && pos[3] == '/')))
          return -1;
        if (*pos == '%') {
          // decode hex value
          // this is a percent encoded value.
          if (hex2byte((uint8_t *)tmp.data + tmp.len, (uint8_t *)pos + 1))
            return -1;
          tmp.len++;
          pos += 3;
        } else
          tmp.data[tmp.len++] = *(pos++);
      }
      tmp.data[tmp.len] = 0;
      fiobj_str_resize(filename, tmp.len);
    }
    if (tmp.data[tmp.len - 1] == '/')
      fiobj_str_write(filename, "index.html", 10);
  }
  /* test for file existance  */

  int file = -1;
  uint8_t is_gz = 0;

  fio_str_info_s s = fiobj_obj2cstr(filename);
  {
    FIOBJ tmp = fiobj_hash_get2(h->headers, accept_enc_hash);
    if (!tmp)
      goto no_gzip_support;
    fio_str_info_s ac_str = fiobj_obj2cstr(tmp);
    if (!ac_str.data || !strstr(ac_str.data, "gzip"))
      goto no_gzip_support;
    if (s.data[s.len - 3] != '.' || s.data[s.len - 2] != 'g' ||
        s.data[s.len - 1] != 'z') {
      fiobj_str_write(filename, ".gz", 3);
      s = fiobj_obj2cstr(filename);
      if (!stat(s.data, &file_data) &&
          (S_ISREG(file_data.st_mode) || S_ISLNK(file_data.st_mode))) {
        is_gz = 1;
        goto found_file;
      }
      fiobj_str_resize(filename, s.len - 3);
    }
  }
no_gzip_support:
  if (stat(s.data, &file_data) ||
      !(S_ISREG(file_data.st_mode) || S_ISLNK(file_data.st_mode)))
    return -1;
found_file:
  /* set last-modified */
  {
    FIOBJ tmp = fiobj_str_buf(32);
    fiobj_str_resize(
        tmp, http_time2str(fiobj_obj2cstr(tmp).data, file_data.st_mtime));
    http_set_header(h, HTTP_HEADER_LAST_MODIFIED, tmp);
  }
  /* set cache-control */
  http_set_header(h, HTTP_HEADER_CACHE_CONTROL, fiobj_dup(HTTP_HVALUE_MAX_AGE));
  /* set & test etag */
  uint64_t etag = (uint64_t)file_data.st_size;
  etag ^= (uint64_t)file_data.st_mtime;
  etag = fiobj_hash_string(&etag, sizeof(uint64_t));
  FIOBJ etag_str = fiobj_str_buf(32);
  fiobj_str_resize(etag_str,
                   fio_base64_encode(fiobj_obj2cstr(etag_str).data,
                                     (void *)&etag, sizeof(uint64_t)));
  /* set */
  http_set_header(h, HTTP_HEADER_ETAG, etag_str);
  /* test */
  {
    static uint64_t none_match_hash = 0;
    if (!none_match_hash)
      none_match_hash = fiobj_hash_string("if-none-match", 13);
    FIOBJ tmp2 = fiobj_hash_get2(h->headers, none_match_hash);
    if (tmp2 && fiobj_iseq(tmp2, etag_str)) {
      h->status = 304;
      http_finish(h);
      return 0;
    }
  }
  /* handle range requests */
  int64_t offset = 0;
  int64_t length = file_data.st_size;
  {
    static uint64_t ifrange_hash = 0;
    if (!ifrange_hash)
      ifrange_hash = fiobj_hash_string("if-range", 8);
    FIOBJ tmp = fiobj_hash_get2(h->headers, ifrange_hash);
    if (tmp && fiobj_iseq(tmp, etag_str)) {
      fiobj_hash_delete2(h->headers, range_hash);
    } else {
      tmp = fiobj_hash_get2(h->headers, range_hash);
      if (tmp) {
        /* range ahead... */
        if (FIOBJ_TYPE_IS(tmp, FIOBJ_T_ARRAY))
          tmp = fiobj_ary_index(tmp, 0);
        fio_str_info_s range = fiobj_obj2cstr(tmp);
        if (!range.data || memcmp("bytes=", range.data, 6))
          goto open_file;
        char *pos = range.data + 6;
        int64_t start_at = 0, end_at = 0;
        start_at = fio_atol(&pos);
        if (start_at >= file_data.st_size)
          goto open_file;
        if (start_at >= 0) {
          pos++;
          end_at = fio_atol(&pos);
          if (end_at <= 0)
            goto open_file;
        }
        /* we ignore multimple ranges, only responding with the first range. */
        if (start_at < 0) {
          if (0 - start_at < file_data.st_size) {
            offset = file_data.st_size - start_at;
            length = 0 - start_at;
          }
        } else if (end_at) {
          offset = start_at;
          length = end_at - start_at + 1;
          if (length + start_at > file_data.st_size || length <= 0)
            length = length - start_at;
        } else {
          offset = start_at;
          length = length - start_at;
        }
        h->status = 206;

        {
          FIOBJ cranges = fiobj_str_buf(1);
          fiobj_str_printf(cranges, "bytes %lu-%lu/%lu",
                           (unsigned long)start_at,
                           (unsigned long)(start_at + length - 1),
                           (unsigned long)file_data.st_size);
          http_set_header(h, HTTP_HEADER_CONTENT_RANGE, cranges);
        }
        http_set_header(h, HTTP_HEADER_ACCEPT_RANGES,
                        fiobj_dup(HTTP_HVALUE_BYTES));
      }
    }
  }
  /* test for an OPTIONS request or invalid methods */
  s = fiobj_obj2cstr(h->method);
  switch (s.len) {
  case 7:
    if (!strncasecmp("options", s.data, 7)) {
      http_set_header2(h, (fio_str_info_s){.data = (char *)"allow", .len = 5},
                       (fio_str_info_s){.data = (char *)"GET, HEAD", .len = 9});
      h->status = 200;
      http_finish(h);
      return 0;
    }
    break;
  case 3:
    if (!strncasecmp("get", s.data, 3))
      goto open_file;
    break;
  case 4:
    if (!strncasecmp("head", s.data, 4)) {
      http_set_header(h, HTTP_HEADER_CONTENT_LENGTH, fiobj_num_new(length));
      http_finish(h);
      return 0;
    }
    break;
  }
  http_send_error(h, 403);
  return 0;
open_file:
  s = fiobj_obj2cstr(filename);
  file = open(s.data, O_RDONLY);
  if (file == -1) {
    FIO_LOG_ERROR("(HTTP) couldn't open file %s!\n", s.data);
    perror("     ");
    http_send_error(h, 500);
    return 0;
  }
  {
    FIOBJ tmp = 0;
    uintptr_t pos = 0;
    if (is_gz) {
      http_set_header(h, HTTP_HEADER_CONTENT_ENCODING,
                      fiobj_dup(HTTP_HVALUE_GZIP));

      pos = s.len - 4;
      while (pos && s.data[pos] != '.')
        pos--;
      pos++; /* assuming, but that's fine. */
      tmp = http_mimetype_find(s.data + pos, s.len - pos - 3);

    } else {
      pos = s.len - 1;
      while (pos && s.data[pos] != '.')
        pos--;
      pos++; /* assuming, but that's fine. */
      tmp = http_mimetype_find(s.data + pos, s.len - pos);
    }
    if (tmp)
      http_set_header(h, HTTP_HEADER_CONTENT_TYPE, tmp);
  }
  http_sendfile(h, file, length, offset);
  return 0;
}