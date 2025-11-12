bool sanity_check_extent_cache(struct inode *inode)
{
	struct f2fs_sb_info *sbi = F2FS_I_SB(inode);
	struct f2fs_inode_info *fi = F2FS_I(inode);
	struct extent_tree *et = fi->extent_tree[EX_READ];
	struct extent_info *ei;

	if (!et)
		return true;

	ei = &et->largest;
	if (!ei->len)
		return true;

	/* Let's drop, if checkpoint got corrupted. */
	if (is_set_ckpt_flags(sbi, CP_ERROR_FLAG)) {
		ei->len = 0;
		et->largest_updated = true;
		return true;
	}

	if (!f2fs_is_valid_blkaddr(sbi, ei->blk, DATA_GENERIC_ENHANCE) ||
	    !f2fs_is_valid_blkaddr(sbi, ei->blk + ei->len - 1,
					DATA_GENERIC_ENHANCE)) {
		f2fs_warn(sbi, "%s: inode (ino=%lx) extent info [%u, %u, %u] is incorrect, run fsck to fix",
			  __func__, inode->i_ino,
			  ei->blk, ei->fofs, ei->len);
		return false;
	}
	return true;
}
