void gfs2_jindex_free(struct gfs2_sbd *sdp)
{
	struct list_head list;
	struct gfs2_jdesc *jd;

	spin_lock(&sdp->sd_jindex_spin);
	list_add(&list, &sdp->sd_jindex_list);
	list_del_init(&sdp->sd_jindex_list);
	sdp->sd_journals = 0;
	spin_unlock(&sdp->sd_jindex_spin);

	sdp->sd_jdesc = NULL;
	while (!list_empty(&list)) {
		jd = list_first_entry(&list, struct gfs2_jdesc, jd_list);
		gfs2_free_journal_extents(jd);
		list_del(&jd->jd_list);
		iput(jd->jd_inode);
		jd->jd_inode = NULL;
		kfree(jd);
	}
}
