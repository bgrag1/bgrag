static void fio_worker_cleanup(void) {
  /* switch to winding down */
  if (fio_data->is_worker)
    FIO_LOG_INFO("(%d) detected exit signal.", (int)getpid());
  else
    FIO_LOG_INFO("Server Detected exit signal.");
  fio_state_callback_force(FIO_CALL_ON_SHUTDOWN);
  for (size_t i = 0; i <= fio_data->max_protocol_fd; ++i) {
    if (fd_data(i).protocol) {
      fio_defer_push_task(deferred_on_shutdown, (void *)fd2uuid(i), NULL);
    }
  }
  fio_defer_push_task(fio_cycle_unwind, NULL, NULL);
  fio_defer_perform();
  for (size_t i = 0; i <= fio_data->max_protocol_fd; ++i) {
    if (fd_data(i).protocol || fd_data(i).open) {
      fio_force_close(fd2uuid(i));
    }
  }
  fio_timer_clear_all();
  fio_defer_perform();
  if (!fio_data->is_worker) {
    kill(0, SIGINT);
    while (wait(NULL) != -1)
      ;
  }
  fio_defer_perform();
  fio_state_callback_force(FIO_CALL_ON_FINISH);
  fio_defer_perform();
  fio_signal_handler_reset();
  if (fio_data->parent == getpid()) {
    FIO_LOG_INFO("   ---  Shutdown Complete  ---\n");
  } else {
    FIO_LOG_INFO("(%d) cleanup complete.", (int)getpid());
  }
}