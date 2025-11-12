static int fetch_objects(struct repository *repo,
			 const char *remote_name,
			 const struct object_id *oids,
			 int oid_nr)
{
	struct child_process child = CHILD_PROCESS_INIT;
	int i;
	FILE *child_in;

	/* TODO: This should use NO_LAZY_FETCH_ENVIRONMENT */
	if (git_env_bool("GIT_NO_LAZY_FETCH", 0)) {
		static int warning_shown;
		if (!warning_shown) {
			warning_shown = 1;
			warning(_("lazy fetching disabled; some objects may not be available"));
		}
		return -1;
	}

	child.git_cmd = 1;
	child.in = -1;
	if (repo != the_repository)
		prepare_other_repo_env(&child.env, repo->gitdir);
	strvec_pushl(&child.args, "-c", "fetch.negotiationAlgorithm=noop",
		     "fetch", remote_name, "--no-tags",
		     "--no-write-fetch-head", "--recurse-submodules=no",
		     "--filter=blob:none", "--stdin", NULL);
	if (start_command(&child))
		die(_("promisor-remote: unable to fork off fetch subprocess"));
	child_in = xfdopen(child.in, "w");

	trace2_data_intmax("promisor", repo, "fetch_count", oid_nr);

	for (i = 0; i < oid_nr; i++) {
		if (fputs(oid_to_hex(&oids[i]), child_in) < 0)
			die_errno(_("promisor-remote: could not write to fetch subprocess"));
		if (fputc('\n', child_in) < 0)
			die_errno(_("promisor-remote: could not write to fetch subprocess"));
	}

	if (fclose(child_in) < 0)
		die_errno(_("promisor-remote: could not close stdin to fetch subprocess"));
	return finish_command(&child) ? -1 : 0;
}