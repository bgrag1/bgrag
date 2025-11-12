static int ip_identify_match_check(void *obj, void *arg, int flags)
{
	struct ip_identify_match *identify = obj;
	struct ast_sockaddr_with_tp *addr_with_tp = arg;
	struct ast_sockaddr address = addr_with_tp->addr;
	int sense;

	sense = ast_apply_ha(identify->matches, &address);
	if (sense != AST_SENSE_ALLOW) {
		ast_debug(3, "Address %s matches identify '%s'\n",
				ast_sockaddr_stringify(&address),
				ast_sorcery_object_get_id(identify));
		if (ast_strlen_zero(identify->transport) || !strcasecmp(identify->transport, addr_with_tp->tp)) {
			ast_debug(3, "Transport %s matches identify '%s'\n",
				addr_with_tp->tp,
				ast_sorcery_object_get_id(identify));
			return CMP_MATCH;
		} else {
			ast_debug(3, "Transport %s match not matched identify '%s'\n",
				addr_with_tp->tp,
				ast_sorcery_object_get_id(identify));
			return 0;
		}
	} else {
		ast_debug(3, "Address %s does not match identify '%s'\n",
				ast_sockaddr_stringify(&address),
				ast_sorcery_object_get_id(identify));
		return 0;
	}
}