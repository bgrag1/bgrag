static void http1_on_ready(intptr_t uuid, fio_protocol_s *protocol) {
  /* resume slow clients from suspension */
  http1pr_s *p = (http1pr_s *)protocol;
  if (p->stop & 4) {
    p->stop ^= 4; /* flip back the bit, so it's zero */
    fio_force_event(uuid, FIO_EVENT_ON_DATA);
  }
  (void)protocol;
}