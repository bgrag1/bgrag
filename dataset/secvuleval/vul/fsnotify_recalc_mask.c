void fsnotify_recalc_mask(struct fsnotify_mark_connector *conn)
{
	if (!conn)
		return;

	spin_lock(&conn->lock);
	__fsnotify_recalc_mask(conn);
	spin_unlock(&conn->lock);
	if (conn->type == FSNOTIFY_OBJ_TYPE_INODE)
		__fsnotify_update_child_dentry_flags(
					fsnotify_conn_inode(conn));
}
