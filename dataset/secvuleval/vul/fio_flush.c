ssize_t fio_flush(intptr_t uuid) {
  if (!uuid_is_valid(uuid))
    goto invalid;
  errno = 0;
  ssize_t flushed = 0;
  int tmp;
  /* start critical section */
  if (fio_trylock(&uuid_data(uuid).sock_lock))
    goto would_block;

  if (!uuid_data(uuid).packet)
    goto flush_rw_hook;

  const fio_packet_s *old_packet = uuid_data(uuid).packet;
  const size_t old_sent = uuid_data(uuid).sent;

  tmp = uuid_data(uuid).packet->write_func(fio_uuid2fd(uuid),
                                           uuid_data(uuid).packet);
  if (tmp <= 0) {
    goto test_errno;
  }

  if (uuid_data(uuid).packet_count >= 1024 &&
      uuid_data(uuid).packet == old_packet &&
      uuid_data(uuid).sent >= old_sent &&
      (uuid_data(uuid).sent - old_sent) < 32768) {
    /* Slowloris attack assumed */
    goto attacked;
  }

  /* end critical section */
  fio_unlock(&uuid_data(uuid).sock_lock);

  /* test for fio_close marker */
  if (!uuid_data(uuid).packet && uuid_data(uuid).close)
    goto closed;

  /* return state */
  return uuid_data(uuid).open && uuid_data(uuid).packet != NULL;

would_block:
  errno = EWOULDBLOCK;
  return -1;

closed:
  fio_force_close(uuid);
  return -1;

flush_rw_hook:
  flushed = uuid_data(uuid).rw_hooks->flush(uuid, uuid_data(uuid).rw_udata);
  fio_unlock(&uuid_data(uuid).sock_lock);
  if (!flushed)
    return 0;
  if (flushed < 0) {
    goto test_errno;
  }
  touchfd(fio_uuid2fd(uuid));
  return 1;

test_errno:
  fio_unlock(&uuid_data(uuid).sock_lock);
  switch (errno) {
  case EWOULDBLOCK: /* fallthrough */
#if EWOULDBLOCK != EAGAIN
  case EAGAIN: /* fallthrough */
#endif
  case ENOTCONN:      /* fallthrough */
  case EINPROGRESS:   /* fallthrough */
  case ENOSPC:        /* fallthrough */
  case EADDRNOTAVAIL: /* fallthrough */
  case EINTR:
    return 1;
  case EFAULT:
    FIO_LOG_ERROR("fio_flush EFAULT - possible memory address error sent to "
                  "Unix socket.");
    /* fallthrough */
  case EPIPE:  /* fallthrough */
  case EIO:    /* fallthrough */
  case EINVAL: /* fallthrough */
  case EBADF:
    uuid_data(uuid).close = 1;
    fio_force_close(uuid);
    return -1;
  }
  fprintf(stderr, "UUID error: %p (%d)\n", (void *)uuid, errno);
  perror("No errno handler");
  return 0;

invalid:
  /* bad UUID */
  errno = EBADF;
  return -1;

attacked:
  /* don't close, just detach from facil.io and mark uuid as invalid */
  FIO_LOG_WARNING("(facil.io) possible Slowloris attack from %.*s",
                  (int)fio_peer_addr(uuid).len, fio_peer_addr(uuid).data);
  fio_unlock(&uuid_data(uuid).sock_lock);
  fio_clear_fd(fio_uuid2fd(uuid), 0);
  return -1;
}