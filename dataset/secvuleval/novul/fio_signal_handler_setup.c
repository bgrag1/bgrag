static void fio_signal_handler_setup(void) {
  /* setup signal handling */
  struct sigaction act;
  if (fio_trylock(&fio_signal_set_flag))
    return;

  memset(&act, 0, sizeof(act));

  act.sa_handler = sig_int_handler;
  sigemptyset(&act.sa_mask);
  act.sa_flags = SA_RESTART | SA_NOCLDSTOP;

  if (sigaction(SIGINT, &act, &fio_old_sig_int)) {
    perror("couldn't set signal handler");
    return;
  };

  if (sigaction(SIGTERM, &act, &fio_old_sig_term)) {
    perror("couldn't set signal handler");
    return;
  };
#if !FIO_DISABLE_HOT_RESTART
  if (sigaction(SIGUSR1, &act, &fio_old_sig_usr1)) {
    perror("couldn't set signal handler");
    return;
  };
#endif

  act.sa_handler = SIG_IGN;
  if (sigaction(SIGPIPE, &act, &fio_old_sig_pipe)) {
    perror("couldn't set signal handler");
    return;
  };
}