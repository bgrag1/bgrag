void gfs2_jindex_free(struct gfs2_sbd *sdp)
{
	struct list_head list;
	struct gfs2_jdesc *jd;

	spin_lock(&sdp->sd_jindex_spin);
	list_add(&list, &sdp->sd_jindex_list);
	list_del_init(&sdp->sd_jindex_list);
	sdp->sd_journals = 0;
	spin_unlock(&sdp->sd_jindex_spin);

	down_write(&sdp->sd_log_flush_lock);
	sdp->sd_jdesc = NULL;
	up_write(&sdp->sd_log_flush_lock);

	while (!list_empty(&list)) {
		jd = list_first_entry(&list, struct gfs2_jdesc, jd_list);
		BUG_ON(jd->jd_log_bio);
		gfs2_free_journal_extents(jd);
		list_del(&jd->jd_list);
		iput(jd->jd_inode);
		jd->jd_inode = NULL;
		kfree(jd);
	}
}
