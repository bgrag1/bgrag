static int http1_on_error(http1_parser_s *parser) {
  if (parser2http(parser)->close)
    return -1;
  FIO_LOG_DEBUG("HTTP parser error.");
  fio_close(parser2http(parser)->p.uuid);
  return -1;
}