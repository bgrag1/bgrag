FIO_FUNC inline void fio_throttle_thread(size_t nano_sec) {
  const struct timespec tm = {.tv_nsec = (long)(nano_sec % 1000000000),
                              .tv_sec = (time_t)(nano_sec / 1000000000)};
  nanosleep(&tm, NULL);
}