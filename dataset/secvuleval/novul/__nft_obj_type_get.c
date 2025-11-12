static const struct nft_object_type *__nft_obj_type_get(u32 objtype, u8 family)
{
	const struct nft_object_type *type;

	list_for_each_entry_rcu(type, &nf_tables_objects, list) {
		if (type->family != NFPROTO_UNSPEC &&
		    type->family != family)
			continue;

		if (objtype == type->type)
			return type;
	}
	return NULL;
}
