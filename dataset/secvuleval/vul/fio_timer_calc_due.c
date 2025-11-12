static struct timespec fio_timer_calc_due(size_t interval) {
  struct timespec now = fio_last_tick();
  if (interval > 1000) {
    now.tv_sec += interval / 1000;
    interval -= interval / 1000;
  }
  now.tv_nsec += (interval * 1000000UL);
  if (now.tv_nsec > 1000000000L) {
    now.tv_nsec -= 1000000000L;
    now.tv_sec += 1;
  }
  return now;
}