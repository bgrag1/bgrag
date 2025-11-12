static json_t * validate_authorization_code(struct _oauth2_config * config, const char * code, const char * client_id, const char * redirect_uri, const char * code_verifier, const char * ip_source) {
  char * code_hash = config->glewlwyd_config->glewlwyd_callback_generate_hash(config->glewlwyd_config, code), * expiration_clause = NULL, * scope_list = NULL, * tmp;
  json_t * j_query, * j_result = NULL, * j_result_scope = NULL, * j_return, * j_element = NULL, * j_scope_param;
  int res;
  size_t index = 0;
  json_int_t maximum_duration = config->refresh_token_duration, maximum_duration_override = -1;
  int rolling_refresh = config->refresh_token_rolling, rolling_refresh_override = -1;

  if (code_hash != NULL) {
    if (config->glewlwyd_config->glewlwyd_config->conn->type==HOEL_DB_TYPE_MARIADB) {
      expiration_clause = o_strdup("> NOW()");
    } else if (config->glewlwyd_config->glewlwyd_config->conn->type==HOEL_DB_TYPE_PGSQL) {
      expiration_clause = o_strdup("> NOW()");
    } else { // HOEL_DB_TYPE_SQLITE
      expiration_clause = o_strdup("> (strftime('%s','now'))");
    }
    j_query = json_pack("{sss[ssss]s{sssssssss{ssss}}}",
                        "table",
                        GLEWLWYD_PLUGIN_OAUTH2_TABLE_CODE,
                        "columns",
                          "gpgc_username AS username",
                          "gpgc_id",
                          "gpgc_code_challenge AS code_challenge",
                          "gpgc_enabled AS enabled",
                        "where",
                          "gpgc_plugin_name",
                          config->name,
                          "gpgc_client_id",
                          client_id,
                          "gpgc_redirect_uri",
                          redirect_uri,
                          "gpgc_code_hash",
                          code_hash,
                          "gpgc_expires_at",
                            "operator",
                            "raw",
                            "value",
                            expiration_clause);
    o_free(expiration_clause);
    res = h_select(config->glewlwyd_config->glewlwyd_config->conn, j_query, &j_result, NULL);
    json_decref(j_query);
    if (res == H_OK) {
      if (json_array_size(j_result)) {
        if (json_integer_value(json_object_get(json_array_get(j_result, 0), "enabled"))) {
          if ((res = validate_code_challenge(config, json_array_get(j_result, 0), code_verifier)) == G_OK) {
            j_query = json_pack("{sss[s]s{sO}}",
                                "table",
                                GLEWLWYD_PLUGIN_OAUTH2_TABLE_CODE_SCOPE,
                                "columns",
                                  "gpgcs_scope AS name",
                                "where",
                                  "gpgc_id",
                                  json_object_get(json_array_get(j_result, 0), "gpgc_id"));
            res = h_select(config->glewlwyd_config->glewlwyd_config->conn, j_query, &j_result_scope, NULL);
            json_decref(j_query);
            if (res == H_OK && json_array_size(j_result_scope) > 0) {
              if (!json_object_set_new(json_array_get(j_result, 0), "scope", json_array())) {
                json_array_foreach(j_result_scope, index, j_element) {
                  if (scope_list == NULL) {
                    scope_list = o_strdup(json_string_value(json_object_get(j_element, "name")));
                  } else {
                    tmp = msprintf("%s %s", scope_list, json_string_value(json_object_get(j_element, "name")));
                    o_free(scope_list);
                    scope_list = tmp;
                  }
                  if ((j_scope_param = get_scope_parameters(config, json_string_value(json_object_get(j_element, "name")))) != NULL) {
                    json_object_update(j_element, j_scope_param);
                    json_decref(j_scope_param);
                  }
                  if (json_object_get(j_element, "refresh-token-rolling") != NULL && rolling_refresh_override != 0) {
                    rolling_refresh_override = json_object_get(j_element, "refresh-token-rolling")==json_true();
                  }
                  if (json_integer_value(json_object_get(j_element, "refresh-token-duration")) && (json_integer_value(json_object_get(j_element, "refresh-token-duration")) < maximum_duration_override || maximum_duration_override == -1)) {
                    maximum_duration_override = json_integer_value(json_object_get(j_element, "refresh-token-duration"));
                  }
                  json_array_append(json_object_get(json_array_get(j_result, 0), "scope"), j_element);
                }
                if (rolling_refresh_override > -1) {
                  rolling_refresh = rolling_refresh_override;
                }
                if (maximum_duration_override > -1) {
                  maximum_duration = maximum_duration_override;
                }
                json_object_set_new(json_array_get(j_result, 0), "scope_list", json_string(scope_list));
                json_object_set_new(json_array_get(j_result, 0), "refresh-token-rolling", rolling_refresh?json_true():json_false());
                json_object_set_new(json_array_get(j_result, 0), "refresh-token-duration", json_integer(maximum_duration));
                j_return = json_pack("{sisO}", "result", G_OK, "code", json_array_get(j_result, 0));
                o_free(scope_list);
              } else {
                y_log_message(Y_LOG_LEVEL_ERROR, "validate_authorization_code - oauth2 - Error allocating resources for json_array()");
                j_return = json_pack("{si}", "result", G_ERROR_MEMORY);
              }
            } else {
              y_log_message(Y_LOG_LEVEL_ERROR, "validate_authorization_code - oauth2 - Error executing j_query (2)");
              config->glewlwyd_config->glewlwyd_plugin_callback_metrics_increment_counter(config->glewlwyd_config, GLWD_METRICS_DATABSE_ERROR, 1, NULL);
              j_return = json_pack("{si}", "result", G_ERROR_DB);
            }
            json_decref(j_result_scope);
          } else if (res == G_ERROR_UNAUTHORIZED) {
            j_return = json_pack("{si}", "result", G_ERROR_UNAUTHORIZED);
          } else if (res == G_ERROR_PARAM) {
            j_return = json_pack("{si}", "result", G_ERROR_PARAM);
          } else {
            y_log_message(Y_LOG_LEVEL_ERROR, "validate_authorization_code - oauth2 - Error validate_code_challenge");
            j_return = json_pack("{si}", "result", G_ERROR);
          }
        } else {
          if (json_true() == json_object_get(config->j_params, "auth-type-code-revoke-replayed")) {
            if (revoke_tokens_from_code(config, json_integer_value(json_object_get(json_array_get(j_result, 0), "gpgc_id")), ip_source) != G_OK) {
              y_log_message(Y_LOG_LEVEL_ERROR, "validate_authorization_code - oauth2 - Error revoke_tokens_from_code");
            }
          }
          j_return = json_pack("{si}", "result", G_ERROR_UNAUTHORIZED);
        }
      } else {
        j_return = json_pack("{si}", "result", G_ERROR_UNAUTHORIZED);
      }
    } else {
      y_log_message(Y_LOG_LEVEL_ERROR, "validate_authorization_code - oauth2 - Error executing j_query (1)");
      config->glewlwyd_config->glewlwyd_plugin_callback_metrics_increment_counter(config->glewlwyd_config, GLWD_METRICS_DATABSE_ERROR, 1, NULL);
      j_return = json_pack("{si}", "result", G_ERROR_DB);
    }
    json_decref(j_result);
  } else {
    y_log_message(Y_LOG_LEVEL_ERROR, "validate_authorization_code - oauth2 - Error glewlwyd_callback_generate_hash");
    j_return = json_pack("{si}", "result", G_ERROR);
  }
  o_free(code_hash);
  return j_return;
}