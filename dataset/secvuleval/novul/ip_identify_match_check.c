static int ip_identify_match_check(void *obj, void *arg, int flags)
{
	struct ip_identify_match *identify = obj;
	struct ast_sockaddr *addr = arg;
	int sense;

	sense = ast_apply_ha(identify->matches, addr);
	if (sense != AST_SENSE_ALLOW) {
		ast_debug(3, "Source address %s matches identify '%s'\n",
				ast_sockaddr_stringify(addr),
				ast_sorcery_object_get_id(identify));
		return CMP_MATCH;
	} else {
		ast_debug(3, "Source address %s does not match identify '%s'\n",
				ast_sockaddr_stringify(addr),
				ast_sorcery_object_get_id(identify));
		return 0;
	}
}