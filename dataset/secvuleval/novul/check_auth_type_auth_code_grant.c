static int check_auth_type_auth_code_grant (const struct _u_request * request, struct _u_response * response, void * user_data) {
  struct _oauth2_config * config = (struct _oauth2_config *)user_data;
  char * authorization_code = NULL, * redirect_url, * issued_for, * state_param = NULL, * state_encoded, code_challenge_stored[GLEWLWYD_CODE_CHALLENGE_MAX_LENGTH + 1] = {0};
  const char * ip_source = get_ip_source(request, config->glewlwyd_config->glewlwyd_config->originating_ip_header);
  json_t * j_session, * j_client = check_client_valid(config, u_map_get(request->map_url, "client_id"), request->auth_basic_user, request->auth_basic_password, u_map_get(request->map_url, "redirect_uri"), GLEWLWYD_AUTHORIZATION_TYPE_AUTHORIZATION_CODE, 1, ip_source);
  int res;

  if (u_map_get(request->map_url, "state") != NULL) {
    state_encoded = ulfius_url_encode(u_map_get(request->map_url, "state"));
    state_param = msprintf("&state=%s", state_encoded);
    o_free(state_encoded);
  } else {
    state_param = o_strdup("");
  }
  // Check if client is allowed to perform this request
  if (check_result_value(j_client, G_OK)) {
    // Client is allowed to use auth_code grant with this redirection_uri
    if (!o_strnullempty(u_map_get(request->map_url, "scope"))) {
      if (u_map_has_key(request->map_url, "g_continue")) {
        j_session = validate_session_client_scope(config, request, u_map_get(request->map_url, "client_id"), u_map_get(request->map_url, "scope"));
        if (check_result_value(j_session, G_OK)) {
          if (json_object_get(json_object_get(j_session, "session"), "authorization_required") == json_false()) {
            // User has granted access to the cleaned scope list for this client
            // Generate code, generate the url and redirect to it
            issued_for = get_client_hostname(request, config->glewlwyd_config->glewlwyd_config->originating_ip_header);
            if (issued_for != NULL) {
              if (config->glewlwyd_config->glewlwyd_callback_trigger_session_used(config->glewlwyd_config, request, json_string_value(json_object_get(json_object_get(j_session, "session"), "scope_filtered"))) == G_OK) {
                if ((res = is_code_challenge_valid(config, u_map_get(request->map_url, "code_challenge"), u_map_get(request->map_url, "code_challenge_method"), code_challenge_stored)) == G_OK) {
                  if ((authorization_code = generate_authorization_code(config, json_string_value(json_object_get(json_object_get(json_object_get(j_session, "session"), "user"), "username")), u_map_get(request->map_url, "client_id"), json_string_value(json_object_get(json_object_get(j_session, "session"), "scope_filtered")), u_map_get(request->map_url, "redirect_uri"), issued_for, u_map_get_case(request->map_header, "user-agent"), code_challenge_stored)) != NULL) {
                    redirect_url = msprintf("%s%scode=%s%s", u_map_get(request->map_url, "redirect_uri"), (o_strchr(u_map_get(request->map_url, "redirect_uri"), '?')!=NULL?"&":"?"), authorization_code, state_param);
                    ulfius_add_header_to_response(response, "Location", redirect_url);
                    response->status = 302;
                    o_free(redirect_url);
                    config->glewlwyd_config->glewlwyd_plugin_callback_metrics_increment_counter(config->glewlwyd_config, GLWD_METRICS_OAUTH2_CODE, 1, "plugin", config->name, NULL);
                  } else {
                    redirect_url = msprintf("%s%serror=server_error", u_map_get(request->map_url, "redirect_uri"), (o_strchr(u_map_get(request->map_url, "redirect_uri"), '?')!=NULL?"&":"?"));
                    ulfius_add_header_to_response(response, "Location", redirect_url);
                    o_free(redirect_url);
                    y_log_message(Y_LOG_LEVEL_ERROR, "check_auth_type_auth_code_grant - oauth2 - Error generate_authorization_code");
                    response->status = 302;
                  }
                  o_free(authorization_code);
                } else if (res == G_ERROR_PARAM) {
                  redirect_url = msprintf("%s%serror=invalid_request", u_map_get(request->map_url, "redirect_uri"), (o_strchr(u_map_get(request->map_url, "redirect_uri"), '?')!=NULL?"&":"?"));
                  ulfius_add_header_to_response(response, "Location", redirect_url);
                  o_free(redirect_url);
                  y_log_message(Y_LOG_LEVEL_DEBUG, "check_auth_type_auth_code_grant - oauth2 - Invalid code_challenge or code_challenge_method, origin: %s", get_ip_source(request, config->glewlwyd_config->glewlwyd_config->originating_ip_header));
                  response->status = 302;
                } else {
                  redirect_url = msprintf("%s%serror=server_error", u_map_get(request->map_url, "redirect_uri"), (o_strchr(u_map_get(request->map_url, "redirect_uri"), '?')!=NULL?"&":"?"));
                  ulfius_add_header_to_response(response, "Location", redirect_url);
                  o_free(redirect_url);
                  y_log_message(Y_LOG_LEVEL_ERROR, "check_auth_type_auth_code_grant - oauth2 - Error is_code_challenge_valid");
                  response->status = 302;
                }
              } else {
                redirect_url = msprintf("%s%serror=server_error", u_map_get(request->map_url, "redirect_uri"), (o_strchr(u_map_get(request->map_url, "redirect_uri"), '?')!=NULL?"&":"?"));
                ulfius_add_header_to_response(response, "Location", redirect_url);
                o_free(redirect_url);
                y_log_message(Y_LOG_LEVEL_ERROR, "check_auth_type_auth_code_grant - oauth2 - Error glewlwyd_callback_trigger_session_used");
                response->status = 302;
              }
            } else {
              redirect_url = msprintf("%s%serror=server_error", u_map_get(request->map_url, "redirect_uri"), (o_strchr(u_map_get(request->map_url, "redirect_uri"), '?')!=NULL?"&":"?"));
              ulfius_add_header_to_response(response, "Location", redirect_url);
              o_free(redirect_url);
              y_log_message(Y_LOG_LEVEL_ERROR, "check_auth_type_auth_code_grant - oauth2 - Error get_client_hostname");
              response->status = 302;
            }
            o_free(issued_for);
          } else {
            // Redirect to login page
            redirect_url = get_login_url(config, request, "auth", u_map_get(request->map_url, "client_id"), u_map_get(request->map_url, "scope"), NULL);
            ulfius_add_header_to_response(response, "Location", redirect_url);
            o_free(redirect_url);
            response->status = 302;
          }
        } else if (check_result_value(j_session, G_ERROR_NOT_FOUND)) {
          // Redirect to login page
          redirect_url = get_login_url(config, request, "auth", u_map_get(request->map_url, "client_id"), u_map_get(request->map_url, "scope"), NULL);
          ulfius_add_header_to_response(response, "Location", redirect_url);
          o_free(redirect_url);
          response->status = 302;
        } else if (check_result_value(j_session, G_ERROR_UNAUTHORIZED)) {
          // Scope is not allowed for this user
          response->status = 302;
          y_log_message(Y_LOG_LEVEL_DEBUG, "check_auth_type_auth_code_grant - oauth2 - scope list '%s' is invalid for user '%s', origin: %s", u_map_get(request->map_url, "scope"), json_string_value(json_object_get(json_object_get(json_object_get(j_session, "session"), "user"), "username")), ip_source);
          redirect_url = msprintf("%s%serror=invalid_scope%s", u_map_get(request->map_url, "redirect_uri"), (o_strchr(u_map_get(request->map_url, "redirect_uri"), '?')!=NULL?"&":"?"), state_param);
          ulfius_add_header_to_response(response, "Location", redirect_url);
          o_free(redirect_url);
        } else {
          redirect_url = msprintf("%s%serror=server_error", u_map_get(request->map_url, "redirect_uri"), (o_strchr(u_map_get(request->map_url, "redirect_uri"), '?')!=NULL?"&":"?"));
          ulfius_add_header_to_response(response, "Location", redirect_url);
          o_free(redirect_url);
          y_log_message(Y_LOG_LEVEL_ERROR, "check_auth_type_auth_code_grant - oauth2 - Error validate_session_client_scope");
          response->status = 302;
        }
        json_decref(j_session);
      } else {
        // Redirect to login page
        redirect_url = get_login_url(config, request, "auth", u_map_get(request->map_url, "client_id"), u_map_get(request->map_url, "scope"), NULL);
        ulfius_add_header_to_response(response, "Location", redirect_url);
        o_free(redirect_url);
        response->status = 302;
      }
    } else {
      // Scope is not allowed for this user
      y_log_message(Y_LOG_LEVEL_DEBUG, "check_auth_type_auth_code_grant - oauth2 - scope list is missing or empty, origin: %s", ip_source);
      response->status = 403;
    }
  } else {
    // client is not authorized
    response->status = 403;
    config->glewlwyd_config->glewlwyd_plugin_callback_metrics_increment_counter(config->glewlwyd_config, GLWD_METRICS_OAUTH2_UNAUTHORIZED_CLIENT, 1, "plugin", config->name, NULL);
  }
  o_free(state_param);
  json_decref(j_client);
  return U_CALLBACK_CONTINUE;
}