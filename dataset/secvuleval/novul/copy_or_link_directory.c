static void copy_or_link_directory(struct strbuf *src, struct strbuf *dest,
				   const char *src_repo)
{
	int src_len, dest_len;
	struct dir_iterator *iter;
	int iter_status;

	/*
	 * Refuse copying directories by default which aren't owned by us. The
	 * code that performs either the copying or hardlinking is not prepared
	 * to handle various edge cases where an adversary may for example
	 * racily swap out files for symlinks. This can cause us to
	 * inadvertently use the wrong source file.
	 *
	 * Furthermore, even if we were prepared to handle such races safely,
	 * creating hardlinks across user boundaries is an inherently unsafe
	 * operation as the hardlinked files can be rewritten at will by the
	 * potentially-untrusted user. We thus refuse to do so by default.
	 */
	die_upon_dubious_ownership(NULL, NULL, src_repo);

	mkdir_if_missing(dest->buf, 0777);

	iter = dir_iterator_begin(src->buf, DIR_ITERATOR_PEDANTIC);

	if (!iter)
		die_errno(_("failed to start iterator over '%s'"), src->buf);

	strbuf_addch(src, '/');
	src_len = src->len;
	strbuf_addch(dest, '/');
	dest_len = dest->len;

	while ((iter_status = dir_iterator_advance(iter)) == ITER_OK) {
		strbuf_setlen(src, src_len);
		strbuf_addstr(src, iter->relative_path);
		strbuf_setlen(dest, dest_len);
		strbuf_addstr(dest, iter->relative_path);

		if (S_ISLNK(iter->st.st_mode))
			die(_("symlink '%s' exists, refusing to clone with --local"),
			    iter->relative_path);

		if (S_ISDIR(iter->st.st_mode)) {
			mkdir_if_missing(dest->buf, 0777);
			continue;
		}

		/* Files that cannot be copied bit-for-bit... */
		if (!fspathcmp(iter->relative_path, "info/alternates")) {
			copy_alternates(src, src_repo);
			continue;
		}

		if (unlink(dest->buf) && errno != ENOENT)
			die_errno(_("failed to unlink '%s'"), dest->buf);
		if (!option_no_hardlinks) {
			if (!link(src->buf, dest->buf)) {
				struct stat st;

				/*
				 * Sanity-check whether the created hardlink
				 * actually links to the expected file now. This
				 * catches time-of-check-time-of-use bugs in
				 * case the source file was meanwhile swapped.
				 */
				if (lstat(dest->buf, &st))
					die(_("hardlink cannot be checked at '%s'"), dest->buf);
				if (st.st_mode != iter->st.st_mode ||
				    st.st_ino != iter->st.st_ino ||
				    st.st_dev != iter->st.st_dev ||
				    st.st_size != iter->st.st_size ||
				    st.st_uid != iter->st.st_uid ||
				    st.st_gid != iter->st.st_gid)
					die(_("hardlink different from source at '%s'"), dest->buf);

				continue;
			}
			if (option_local > 0)
				die_errno(_("failed to create link '%s'"), dest->buf);
			option_no_hardlinks = 1;
		}
		if (copy_file_with_time(dest->buf, src->buf, 0666))
			die_errno(_("failed to copy file to '%s'"), dest->buf);
	}

	if (iter_status != ITER_DONE) {
		strbuf_setlen(src, src_len);
		die(_("failed to iterate over '%s'"), src->buf);
	}
}