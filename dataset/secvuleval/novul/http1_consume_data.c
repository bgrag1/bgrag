static inline void http1_consume_data(intptr_t uuid, http1pr_s *p) {
  if (fio_pending(uuid) > 4) {
    goto throttle;
  }
  ssize_t i = 0;
  size_t org_len = p->buf_len;
  int pipeline_limit = 8;
  if (!p->buf_len)
    return;
  do {
    i = http1_fio_parser(.parser = &p->parser,
                         .buffer = p->buf + (org_len - p->buf_len),
                         .length = p->buf_len, .on_request = http1_on_request,
                         .on_response = http1_on_response,
                         .on_method = http1_on_method,
                         .on_status = http1_on_status, .on_path = http1_on_path,
                         .on_query = http1_on_query,
                         .on_http_version = http1_on_http_version,
                         .on_header = http1_on_header,
                         .on_body_chunk = http1_on_body_chunk,
                         .on_error = http1_on_error);
    p->buf_len -= i;
    --pipeline_limit;
  } while (i && p->buf_len && pipeline_limit && !p->stop);

  if (p->buf_len && org_len != p->buf_len) {
    memmove(p->buf, p->buf + (org_len - p->buf_len), p->buf_len);
  }

  if (p->buf_len == HTTP_MAX_HEADER_LENGTH) {
    /* no room to read... parser not consuming data */
    if (p->request.method)
      http_send_error(&p->request, 413);
    else {
      p->request.method = fiobj_str_tmp();
      http_send_error(&p->request, 413);
    }
  }

  if (!pipeline_limit) {
    fio_force_event(uuid, FIO_EVENT_ON_DATA);
  }
  return;

throttle:
  /* throttle busy clients (slowloris) */
  p->stop |= 4;
  fio_suspend(uuid);
  FIO_LOG_DEBUG("(HTTP/1,1) throttling client at %.*s",
                (int)fio_peer_addr(uuid).len, fio_peer_addr(uuid).data);
}