static int protect_dir(const char *path, mode_t mode, int do_mkdir,
	struct instance_data *idata)
{
	char *p = strdup(path);
	char *d;
	char *dir = p;
	int dfd = AT_FDCWD;
	int dfd_next;
	int save_errno;
	int flags = O_RDONLY | O_DIRECTORY;
	int rv = -1;
	struct stat st;

	if (p == NULL) {
		goto error;
	}

	if (*dir == '/') {
		dfd = open("/", flags);
		if (dfd == -1) {
			goto error;
		}
		dir++;	/* assume / is safe */
	}

	while ((d=strchr(dir, '/')) != NULL) {
		*d = '\0';
		dfd_next = openat(dfd, dir, flags);
		if (dfd_next == -1) {
			goto error;
		}

		if (dfd != AT_FDCWD)
			close(dfd);
		dfd = dfd_next;

		if (fstat(dfd, &st) != 0) {
			goto error;
		}

		if (flags & O_NOFOLLOW) {
			/* we are inside user-owned dir - protect */
			if (protect_mount(dfd, p, idata) == -1)
				goto error;
		} else if (st.st_uid != 0 || st.st_gid != 0 ||
			(st.st_mode & S_IWOTH)) {
			/* do not follow symlinks on subdirectories */
			flags |= O_NOFOLLOW;
		}

		*d = '/';
		dir = d + 1;
	}

	rv = openat(dfd, dir, flags);

	if (rv == -1) {
		if (!do_mkdir || mkdirat(dfd, dir, mode) != 0) {
			goto error;
		}
		rv = openat(dfd, dir, flags);
	}

	if (flags & O_NOFOLLOW) {
		/* we are inside user-owned dir - protect */
		if (protect_mount(rv, p, idata) == -1) {
			save_errno = errno;
			close(rv);
			rv = -1;
			errno = save_errno;
		}
	}

error:
	save_errno = errno;
	free(p);
	if (dfd != AT_FDCWD && dfd >= 0)
		close(dfd);
	errno = save_errno;

	return rv;
}