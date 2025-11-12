void FIO_TLS_WEAK fio_tls_destroy(fio_tls_s *tls) {
  if (!tls)
    return;
  REQUIRE_LIBRARY();
  if (fio_atomic_sub(&tls->ref, 1))
    return;
  fio_tls_destroy_context(tls);
  alpn_list_free(&tls->alpn);
  cert_ary_free(&tls->sni);
  trust_ary_free(&tls->trust);
  free(tls);
}