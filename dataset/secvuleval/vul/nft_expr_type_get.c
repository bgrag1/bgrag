static const struct nft_expr_type *nft_expr_type_get(struct net *net,
						     u8 family,
						     struct nlattr *nla)
{
	const struct nft_expr_type *type;

	if (nla == NULL)
		return ERR_PTR(-EINVAL);

	type = __nft_expr_type_get(family, nla);
	if (type != NULL && try_module_get(type->owner))
		return type;

	lockdep_nfnl_nft_mutex_not_held();
// #ifdef CONFIG_MODULES
	if (type == NULL) {
		if (nft_expr_type_request_module(net, family, nla) == -EAGAIN)
			return ERR_PTR(-EAGAIN);

		if (nft_request_module(net, "nft-expr-%.*s",
				       nla_len(nla),
				       (char *)nla_data(nla)) == -EAGAIN)
			return ERR_PTR(-EAGAIN);
	}
#endif
	return ERR_PTR(-ENOENT);
}
