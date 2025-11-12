static void submit_flushes(struct work_struct *ws)
{
	struct mddev *mddev = container_of(ws, struct mddev, flush_work);
	struct md_rdev *rdev;

	mddev->start_flush = ktime_get_boottime();
	INIT_WORK(&mddev->flush_work, md_submit_flush_data);
	atomic_set(&mddev->flush_pending, 1);
	rcu_read_lock();
	rdev_for_each_rcu(rdev, mddev)
		if (rdev->raid_disk >= 0 &&
		    !test_bit(Faulty, &rdev->flags)) {
			struct bio *bi;

			atomic_inc(&rdev->nr_pending);
			rcu_read_unlock();
			bi = bio_alloc_bioset(rdev->bdev, 0,
					      REQ_OP_WRITE | REQ_PREFLUSH,
					      GFP_NOIO, &mddev->bio_set);
			bi->bi_end_io = md_end_flush;
			bi->bi_private = rdev;
			atomic_inc(&mddev->flush_pending);
			submit_bio(bi);
			rcu_read_lock();
		}
	rcu_read_unlock();
	if (atomic_dec_and_test(&mddev->flush_pending))
		queue_work(md_wq, &mddev->flush_work);
}
