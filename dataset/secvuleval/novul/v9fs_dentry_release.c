static void v9fs_dentry_release(struct dentry *dentry)
{
	struct hlist_node *p, *n;
	struct hlist_head head;

	p9_debug(P9_DEBUG_VFS, " dentry: %pd (%p)\n",
		 dentry, dentry);

	spin_lock(&dentry->d_lock);
	hlist_move_list((struct hlist_head *)&dentry->d_fsdata, &head);
	spin_unlock(&dentry->d_lock);

	hlist_for_each_safe(p, n, &head)
		p9_fid_put(hlist_entry(p, struct p9_fid, dlist));
}
