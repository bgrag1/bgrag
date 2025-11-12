static void sig_int_handler(int sig) {
  struct sigaction *old = NULL;
  switch (sig) {
#if !FIO_DISABLE_HOT_RESTART
  case SIGUSR1:
    fio_signal_children_flag = 1;
    old = &fio_old_sig_usr1;
    break;
#endif
    /* fallthrough */
  case SIGINT:
    if (!old)
      old = &fio_old_sig_int;
    /* fallthrough */
  case SIGTERM:
    if (!old)
      old = &fio_old_sig_term;
    fio_stop();
    break;
  case SIGPIPE:
    if (!old)
      old = &fio_old_sig_pipe;
  /* fallthrough */
  default:
    break;
  }
  /* propagate signale handling to previous existing handler (if any) */
  if (old && old->sa_handler != SIG_IGN && old->sa_handler != SIG_DFL)
    old->sa_handler(sig);
}