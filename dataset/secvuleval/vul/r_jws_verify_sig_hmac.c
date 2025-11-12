static int r_jws_verify_sig_hmac(jws_t * jws, jwk_t * jwk) {
  unsigned char * sig = r_jws_sign_hmac(jws, jwk);
  int ret;

  if (sig != NULL && 0 == o_strcmp((const char *)jws->signature_b64url, (const char *)sig)) {
    ret = RHN_OK;
  } else {
    ret = RHN_ERROR_INVALID;
  }
  o_free(sig);
  return ret;
}