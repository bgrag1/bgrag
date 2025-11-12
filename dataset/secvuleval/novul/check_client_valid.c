static json_t * check_client_valid(struct _oauth2_config * config, const char * client_id, const char * client_header_login, const char * client_header_password, const char * redirect_uri, unsigned short authorization_type, int implicit_flow, const char * ip_source) {
  json_t * j_client, * j_element = NULL, * j_return;
  int uri_found = 0, authorization_type_enabled;
  size_t index = 0;

  if (client_id == NULL) {
    y_log_message(Y_LOG_LEVEL_DEBUG, "check_client_valid - oauth2 - Error client_id is NULL, origin: %s", ip_source);
    return json_pack("{si}", "result", G_ERROR_PARAM);
  } else if (client_header_login != NULL && 0 != o_strcmp(client_header_login, client_id)) {
    y_log_message(Y_LOG_LEVEL_DEBUG, "check_client_valid - oauth2 - Error, client_id specified is different from client_id in the basic auth header, origin: %s", ip_source);
    return json_pack("{si}", "result", G_ERROR_PARAM);
  }
  j_client = config->glewlwyd_config->glewlwyd_callback_check_client_valid(config->glewlwyd_config, client_id, client_header_password);
  if (check_result_value(j_client, G_OK) && json_object_get(json_object_get(j_client, "client"), "enabled") == json_true()) {
    if (!implicit_flow && client_header_password == NULL && json_object_get(json_object_get(j_client, "client"), "confidential") == json_true()) {
      y_log_message(Y_LOG_LEVEL_DEBUG, "check_client_valid - oauth2 - Error, confidential client must be authentified with its password, origin: %s", ip_source);
      j_return = json_pack("{si}", "result", G_ERROR_UNAUTHORIZED);
    } else {
      if (redirect_uri != NULL) {
        json_array_foreach(json_object_get(json_object_get(j_client, "client"), "redirect_uri"), index, j_element) {
          if (0 == o_strcmp(json_string_value(j_element), redirect_uri)) {
            uri_found = 1;
          }
        }
      }

      authorization_type_enabled = 0;
      json_array_foreach(json_object_get(json_object_get(j_client, "client"), "authorization_type"), index, j_element) {
        if (authorization_type == GLEWLWYD_AUTHORIZATION_TYPE_AUTHORIZATION_CODE && 0 == o_strcmp(json_string_value(j_element), "code")) {
          authorization_type_enabled = 1;
        } else if (authorization_type == GLEWLWYD_AUTHORIZATION_TYPE_IMPLICIT && 0 == o_strcmp(json_string_value(j_element), "token")) {
          authorization_type_enabled = 1;
        } else if (authorization_type == GLEWLWYD_AUTHORIZATION_TYPE_RESOURCE_OWNER_PASSWORD_CREDENTIALS && 0 == o_strcmp(json_string_value(j_element), "password")) {
          authorization_type_enabled = 1;
          uri_found = 1; // bypass redirect_uri check for client credentials since it's not needed
        } else if (authorization_type == GLEWLWYD_AUTHORIZATION_TYPE_REFRESH_TOKEN && 0 == o_strcmp(json_string_value(j_element), "refresh_token")) {
          authorization_type_enabled = 1;
          uri_found = 1; // bypass redirect_uri check for client credentials since it's not needed
        } else if (authorization_type == GLEWLWYD_AUTHORIZATION_TYPE_DELETE_TOKEN && 0 == o_strcmp(json_string_value(j_element), "delete_token")) {
          authorization_type_enabled = 1;
          uri_found = 1; // bypass redirect_uri check for client credentials since it's not needed
        } else if (authorization_type == GLEWLWYD_AUTHORIZATION_TYPE_DEVICE_AUTHORIZATION && 0 == o_strcmp(json_string_value(j_element), "device_authorization")) {
          authorization_type_enabled = 1;
          uri_found = 1; // bypass redirect_uri check for client credentials since it's not needed
        }
      }
      if (!uri_found) {
        y_log_message(Y_LOG_LEVEL_DEBUG, "check_client_valid - oauth2 - Error, redirect_uri '%s' is invalid for the client '%s', origin: %s", redirect_uri, client_id, ip_source);
      }
      if (!authorization_type_enabled) {
        y_log_message(Y_LOG_LEVEL_DEBUG, "check_client_valid - oauth2 - Error, authorization type is not enabled for the client '%s', origin: %s", client_id, ip_source);
      }
      if (uri_found && authorization_type_enabled) {
        j_return = json_pack("{sisO}", "result", G_OK, "client", json_object_get(j_client, "client"));
      } else {
        j_return = json_pack("{si}", "result", G_ERROR_PARAM);
      }
    }
  } else {
    y_log_message(Y_LOG_LEVEL_DEBUG, "check_client_valid - oauth2 - Error, client '%s' is invalid, origin: %s", client_id, ip_source);
    j_return = json_pack("{si}", "result", G_ERROR_UNAUTHORIZED);
  }
  json_decref(j_client);
  return j_return;
}