void fsnotify_recalc_mask(struct fsnotify_mark_connector *conn)
{
	bool update_children;

	if (!conn)
		return;

	spin_lock(&conn->lock);
	update_children = !fsnotify_conn_watches_children(conn);
	__fsnotify_recalc_mask(conn);
	update_children &= fsnotify_conn_watches_children(conn);
	spin_unlock(&conn->lock);
	/*
	 * Set children's PARENT_WATCHED flags only if parent started watching.
	 * When parent stops watching, we clear false positive PARENT_WATCHED
	 * flags lazily in __fsnotify_parent().
	 */
	if (update_children)
		fsnotify_conn_set_children_dentry_flags(conn);
}
