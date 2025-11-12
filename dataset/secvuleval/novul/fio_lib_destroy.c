static void __attribute__((destructor)) fio_lib_destroy(void) {
  uint8_t add_eol = fio_is_master();
  fio_data->active = 0;
  fio_on_fork();
  fio_defer_perform();
  fio_timer_clear_all();
  fio_defer_perform();
  fio_state_callback_force(FIO_CALL_AT_EXIT);
  fio_state_callback_clear_all();
  fio_defer_perform();
  fio_poll_close();
  fio_free(fio_data);
  /* memory library destruction must be last */
  fio_mem_destroy();
  FIO_LOG_DEBUG("(%d) facil.io resources released, exit complete.",
                (int)getpid());
  if (add_eol)
    fprintf(stderr, "\n"); /* add EOL to logs (logging adds EOL before text */
}