static int http1_on_request(http1_parser_s *parser) {
  http1pr_s *p = parser2http(parser);
  http_on_request_handler______internal(&http1_pr2handle(p), p->p.settings);
  if (p->request.method && !p->stop)
    http_finish(&p->request);
  h1_reset(p);
  return !p->close && fio_is_closed(p->p.uuid);
}