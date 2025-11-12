static int ip_identify_apply(const struct ast_sorcery *sorcery, void *obj)
{
	struct ip_identify_match *identify = obj;
	char *current_string;
	struct ao2_iterator i;

	/* Validate the identify object configuration */
	if (ast_strlen_zero(identify->endpoint_name)) {
		ast_log(LOG_ERROR, "Identify '%s' missing required endpoint name.\n",
			ast_sorcery_object_get_id(identify));
		return -1;
	}
	if (ast_strlen_zero(identify->match_header) /* No header to match */
		&& ast_strlen_zero(identify->match_request_uri) /* and no request to match */
		/* and no static IP addresses with a mask */
		&& !identify->matches
		/* and no addresses to resolve */
		&& (!identify->hosts || !ao2_container_count(identify->hosts))) {
		ast_log(LOG_ERROR, "Identify '%s' is not configured to match anything.\n",
			ast_sorcery_object_get_id(identify));
		return -1;
	}

	if (!ast_strlen_zero(identify->transport)) {
		if (ast_string_field_set(identify, transport, identify->transport)) {
			return -1;
		}
	}

	if (!ast_strlen_zero(identify->match_header)) {
		char *c_header;
		char *c_value;
		int len;

		/* Split the header name and value */
		c_header = ast_strdupa(identify->match_header);
		c_value = strchr(c_header, ':');
		if (!c_value) {
			ast_log(LOG_ERROR, "Identify '%s' missing ':' separator in match_header '%s'.\n",
				ast_sorcery_object_get_id(identify), identify->match_header);
			return -1;
		}
		*c_value = '\0';
		c_value = ast_strip(c_value + 1);
		c_header = ast_strip(c_header);

		if (ast_strlen_zero(c_header)) {
			ast_log(LOG_ERROR, "Identify '%s' has no SIP header to match in match_header '%s'.\n",
				ast_sorcery_object_get_id(identify), identify->match_header);
			return -1;
		}

		if (!strcmp(c_value, "//")) {
			/* An empty regex is the same as an empty literal string. */
			c_value = "";
		}

		if (ast_string_field_set(identify, match_header_name, c_header)
			|| ast_string_field_set(identify, match_header_value, c_value)) {
			return -1;
		}

		len = strlen(c_value);
		if (2 < len && c_value[0] == '/' && c_value[len - 1] == '/') {
			/* Make "/regex/" into "regex" */
			c_value[len - 1] = '\0';
			++c_value;

			if (regcomp(&identify->regex_header_buf, c_value, REG_EXTENDED | REG_NOSUB)) {
				ast_log(LOG_ERROR, "Identify '%s' failed to compile match_request_uri regex '%s'.\n",
					ast_sorcery_object_get_id(identify), c_value);
				return -1;
			}
			identify->is_header_regex = 1;
		}
	}

	if (!ast_strlen_zero(identify->match_request_uri)) {
		char *c_string;
		int len;

		len = strlen(identify->match_request_uri);
		c_string = ast_strdupa(identify->match_request_uri);

		if (!strcmp(c_string, "//")) {
			/* An empty regex is the same as an empty literal string. */
			c_string = "";
		}

		if (2 < len && c_string[0] == '/' && c_string[len - 1] == '/') {
			/* Make "/regex/" into "regex" */
			c_string[len - 1] = '\0';
			++c_string;

			if (regcomp(&identify->regex_request_uri_buf, c_string, REG_EXTENDED | REG_NOSUB)) {
				ast_log(LOG_ERROR, "Identify '%s' failed to compile match_header regex '%s'.\n",
					ast_sorcery_object_get_id(identify), c_string);
				return -1;
			}
			identify->is_request_uri_regex = 1;
		}
	}

	if (!identify->hosts) {
		/* No match addresses to resolve */
		return 0;
	}

	/* Hosts can produce dynamic content, so mark the identify as such */
	ast_sorcery_object_set_has_dynamic_contents(obj);

	/* Resolve the match addresses now */
	i = ao2_iterator_init(identify->hosts, 0);
	while ((current_string = ao2_iterator_next(&i))) {
		int results = 0;
		char *colon = strrchr(current_string, ':');

		/* We skip SRV lookup if a colon is present, assuming a port was specified */
		if (!colon) {
			/* No port, and we know this is not an IP address, so perform SRV resolution on it */
			if (identify->srv_lookups) {
				results = ip_identify_match_srv_lookup(identify, "_sip._udp", current_string,
					results);
				if (results != -1) {
					results = ip_identify_match_srv_lookup(identify, "_sip._tcp",
						current_string, results);
				}
				if (results != -1) {
					results = ip_identify_match_srv_lookup(identify, "_sips._tcp",
						current_string, results);
				}
			}
		}

		/* If SRV fails fall back to a normal lookup on the host itself */
		if (!results) {
			results = ip_identify_match_host_lookup(identify, current_string);
		}

		if (results == 0) {
			ast_log(LOG_WARNING, "Identify '%s' provided address '%s' did not resolve to any address\n",
				ast_sorcery_object_get_id(identify), current_string);
		} else if (results == -1) {
			ast_log(LOG_ERROR, "Identify '%s' failed when adding resolution results of '%s'\n",
				ast_sorcery_object_get_id(identify), current_string);
			ao2_ref(current_string, -1);
			ao2_iterator_destroy(&i);
			return -1;
		}

		ao2_ref(current_string, -1);
	}
	ao2_iterator_destroy(&i);

	ao2_ref(identify->hosts, -1);
	identify->hosts = NULL;

	return 0;
}