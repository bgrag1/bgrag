static const struct nft_expr_type *__nft_expr_type_get(u8 family,
						       struct nlattr *nla)
{
	const struct nft_expr_type *type, *candidate = NULL;

	list_for_each_entry_rcu(type, &nf_tables_expressions, list) {
		if (!nla_strcmp(nla, type->name)) {
			if (!type->family && !candidate)
				candidate = type;
			else if (type->family == family)
				candidate = type;
		}
	}
	return candidate;
}
