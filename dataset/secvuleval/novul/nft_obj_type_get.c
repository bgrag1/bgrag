static const struct nft_object_type *
nft_obj_type_get(struct net *net, u32 objtype, u8 family)
{
	const struct nft_object_type *type;

	rcu_read_lock();
	type = __nft_obj_type_get(objtype, family);
	if (type != NULL && try_module_get(type->owner)) {
		rcu_read_unlock();
		return type;
	}
	rcu_read_unlock();

	lockdep_nfnl_nft_mutex_not_held();
// #ifdef CONFIG_MODULES
	if (type == NULL) {
		if (nft_request_module(net, "nft-obj-%u", objtype) == -EAGAIN)
			return ERR_PTR(-EAGAIN);
	}
#endif
	return ERR_PTR(-ENOENT);
}
