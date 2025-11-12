static inline __attribute__((unused)) ssize_t fiobj_send_free(intptr_t uuid,
                                                              FIOBJ o) {
  fio_str_info_s s = fiobj_obj2cstr(o);
  return fio_write2(uuid, .data.buffer = (void *)(o),
                    .offset = (((intptr_t)s.data) - ((intptr_t)(o))),
                    .length = s.len, .after.dealloc = fiobj4sock_dealloc);
}