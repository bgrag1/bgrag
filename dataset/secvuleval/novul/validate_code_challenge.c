static int validate_code_challenge(struct _oauth2_config * config, json_t * j_result_code, const char * code_verifier) {
  int ret;
  unsigned char code_verifier_hash[32] = {0}, code_verifier_hash_b64[64] = {0};
  size_t code_verifier_hash_len = 32, code_verifier_hash_b64_len = 0;
  gnutls_datum_t key_data;

  if (json_object_get(config->j_params, "pkce-allowed") != json_true()) {
    if (o_strnullempty(code_verifier)) {
      ret = G_OK;
    } else {
      y_log_message(Y_LOG_LEVEL_DEBUG, "oidc validate_code_challenge - code_verifier unauthorized");
      ret = G_ERROR_UNAUTHORIZED;
    }
  } else if ((!o_strnullempty(code_verifier) && json_string_null_or_empty(json_object_get(j_result_code, "code_challenge"))) || (o_strnullempty(code_verifier) && !json_string_null_or_empty(json_object_get(j_result_code, "code_challenge")))) {
    y_log_message(Y_LOG_LEVEL_DEBUG, "oidc validate_code_challenge - Invalid code_challenge or code_verifier");
    ret = G_ERROR_UNAUTHORIZED;
  } else {
    if (is_pkce_char_valid(code_verifier)) {
      if (0 == o_strncmp(GLEWLWYD_CODE_CHALLENGE_S256_PREFIX, json_string_value(json_object_get(j_result_code, "code_challenge")), o_strlen(GLEWLWYD_CODE_CHALLENGE_S256_PREFIX))) {
        key_data.data = (unsigned char *)code_verifier;
        key_data.size = (unsigned int)o_strlen(code_verifier);
        if (gnutls_fingerprint(GNUTLS_DIG_SHA256, &key_data, code_verifier_hash, &code_verifier_hash_len) == GNUTLS_E_SUCCESS) {
          if (o_base64url_encode(code_verifier_hash, code_verifier_hash_len, code_verifier_hash_b64, &code_verifier_hash_b64_len)) {
            code_verifier_hash_b64[code_verifier_hash_b64_len] = '\0';
            if (0 == o_strcmp(json_string_value(json_object_get(j_result_code, "code_challenge"))+o_strlen(GLEWLWYD_CODE_CHALLENGE_S256_PREFIX), (const char *)code_verifier_hash_b64)) {
              ret = G_OK;
            } else {
              y_log_message(Y_LOG_LEVEL_DEBUG, "oidc validate_code_challenge - Invalid code_challenge value");
              ret = G_ERROR_UNAUTHORIZED;
            }
          } else {
            y_log_message(Y_LOG_LEVEL_ERROR, "oidc validate_code_challenge - Error o_base64url_encode");
            ret = G_ERROR;
          }
        } else {
          y_log_message(Y_LOG_LEVEL_ERROR, "oidc validate_code_challenge - Error gnutls_fingerprint");
          ret = G_ERROR;
        }
      } else {
        if (0 == o_strcmp(json_string_value(json_object_get(j_result_code, "code_challenge")), code_verifier)) {
          ret = G_OK;
        } else {
          y_log_message(Y_LOG_LEVEL_DEBUG, "oidc validate_code_challenge - Invalid code_challenge value");
          ret = G_ERROR_UNAUTHORIZED;
        }
      }
    } else {
      y_log_message(Y_LOG_LEVEL_DEBUG, "oidc validate_code_challenge - Invalid code_challenge character set");
      ret = G_ERROR_UNAUTHORIZED;
    }
  }
  return ret;
}