static const char *sip_endpoint_identifier_type2str(enum ast_sip_endpoint_identifier_type method)
{
	const char *str = "<unknown>";

	switch (method) {
	case AST_SIP_ENDPOINT_IDENTIFY_BY_USERNAME:
		str = "username";
		break;
	case AST_SIP_ENDPOINT_IDENTIFY_BY_AUTH_USERNAME:
		str = "auth_username";
		break;
	case AST_SIP_ENDPOINT_IDENTIFY_BY_IP:
		str = "ip";
		break;
	case AST_SIP_ENDPOINT_IDENTIFY_BY_HEADER:
		str = "header";
		break;
	case AST_SIP_ENDPOINT_IDENTIFY_BY_REQUEST_URI:
		str = "request_uri";
		break;
	case AST_SIP_ENDPOINT_IDENTIFY_BY_TRANSPORT:
		str = "transport";
		break;
	}
	return str;
}