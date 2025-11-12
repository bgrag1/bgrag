bool fastly_compute_at_edge_http_req_downstream_tls_cipher_openssl_name(
    fastly_world_string_t *ret, fastly_compute_at_edge_types_error_t *err) {
  auto default_size = 128;
  ret->ptr = static_cast<uint8_t *>(cabi_malloc(default_size, 4));
  auto status = fastly::req_downstream_tls_cipher_openssl_name(reinterpret_cast<char *>(ret->ptr),
                                                               default_size, &ret->len);
  if (status == FASTLY_COMPUTE_AT_EDGE_TYPES_ERROR_BUFFER_LEN) {
    ret->ptr = static_cast<uint8_t *>(cabi_realloc(ret->ptr, default_size, 4, ret->len));
    status = fastly::req_downstream_tls_cipher_openssl_name(reinterpret_cast<char *>(ret->ptr),
                                                            ret->len, &ret->len);
  }
  return convert_result(status, err);
}