static void md_submit_flush_data(struct work_struct *ws)
{
	struct mddev *mddev = container_of(ws, struct mddev, flush_work);
	struct bio *bio = mddev->flush_bio;

	/*
	 * must reset flush_bio before calling into md_handle_request to avoid a
	 * deadlock, because other bios passed md_handle_request suspend check
	 * could wait for this and below md_handle_request could wait for those
	 * bios because of suspend check
	 */
	spin_lock_irq(&mddev->lock);
	mddev->prev_flush_start = mddev->start_flush;
	mddev->flush_bio = NULL;
	spin_unlock_irq(&mddev->lock);
	wake_up(&mddev->sb_wait);

	if (bio->bi_iter.bi_size == 0) {
		/* an empty barrier - all done */
		bio_endio(bio);
	} else {
		bio->bi_opf &= ~REQ_PREFLUSH;
		md_handle_request(mddev, bio);
	}
}
