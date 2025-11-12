static struct ast_sip_endpoint *ip_identify(pjsip_rx_data *rdata)
{
	struct ast_sockaddr addr = { { 0, } };

	ast_sockaddr_parse(&addr, rdata->pkt_info.src_name, PARSE_PORT_FORBID);
	ast_sockaddr_set_port(&addr, rdata->pkt_info.src_port);

	return common_identify(ip_identify_match_check, &addr);
}