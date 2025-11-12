bool sanity_check_extent_cache(struct inode *inode, struct page *ipage)
{
	struct f2fs_sb_info *sbi = F2FS_I_SB(inode);
	struct f2fs_extent *i_ext = &F2FS_INODE(ipage)->i_ext;
	struct extent_info ei;

	get_read_extent_info(&ei, i_ext);

	if (!ei.len)
		return true;

	if (!f2fs_is_valid_blkaddr(sbi, ei.blk, DATA_GENERIC_ENHANCE) ||
	    !f2fs_is_valid_blkaddr(sbi, ei.blk + ei.len - 1,
					DATA_GENERIC_ENHANCE)) {
		f2fs_warn(sbi, "%s: inode (ino=%lx) extent info [%u, %u, %u] is incorrect, run fsck to fix",
			  __func__, inode->i_ino,
			  ei.blk, ei.fofs, ei.len);
		return false;
	}
	return true;
}
