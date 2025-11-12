static void fio_cluster_listen_on_close(intptr_t uuid,
                                        fio_protocol_s *protocol) {
  free(protocol);
  cluster_data.uuid = -1;
  if (fio_parent_pid() == getpid()) {
#if DEBUG
    FIO_LOG_DEBUG("(%d) stopped listening for cluster connections",
                  (int)getpid());
#endif
    if (fio_data->active)
      kill(0, SIGINT);
  }
  (void)uuid;
}