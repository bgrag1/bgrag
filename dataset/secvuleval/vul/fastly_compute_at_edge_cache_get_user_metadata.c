bool fastly_compute_at_edge_cache_get_user_metadata(fastly_compute_at_edge_cache_handle_t handle,
                                                    fastly_world_list_u8_t *ret,
                                                    fastly_compute_at_edge_cache_error_t *err) {
  size_t default_size = 16 * 1024;
  ret->ptr = static_cast<uint8_t *>(cabi_malloc(default_size, 4));
  auto status = fastly::cache_get_user_metadata(handle, reinterpret_cast<char *>(ret->ptr),
                                                default_size, &ret->len);
  if (status == FASTLY_COMPUTE_AT_EDGE_TYPES_ERROR_BUFFER_LEN) {
    cabi_realloc(ret->ptr, default_size, 4, ret->len);
    status = fastly::cache_get_user_metadata(handle, reinterpret_cast<char *>(ret->ptr), ret->len,
                                             &ret->len);
  }
  return convert_result(status, err);
}