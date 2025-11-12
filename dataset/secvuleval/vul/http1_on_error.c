static int http1_on_error(http1_parser_s *parser) {
  FIO_LOG_DEBUG("HTTP parser error at HTTP/1.1 buffer position %zu/%zu",
                parser->state.next - parser2http(parser)->buf,
                parser2http(parser)->buf_len);
  fio_close(parser2http(parser)->p.uuid);
  return -1;
}