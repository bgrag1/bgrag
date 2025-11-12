static const struct nft_object_type *
nft_obj_type_get(struct net *net, u32 objtype, u8 family)
{
	const struct nft_object_type *type;

	type = __nft_obj_type_get(objtype, family);
	if (type != NULL && try_module_get(type->owner))
		return type;

	lockdep_nfnl_nft_mutex_not_held();
// #ifdef CONFIG_MODULES
	if (type == NULL) {
		if (nft_request_module(net, "nft-obj-%u", objtype) == -EAGAIN)
			return ERR_PTR(-EAGAIN);
	}
#endif
	return ERR_PTR(-ENOENT);
}
