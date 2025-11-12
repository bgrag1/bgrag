void f2fs_init_read_extent_tree(struct inode *inode, struct page *ipage)
{
	struct f2fs_sb_info *sbi = F2FS_I_SB(inode);
	struct extent_tree_info *eti = &sbi->extent_tree[EX_READ];
	struct f2fs_extent *i_ext = &F2FS_INODE(ipage)->i_ext;
	struct extent_tree *et;
	struct extent_node *en;
	struct extent_info ei;

	if (!__may_extent_tree(inode, EX_READ)) {
		/* drop largest read extent */
		if (i_ext && i_ext->len) {
			f2fs_wait_on_page_writeback(ipage, NODE, true, true);
			i_ext->len = 0;
			set_page_dirty(ipage);
		}
		goto out;
	}

	et = __grab_extent_tree(inode, EX_READ);

	if (!i_ext || !i_ext->len)
		goto out;

	get_read_extent_info(&ei, i_ext);

	write_lock(&et->lock);
	if (atomic_read(&et->node_cnt))
		goto unlock_out;

	en = __attach_extent_node(sbi, et, &ei, NULL,
				&et->root.rb_root.rb_node, true);
	if (en) {
		et->largest = en->ei;
		et->cached_en = en;

		spin_lock(&eti->extent_lock);
		list_add_tail(&en->list, &eti->extent_list);
		spin_unlock(&eti->extent_lock);
	}
unlock_out:
	write_unlock(&et->lock);
out:
	if (!F2FS_I(inode)->extent_tree[EX_READ])
		set_inode_flag(inode, FI_NO_EXTENT);
}
