static void rt6_probe(struct fib6_nh *fib6_nh)
{
	struct __rt6_probe_work *work = NULL;
	const struct in6_addr *nh_gw;
	unsigned long last_probe;
	struct neighbour *neigh;
	struct net_device *dev;
	struct inet6_dev *idev;

	/*
	 * Okay, this does not seem to be appropriate
	 * for now, however, we need to check if it
	 * is really so; aka Router Reachability Probing.
	 *
	 * Router Reachability Probe MUST be rate-limited
	 * to no more than one per minute.
	 */
	if (!fib6_nh->fib_nh_gw_family)
		return;

	nh_gw = &fib6_nh->fib_nh_gw6;
	dev = fib6_nh->fib_nh_dev;
	rcu_read_lock();
	last_probe = READ_ONCE(fib6_nh->last_probe);
	idev = __in6_dev_get(dev);
	neigh = __ipv6_neigh_lookup_noref(dev, nh_gw);
	if (neigh) {
		if (READ_ONCE(neigh->nud_state) & NUD_VALID)
			goto out;

		write_lock_bh(&neigh->lock);
		if (!(neigh->nud_state & NUD_VALID) &&
		    time_after(jiffies,
			       neigh->updated +
			       READ_ONCE(idev->cnf.rtr_probe_interval))) {
			work = kmalloc(sizeof(*work), GFP_ATOMIC);
			if (work)
				__neigh_set_probe_once(neigh);
		}
		write_unlock_bh(&neigh->lock);
	} else if (time_after(jiffies, last_probe +
				       READ_ONCE(idev->cnf.rtr_probe_interval))) {
		work = kmalloc(sizeof(*work), GFP_ATOMIC);
	}

	if (!work || cmpxchg(&fib6_nh->last_probe,
			     last_probe, jiffies) != last_probe) {
		kfree(work);
	} else {
		INIT_WORK(&work->work, rt6_probe_deferred);
		work->target = *nh_gw;
		netdev_hold(dev, &work->dev_tracker, GFP_ATOMIC);
		work->dev = dev;
		schedule_work(&work->work);
	}

out:
	rcu_read_unlock();
}
