static int r_jws_verify_sig_hmac(jws_t * jws, jwk_t * jwk) {
  unsigned char * sig = r_jws_sign_hmac(jws, jwk);
  int ret;

  if (!o_strnullempty((const char *)jws->signature_b64url) &&
      !o_strnullempty((const char *)sig) &&
      o_strlen((const char *)sig) == o_strlen((const char *)jws->signature_b64url) &&
      0 == gnutls_memcmp(jws->signature_b64url, sig, o_strlen((const char *)sig))) {
    ret = RHN_OK;
  } else {
    ret = RHN_ERROR_INVALID;
  }
  o_free(sig);
  return ret;
}