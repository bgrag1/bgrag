static int dir_contains_only_dotgit(const char *path)
{
	DIR *dir = opendir(path);
	struct dirent *e;
	int ret = 1;

	if (!dir)
		return 0;

	e = readdir_skip_dot_and_dotdot(dir);
	if (!e)
		ret = 0;
	else if (strcmp(DEFAULT_GIT_DIR_ENVIRONMENT, e->d_name) ||
		 (e = readdir_skip_dot_and_dotdot(dir))) {
		error("unexpected item '%s' in '%s'", e->d_name, path);
		ret = 0;
	}

	closedir(dir);
	return ret;
}