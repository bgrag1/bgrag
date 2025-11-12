static struct ast_sip_endpoint *transport_identify(pjsip_rx_data *rdata)
{
	char buffer[PJ_INET6_ADDRSTRLEN];
	pj_status_t status;
	struct ast_sockaddr_with_tp addr_with_tp = { { { 0, } }, };
	union pj_sockaddr sock = rdata->tp_info.transport->local_addr;

	pj_ansi_strxcpy(addr_with_tp.tp, rdata->tp_info.transport->type_name, sizeof(addr_with_tp.tp));

	if (sock.addr.sa_family == PJ_AF_INET6) {
		status = pj_inet_ntop(PJ_AF_INET6, &(sock.ipv6.sin6_addr), buffer, PJ_INET6_ADDRSTRLEN);
		if (status == PJ_SUCCESS && !strcmp(buffer, "::")) {
			ast_log(LOG_WARNING, "Matching against '::' may be unpredictable.\n");
		}
	} else {
		status = pj_inet_ntop(PJ_AF_INET, &(sock.ipv4.sin_addr), buffer, PJ_INET_ADDRSTRLEN);
		if (status == PJ_SUCCESS && !strcmp(buffer, "0.0.0.0")) {
			ast_log(LOG_WARNING, "Matching against '0.0.0.0' may be unpredictable.\n");
		}
	}

	if (status == PJ_SUCCESS) {
		ast_sockaddr_parse(&addr_with_tp.addr, buffer, PARSE_PORT_FORBID);
		ast_sockaddr_set_port(&addr_with_tp.addr, rdata->tp_info.transport->local_name.port);

		return common_identify(ip_identify_match_check, &addr_with_tp);
	} else {
		return NULL;
	}
}