static int sip_endpoint_identifier_str2type(const char *str)
{
	int method;

	if (!strcasecmp(str, "username")) {
		method = AST_SIP_ENDPOINT_IDENTIFY_BY_USERNAME;
	} else if (!strcasecmp(str, "auth_username")) {
		method = AST_SIP_ENDPOINT_IDENTIFY_BY_AUTH_USERNAME;
	} else if (!strcasecmp(str, "ip")) {
		method = AST_SIP_ENDPOINT_IDENTIFY_BY_IP;
	} else if (!strcasecmp(str, "header")) {
		method = AST_SIP_ENDPOINT_IDENTIFY_BY_HEADER;
	} else if (!strcasecmp(str, "request_uri")) {
		method = AST_SIP_ENDPOINT_IDENTIFY_BY_REQUEST_URI;
	} else if (!strcasecmp(str, "transport")) {
		method = AST_SIP_ENDPOINT_IDENTIFY_BY_TRANSPORT;
	} else {
		method = -1;
	}
	return method;
}