inline FIO_FUNC ssize_t fio_sendfile(intptr_t uuid, intptr_t source_fd,
                                     off_t offset, size_t length) {
  return fio_write2(uuid, .data.fd = source_fd, .length = length, .is_fd = 1,
                    .offset = offset);
}