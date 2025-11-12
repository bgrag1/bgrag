void evict_inodes(struct super_block *sb)
{
	struct inode *inode, *next;
	LIST_HEAD(dispose);

again:
	spin_lock(&sb->s_inode_list_lock);
	list_for_each_entry_safe(inode, next, &sb->s_inodes, i_sb_list) {
		if (atomic_read(&inode->i_count))
			continue;

		spin_lock(&inode->i_lock);
		if (inode->i_state & (I_NEW | I_FREEING | I_WILL_FREE)) {
			spin_unlock(&inode->i_lock);
			continue;
		}

		inode->i_state |= I_FREEING;
		inode_lru_list_del(inode);
		spin_unlock(&inode->i_lock);
		list_add(&inode->i_lru, &dispose);

		/*
		 * We can have a ton of inodes to evict at unmount time given
		 * enough memory, check to see if we need to go to sleep for a
		 * bit so we don't livelock.
		 */
		if (need_resched()) {
			spin_unlock(&sb->s_inode_list_lock);
			cond_resched();
			dispose_list(&dispose);
			goto again;
		}
	}
	spin_unlock(&sb->s_inode_list_lock);

	dispose_list(&dispose);
}
