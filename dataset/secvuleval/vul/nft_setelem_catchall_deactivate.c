static int nft_setelem_catchall_deactivate(const struct net *net,
					   struct nft_set *set,
					   struct nft_set_elem *elem)
{
	struct nft_set_elem_catchall *catchall;
	struct nft_set_ext *ext;

	list_for_each_entry(catchall, &set->catchall_list, list) {
		ext = nft_set_elem_ext(set, catchall->elem);
		if (!nft_is_active(net, ext))
			continue;

		kfree(elem->priv);
		elem->priv = catchall->elem;
		nft_set_elem_change_active(net, set, ext);
		return 0;
	}

	return -ENOENT;
}
