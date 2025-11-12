static void http1_on_data_first_time(intptr_t uuid, fio_protocol_s *protocol) {
  http1pr_s *p = (http1pr_s *)protocol;
  ssize_t i;

  i = fio_read(uuid, p->buf + p->buf_len, HTTP_MAX_HEADER_LENGTH - p->buf_len);

  if (i <= 0)
    return;
  p->buf_len += i;

  /* ensure future reads skip this first time HTTP/2.0 test */
  p->p.protocol.on_data = http1_on_data;
  /* Test fot HTTP/2.0 pre-knowledge */
  if (i >= 24 && !memcmp(p->buf, "PRI * HTTP/2.0\r\n\r\nSM\r\n\r\n", 24)) {
    FIO_LOG_WARNING("client claimed unsupported HTTP/2 prior knowledge.");
    fio_close(uuid);
    return;
  }

  /* Finish handling the same way as the normal `on_data` */
  http1_consume_data(uuid, p);
}