static int privcmd_irqfd_assign(struct privcmd_irqfd *irqfd)
{
	struct privcmd_kernel_irqfd *kirqfd, *tmp;
	unsigned long flags;
	__poll_t events;
	struct fd f;
	void *dm_op;
	int ret;

	kirqfd = kzalloc(sizeof(*kirqfd) + irqfd->size, GFP_KERNEL);
	if (!kirqfd)
		return -ENOMEM;
	dm_op = kirqfd + 1;

	if (copy_from_user(dm_op, u64_to_user_ptr(irqfd->dm_op), irqfd->size)) {
		ret = -EFAULT;
		goto error_kfree;
	}

	kirqfd->xbufs.size = irqfd->size;
	set_xen_guest_handle(kirqfd->xbufs.h, dm_op);
	kirqfd->dom = irqfd->dom;
	INIT_WORK(&kirqfd->shutdown, irqfd_shutdown);

	f = fdget(irqfd->fd);
	if (!f.file) {
		ret = -EBADF;
		goto error_kfree;
	}

	kirqfd->eventfd = eventfd_ctx_fileget(f.file);
	if (IS_ERR(kirqfd->eventfd)) {
		ret = PTR_ERR(kirqfd->eventfd);
		goto error_fd_put;
	}

	/*
	 * Install our own custom wake-up handling so we are notified via a
	 * callback whenever someone signals the underlying eventfd.
	 */
	init_waitqueue_func_entry(&kirqfd->wait, irqfd_wakeup);
	init_poll_funcptr(&kirqfd->pt, irqfd_poll_func);

	spin_lock_irqsave(&irqfds_lock, flags);

	list_for_each_entry(tmp, &irqfds_list, list) {
		if (kirqfd->eventfd == tmp->eventfd) {
			ret = -EBUSY;
			spin_unlock_irqrestore(&irqfds_lock, flags);
			goto error_eventfd;
		}
	}

	list_add_tail(&kirqfd->list, &irqfds_list);
	spin_unlock_irqrestore(&irqfds_lock, flags);

	/*
	 * Check if there was an event already pending on the eventfd before we
	 * registered, and trigger it as if we didn't miss it.
	 */
	events = vfs_poll(f.file, &kirqfd->pt);
	if (events & EPOLLIN)
		irqfd_inject(kirqfd);

	/*
	 * Do not drop the file until the kirqfd is fully initialized, otherwise
	 * we might race against the EPOLLHUP.
	 */
	fdput(f);
	return 0;

error_eventfd:
	eventfd_ctx_put(kirqfd->eventfd);

error_fd_put:
	fdput(f);

error_kfree:
	kfree(kirqfd);
	return ret;
}
