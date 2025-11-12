static struct eventfs_inode *eventfs_find_events(struct dentry *dentry)
{
	struct eventfs_inode *ei;

	do {
		// The parent is stable because we do not do renames
		dentry = dentry->d_parent;
		// ... and directories always have d_fsdata
		ei = dentry->d_fsdata;

		/*
		 * If the ei is being freed, the ownership of the children
		 * doesn't matter.
		 */
		if (ei->is_freed) {
			ei = NULL;
			break;
		}
		// Walk upwards until you find the events inode
	} while (!ei->is_events);

	update_events_attr(ei, dentry->d_sb);

	return ei;
}
