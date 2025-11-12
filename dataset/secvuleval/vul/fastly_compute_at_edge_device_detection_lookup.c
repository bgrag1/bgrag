bool fastly_compute_at_edge_device_detection_lookup(
    fastly_world_string_t *user_agent, fastly_world_string_t *ret,
    fastly_compute_at_edge_device_detection_error_t *err) {
  auto default_size = 1024;
  ret->ptr = static_cast<uint8_t *>(cabi_malloc(default_size, 4));
  auto status =
      fastly::device_detection_lookup(reinterpret_cast<char *>(user_agent->ptr), user_agent->len,
                                      reinterpret_cast<char *>(ret->ptr), default_size, &ret->len);
  if (status == FASTLY_COMPUTE_AT_EDGE_TYPES_ERROR_BUFFER_LEN) {
    cabi_realloc(ret->ptr, default_size, 4, ret->len);
    status =
        fastly::device_detection_lookup(reinterpret_cast<char *>(user_agent->ptr), user_agent->len,
                                        reinterpret_cast<char *>(ret->ptr), ret->len, &ret->len);
  }
  return convert_result(status, err);
}