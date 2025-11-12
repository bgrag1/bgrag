static struct ast_sip_endpoint *ip_identify(pjsip_rx_data *rdata)
{
	struct ast_sockaddr_with_tp addr_with_tp = { { { 0, } }, };
	pj_ansi_strxcpy(addr_with_tp.tp, rdata->tp_info.transport->type_name, sizeof(addr_with_tp.tp));

	ast_sockaddr_parse(&addr_with_tp.addr, rdata->pkt_info.src_name, PARSE_PORT_FORBID);
	ast_sockaddr_set_port(&addr_with_tp.addr, rdata->pkt_info.src_port);

	return common_identify(ip_identify_match_check, &addr_with_tp);
}