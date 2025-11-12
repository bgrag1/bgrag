static int callback_oauth2_authorization(const struct _u_request * request, struct _u_response * response, void * user_data) {
  const char * response_type = u_map_get(request->map_url, "response_type");
  int result = U_CALLBACK_CONTINUE;
  char * state_encoded = NULL, * state_param = NULL;

  u_map_put(response->map_header, "Cache-Control", "no-store");
  u_map_put(response->map_header, "Pragma", "no-cache");
  u_map_put(response->map_header, "Referrer-Policy", "no-referrer");

  if (u_map_get(request->map_url, "state") != NULL) {
    state_encoded = ulfius_url_encode(u_map_get(request->map_url, "state"));
    state_param = msprintf("&state=%s", state_encoded);
    o_free(state_encoded);
  } else {
    state_param = o_strdup("");
  }

  if (0 == o_strcmp("code", response_type)) {
    if (is_authorization_type_enabled((struct _oauth2_config *)user_data, GLEWLWYD_AUTHORIZATION_TYPE_AUTHORIZATION_CODE) && u_map_get(request->map_url, "redirect_uri") != NULL) {
      result = check_auth_type_auth_code_grant(request, response, user_data);
    } else {
      response->status = 403;
    }
  } else if (0 == o_strcmp("token", response_type)) {
    if (is_authorization_type_enabled((struct _oauth2_config *)user_data, GLEWLWYD_AUTHORIZATION_TYPE_IMPLICIT) && u_map_get(request->map_url, "redirect_uri") != NULL) {
      result = check_auth_type_implicit_grant(request, response, user_data);
    } else {
      response->status = 403;
    }
  } else {
    response->status = 403;
  }
  o_free(state_param);

  return result;
}