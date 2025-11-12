int fcntl_setlk(unsigned int fd, struct file *filp, unsigned int cmd,
		struct flock *flock)
{
	struct file_lock *file_lock = locks_alloc_lock();
	struct inode *inode = file_inode(filp);
	struct file *f;
	int error;

	if (file_lock == NULL)
		return -ENOLCK;

	error = flock_to_posix_lock(filp, file_lock, flock);
	if (error)
		goto out;

	error = check_fmode_for_setlk(file_lock);
	if (error)
		goto out;

	/*
	 * If the cmd is requesting file-private locks, then set the
	 * FL_OFDLCK flag and override the owner.
	 */
	switch (cmd) {
	case F_OFD_SETLK:
		error = -EINVAL;
		if (flock->l_pid != 0)
			goto out;

		cmd = F_SETLK;
		file_lock->c.flc_flags |= FL_OFDLCK;
		file_lock->c.flc_owner = filp;
		break;
	case F_OFD_SETLKW:
		error = -EINVAL;
		if (flock->l_pid != 0)
			goto out;

		cmd = F_SETLKW;
		file_lock->c.flc_flags |= FL_OFDLCK;
		file_lock->c.flc_owner = filp;
		fallthrough;
	case F_SETLKW:
		file_lock->c.flc_flags |= FL_SLEEP;
	}

	error = do_lock_file_wait(filp, cmd, file_lock);

	/*
	 * Attempt to detect a close/fcntl race and recover by releasing the
	 * lock that was just acquired. There is no need to do that when we're
	 * unlocking though, or for OFD locks.
	 */
	if (!error && file_lock->c.flc_type != F_UNLCK &&
	    !(file_lock->c.flc_flags & FL_OFDLCK)) {
		struct files_struct *files = current->files;
		/*
		 * We need that spin_lock here - it prevents reordering between
		 * update of i_flctx->flc_posix and check for it done in
		 * close(). rcu_read_lock() wouldn't do.
		 */
		spin_lock(&files->file_lock);
		f = files_lookup_fd_locked(files, fd);
		spin_unlock(&files->file_lock);
		if (f != filp) {
			file_lock->c.flc_type = F_UNLCK;
			error = do_lock_file_wait(filp, cmd, file_lock);
			WARN_ON_ONCE(error);
			error = -EBADF;
		}
	}
out:
	trace_fcntl_setlk(inode, file_lock, error);
	locks_free_lock(file_lock);
	return error;
}
