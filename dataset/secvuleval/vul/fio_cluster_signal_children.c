static void fio_cluster_signal_children(void) {
  if (fio_parent_pid() != getpid()) {
    fio_stop();
    return;
  }
  fio_cluster_server_sender(fio_msg_internal_create(0, FIO_CLUSTER_MSG_SHUTDOWN,
                                                    (fio_str_info_s){.len = 0},
                                                    (fio_str_info_s){.len = 0},
                                                    0, 1),
                            -1);
}