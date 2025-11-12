static int cli_print_body(void *obj, void *arg, int flags)
{
	RAII_VAR(struct ast_str *, str, ast_str_create(MAX_OBJECT_FIELD), ast_free);
	struct ip_identify_match *ident = obj;
	struct ast_sip_cli_context *context = arg;
	struct ast_ha *match;
	int indent;

	ast_assert(context->output_buffer != NULL);

	ast_str_append(&context->output_buffer, 0, "%*s:  %s/%s\n",
		CLI_INDENT_TO_SPACES(context->indent_level), "Identify",
		ast_sorcery_object_get_id(ident), ident->endpoint_name);

	if (context->recurse) {
		context->indent_level++;
		indent = CLI_INDENT_TO_SPACES(context->indent_level);

		for (match = ident->matches; match; match = match->next) {
			const char *addr;

			if (ast_sockaddr_port(&match->addr)) {
				addr = ast_sockaddr_stringify(&match->addr);
			} else {
				addr = ast_sockaddr_stringify_addr(&match->addr);
			}

			ast_str_append(&context->output_buffer, 0, "%*s: %s%s/%d\n",
				indent,
				"Match",
				match->sense == AST_SENSE_ALLOW ? "!" : "",
				addr, ast_sockaddr_cidr_bits(&match->netmask));
		}

		if (!ast_strlen_zero(ident->transport)) {
			ast_str_append(&context->output_buffer, 0, "%*s: %s\n",
				indent,
				"Transport",
				ident->transport);
		}

		if (!ast_strlen_zero(ident->match_header)) {
			ast_str_append(&context->output_buffer, 0, "%*s: %s\n",
				indent,
				"Header",
				ident->match_header);
		}

		if (!ast_strlen_zero(ident->match_request_uri)) {
			ast_str_append(&context->output_buffer, 0, "%*s: %s\n",
				indent,
				"RequestURI",
				ident->match_request_uri);
		}

		context->indent_level--;

		if (context->indent_level == 0) {
			ast_str_append(&context->output_buffer, 0, "\n");
		}
	}

	if (context->show_details
		|| (context->show_details_only_level_0 && context->indent_level == 0)) {
		ast_str_append(&context->output_buffer, 0, "\n");
		ast_sip_cli_print_sorcery_objectset(ident, context, 0);
	}

	return 0;
}