static int http1_on_response(http1_parser_s *parser) {
  http1pr_s *p = parser2http(parser);
  http_on_response_handler______internal(&http1_pr2handle(p), p->p.settings);
  if (p->request.status_str && !p->stop)
    http_finish(&p->request);
  h1_reset(p);
  return fio_is_closed(p->p.uuid);
}