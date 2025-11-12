static int load_module(void)
{
	ast_sorcery_apply_config(ast_sip_get_sorcery(), "res_pjsip_endpoint_identifier_ip");
	ast_sorcery_apply_default(ast_sip_get_sorcery(), "identify", "config", "pjsip.conf,criteria=type=identify");

	if (ast_sorcery_object_register(ast_sip_get_sorcery(), "identify", ip_identify_alloc, NULL, ip_identify_apply)) {
		return AST_MODULE_LOAD_DECLINE;
	}

	ast_sorcery_object_field_register(ast_sip_get_sorcery(), "identify", "type", "", OPT_NOOP_T, 0, 0);
	ast_sorcery_object_field_register(ast_sip_get_sorcery(), "identify", "endpoint", "", OPT_STRINGFIELD_T, 0, STRFLDSET(struct ip_identify_match, endpoint_name));
	ast_sorcery_object_field_register_custom(ast_sip_get_sorcery(), "identify", "match", "", ip_identify_match_handler, match_to_str, match_to_var_list, 0, 0);
	ast_sorcery_object_field_register(ast_sip_get_sorcery(), "identify", "match_header", "", OPT_STRINGFIELD_T, 0, STRFLDSET(struct ip_identify_match, match_header));
	ast_sorcery_object_field_register(ast_sip_get_sorcery(), "identify", "match_request_uri", "", OPT_STRINGFIELD_T, 0, STRFLDSET(struct ip_identify_match, match_request_uri));
	ast_sorcery_object_field_register(ast_sip_get_sorcery(), "identify", "srv_lookups", "yes", OPT_BOOL_T, 1, FLDSET(struct ip_identify_match, srv_lookups));
	ast_sorcery_object_field_register(ast_sip_get_sorcery(), "identify", "transport", "", OPT_STRINGFIELD_T, 0, STRFLDSET(struct ip_identify_match, transport));
	ast_sorcery_load_object(ast_sip_get_sorcery(), "identify");

	ast_sip_register_endpoint_identifier_with_name(&ip_identifier, "ip");
	ast_sip_register_endpoint_identifier_with_name(&header_identifier, "header");
	ast_sip_register_endpoint_identifier_with_name(&request_identifier, "request_uri");
	ast_sip_register_endpoint_identifier_with_name(&transport_identifier, "transport");
	ast_sip_register_endpoint_formatter(&endpoint_identify_formatter);

	cli_formatter = ao2_alloc(sizeof(struct ast_sip_cli_formatter_entry), NULL);
	if (!cli_formatter) {
		ast_log(LOG_ERROR, "Unable to allocate memory for cli formatter\n");
		return AST_MODULE_LOAD_DECLINE;
	}
	cli_formatter->name = "identify";
	cli_formatter->print_header = cli_print_header;
	cli_formatter->print_body = cli_print_body;
	cli_formatter->get_container = cli_get_container;
	cli_formatter->iterate = cli_iterator;
	cli_formatter->get_id = ast_sorcery_object_get_id;
	cli_formatter->retrieve_by_id = cli_retrieve_by_id;

	ast_sip_register_cli_formatter(cli_formatter);
	ast_cli_register_multiple(cli_identify, ARRAY_LEN(cli_identify));

	return AST_MODULE_LOAD_SUCCESS;
}