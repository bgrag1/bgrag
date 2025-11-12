int __fsnotify_parent(struct dentry *dentry, __u32 mask, const void *data,
		      int data_type)
{
	const struct path *path = fsnotify_data_path(data, data_type);
	__u32 mnt_mask = path ? real_mount(path->mnt)->mnt_fsnotify_mask : 0;
	struct inode *inode = d_inode(dentry);
	struct dentry *parent;
	bool parent_watched = dentry->d_flags & DCACHE_FSNOTIFY_PARENT_WATCHED;
	bool parent_needed, parent_interested;
	__u32 p_mask;
	struct inode *p_inode = NULL;
	struct name_snapshot name;
	struct qstr *file_name = NULL;
	int ret = 0;

	/* Optimize the likely case of nobody watching this path */
	if (likely(!parent_watched &&
		   !fsnotify_object_watched(inode, mnt_mask, mask)))
		return 0;

	parent = NULL;
	parent_needed = fsnotify_event_needs_parent(inode, mnt_mask, mask);
	if (!parent_watched && !parent_needed)
		goto notify;

	/* Does parent inode care about events on children? */
	parent = dget_parent(dentry);
	p_inode = parent->d_inode;
	p_mask = fsnotify_inode_watches_children(p_inode);
	if (unlikely(parent_watched && !p_mask))
		__fsnotify_update_child_dentry_flags(p_inode);

	/*
	 * Include parent/name in notification either if some notification
	 * groups require parent info or the parent is interested in this event.
	 */
	parent_interested = mask & p_mask & ALL_FSNOTIFY_EVENTS;
	if (parent_needed || parent_interested) {
		/* When notifying parent, child should be passed as data */
		WARN_ON_ONCE(inode != fsnotify_data_inode(data, data_type));

		/* Notify both parent and child with child name info */
		take_dentry_name_snapshot(&name, dentry);
		file_name = &name.name;
		if (parent_interested)
			mask |= FS_EVENT_ON_CHILD;
	}

notify:
	ret = fsnotify(mask, data, data_type, p_inode, file_name, inode, 0);

	if (file_name)
		release_dentry_name_snapshot(&name);
	dput(parent);

	return ret;
}
