const char *enter_repo(const char *path, int strict)
{
	static struct strbuf validated_path = STRBUF_INIT;
	static struct strbuf used_path = STRBUF_INIT;

	if (!path)
		return NULL;

	if (!strict) {
		static const char *suffix[] = {
			"/.git", "", ".git/.git", ".git", NULL,
		};
		const char *gitfile;
		int len = strlen(path);
		int i;
		while ((1 < len) && (path[len-1] == '/'))
			len--;

		/*
		 * We can handle arbitrary-sized buffers, but this remains as a
		 * sanity check on untrusted input.
		 */
		if (PATH_MAX <= len)
			return NULL;

		strbuf_reset(&used_path);
		strbuf_reset(&validated_path);
		strbuf_add(&used_path, path, len);
		strbuf_add(&validated_path, path, len);

		if (used_path.buf[0] == '~') {
			char *newpath = interpolate_path(used_path.buf, 0);
			if (!newpath)
				return NULL;
			strbuf_attach(&used_path, newpath, strlen(newpath),
				      strlen(newpath));
		}
		for (i = 0; suffix[i]; i++) {
			struct stat st;
			size_t baselen = used_path.len;
			strbuf_addstr(&used_path, suffix[i]);
			if (!stat(used_path.buf, &st) &&
			    (S_ISREG(st.st_mode) ||
			    (S_ISDIR(st.st_mode) && is_git_directory(used_path.buf)))) {
				strbuf_addstr(&validated_path, suffix[i]);
				break;
			}
			strbuf_setlen(&used_path, baselen);
		}
		if (!suffix[i])
			return NULL;
		gitfile = read_gitfile(used_path.buf);
		die_upon_dubious_ownership(gitfile, NULL, used_path.buf);
		if (gitfile) {
			strbuf_reset(&used_path);
			strbuf_addstr(&used_path, gitfile);
		}
		if (chdir(used_path.buf))
			return NULL;
		path = validated_path.buf;
	}
	else {
		const char *gitfile = read_gitfile(path);
		die_upon_dubious_ownership(gitfile, NULL, path);
		if (gitfile)
			path = gitfile;
		if (chdir(path))
			return NULL;
	}

	if (is_git_directory(".")) {
		set_git_dir(".", 0);
		check_repository_format(NULL);
		return path;
	}

	return NULL;
}