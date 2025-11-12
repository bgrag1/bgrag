void register_winch_irq(int fd, int tty_fd, int pid, struct tty_port *port,
			unsigned long stack)
{
	struct winch *winch;

	winch = kmalloc(sizeof(*winch), GFP_KERNEL);
	if (winch == NULL) {
		printk(KERN_ERR "register_winch_irq - kmalloc failed\n");
		goto cleanup;
	}

	*winch = ((struct winch) { .fd  	= fd,
				   .tty_fd 	= tty_fd,
				   .pid  	= pid,
				   .port 	= port,
				   .stack	= stack });

	spin_lock(&winch_handler_lock);
	list_add(&winch->list, &winch_handlers);
	spin_unlock(&winch_handler_lock);

	if (um_request_irq(WINCH_IRQ, fd, IRQ_READ, winch_interrupt,
			   IRQF_SHARED, "winch", winch) < 0) {
		printk(KERN_ERR "register_winch_irq - failed to register "
		       "IRQ\n");
		spin_lock(&winch_handler_lock);
		list_del(&winch->list);
		spin_unlock(&winch_handler_lock);
		goto out_free;
	}

	return;

 out_free:
	kfree(winch);
 cleanup:
	os_kill_process(pid, 1);
	os_close_file(fd);
	if (stack != 0)
		free_stack(stack, 0);
}
