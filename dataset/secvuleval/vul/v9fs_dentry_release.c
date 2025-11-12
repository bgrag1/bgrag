static void v9fs_dentry_release(struct dentry *dentry)
{
	struct hlist_node *p, *n;

	p9_debug(P9_DEBUG_VFS, " dentry: %pd (%p)\n",
		 dentry, dentry);
	hlist_for_each_safe(p, n, (struct hlist_head *)&dentry->d_fsdata)
		p9_fid_put(hlist_entry(p, struct p9_fid, dlist));
	dentry->d_fsdata = NULL;
}
