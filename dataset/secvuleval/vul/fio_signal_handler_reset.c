void fio_signal_handler_reset(void) {
  struct sigaction old;
  if (!fio_old_sig_int.sa_handler)
    return;
  memset(&old, 0, sizeof(old));
  sigaction(SIGINT, &fio_old_sig_int, &old);
  sigaction(SIGTERM, &fio_old_sig_term, &old);
  sigaction(SIGPIPE, &fio_old_sig_pipe, &old);
  if (fio_old_sig_chld.sa_handler)
    sigaction(SIGCHLD, &fio_old_sig_chld, &old);
#if !FIO_DISABLE_HOT_RESTART
  sigaction(SIGUSR1, &fio_old_sig_usr1, &old);
  memset(&fio_old_sig_usr1, 0, sizeof(fio_old_sig_usr1));
#endif
  memset(&fio_old_sig_int, 0, sizeof(fio_old_sig_int));
  memset(&fio_old_sig_term, 0, sizeof(fio_old_sig_term));
  memset(&fio_old_sig_pipe, 0, sizeof(fio_old_sig_pipe));
  memset(&fio_old_sig_chld, 0, sizeof(fio_old_sig_chld));
}