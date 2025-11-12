static int nilfs_btree_check_delete(struct nilfs_bmap *btree, __u64 key)
{
	struct buffer_head *bh;
	struct nilfs_btree_node *root, *node;
	__u64 maxkey, nextmaxkey;
	__u64 ptr;
	int nchildren, ret;

	root = nilfs_btree_get_root(btree);
	switch (nilfs_btree_height(btree)) {
	case 2:
		bh = NULL;
		node = root;
		break;
	case 3:
		nchildren = nilfs_btree_node_get_nchildren(root);
		if (nchildren > 1)
			return 0;
		ptr = nilfs_btree_node_get_ptr(root, nchildren - 1,
					       NILFS_BTREE_ROOT_NCHILDREN_MAX);
		ret = nilfs_btree_get_block(btree, ptr, &bh);
		if (ret < 0)
			return ret;
		node = (struct nilfs_btree_node *)bh->b_data;
		break;
	default:
		return 0;
	}

	nchildren = nilfs_btree_node_get_nchildren(node);
	maxkey = nilfs_btree_node_get_key(node, nchildren - 1);
	nextmaxkey = (nchildren > 1) ?
		nilfs_btree_node_get_key(node, nchildren - 2) : 0;
	brelse(bh);

	return (maxkey == key) && (nextmaxkey < NILFS_BMAP_LARGE_LOW);
}
