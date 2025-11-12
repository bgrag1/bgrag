static int unload_module(void)
{
	ast_cli_unregister_multiple(cli_identify, ARRAY_LEN(cli_identify));
	ast_sip_unregister_cli_formatter(cli_formatter);
	ast_sip_unregister_endpoint_formatter(&endpoint_identify_formatter);
	ast_sip_unregister_endpoint_identifier(&header_identifier);
	ast_sip_unregister_endpoint_identifier(&request_identifier);
	ast_sip_unregister_endpoint_identifier(&ip_identifier);
	ast_sip_unregister_endpoint_identifier(&transport_identifier);

	return 0;
}