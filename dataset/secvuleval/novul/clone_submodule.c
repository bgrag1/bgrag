static int clone_submodule(const struct module_clone_data *clone_data,
			   struct string_list *reference)
{
	char *p;
	char *sm_gitdir = clone_submodule_sm_gitdir(clone_data->name);
	char *sm_alternate = NULL, *error_strategy = NULL;
	struct stat st;
	struct child_process cp = CHILD_PROCESS_INIT;
	const char *clone_data_path = clone_data->path;
	char *to_free = NULL;

	if (!is_absolute_path(clone_data->path))
		clone_data_path = to_free = xstrfmt("%s/%s", get_git_work_tree(),
						    clone_data->path);

	if (validate_submodule_git_dir(sm_gitdir, clone_data->name) < 0)
		die(_("refusing to create/use '%s' in another submodule's "
		      "git dir"), sm_gitdir);

	if (!file_exists(sm_gitdir)) {
		if (clone_data->require_init && !stat(clone_data_path, &st) &&
		    !is_empty_dir(clone_data_path))
			die(_("directory not empty: '%s'"), clone_data_path);

		if (safe_create_leading_directories_const(sm_gitdir) < 0)
			die(_("could not create directory '%s'"), sm_gitdir);

		prepare_possible_alternates(clone_data->name, reference);

		strvec_push(&cp.args, "clone");
		strvec_push(&cp.args, "--no-checkout");
		if (clone_data->quiet)
			strvec_push(&cp.args, "--quiet");
		if (clone_data->progress)
			strvec_push(&cp.args, "--progress");
		if (clone_data->depth && *(clone_data->depth))
			strvec_pushl(&cp.args, "--depth", clone_data->depth, NULL);
		if (reference->nr) {
			struct string_list_item *item;

			for_each_string_list_item(item, reference)
				strvec_pushl(&cp.args, "--reference",
					     item->string, NULL);
		}
		if (clone_data->dissociate)
			strvec_push(&cp.args, "--dissociate");
		if (sm_gitdir && *sm_gitdir)
			strvec_pushl(&cp.args, "--separate-git-dir", sm_gitdir, NULL);
		if (clone_data->filter_options && clone_data->filter_options->choice)
			strvec_pushf(&cp.args, "--filter=%s",
				     expand_list_objects_filter_spec(
					     clone_data->filter_options));
		if (clone_data->single_branch >= 0)
			strvec_push(&cp.args, clone_data->single_branch ?
				    "--single-branch" :
				    "--no-single-branch");

		strvec_push(&cp.args, "--");
		strvec_push(&cp.args, clone_data->url);
		strvec_push(&cp.args, clone_data_path);

		cp.git_cmd = 1;
		prepare_submodule_repo_env(&cp.env);
		cp.no_stdin = 1;

		if(run_command(&cp))
			die(_("clone of '%s' into submodule path '%s' failed"),
			    clone_data->url, clone_data_path);

		if (clone_data->require_init && !stat(clone_data_path, &st) &&
		    !dir_contains_only_dotgit(clone_data_path)) {
			char *dot_git = xstrfmt("%s/.git", clone_data_path);
			unlink(dot_git);
			free(dot_git);
			die(_("directory not empty: '%s'"), clone_data_path);
		}
	} else {
		char *path;

		if (clone_data->require_init && !access(clone_data_path, X_OK) &&
		    !is_empty_dir(clone_data_path))
			die(_("directory not empty: '%s'"), clone_data_path);
		if (safe_create_leading_directories_const(clone_data_path) < 0)
			die(_("could not create directory '%s'"), clone_data_path);
		path = xstrfmt("%s/index", sm_gitdir);
		unlink_or_warn(path);
		free(path);
	}

	/*
	 * We already performed this check at the beginning of this function,
	 * before cloning the objects. This tries to detect racy behavior e.g.
	 * in parallel clones, where another process could easily have made the
	 * gitdir nested _after_ it was created.
	 *
	 * To prevent further harm coming from this unintentionally-nested
	 * gitdir, let's disable it by deleting the `HEAD` file.
	 */
	if (validate_submodule_git_dir(sm_gitdir, clone_data->name) < 0) {
		char *head = xstrfmt("%s/HEAD", sm_gitdir);
		unlink(head);
		free(head);
		die(_("refusing to create/use '%s' in another submodule's "
		      "git dir"), sm_gitdir);
	}

	connect_work_tree_and_git_dir(clone_data_path, sm_gitdir, 0);

	p = git_pathdup_submodule(clone_data_path, "config");
	if (!p)
		die(_("could not get submodule directory for '%s'"), clone_data_path);

	/* setup alternateLocation and alternateErrorStrategy in the cloned submodule if needed */
	git_config_get_string("submodule.alternateLocation", &sm_alternate);
	if (sm_alternate)
		git_config_set_in_file(p, "submodule.alternateLocation",
				       sm_alternate);
	git_config_get_string("submodule.alternateErrorStrategy", &error_strategy);
	if (error_strategy)
		git_config_set_in_file(p, "submodule.alternateErrorStrategy",
				       error_strategy);

	free(sm_alternate);
	free(error_strategy);

	free(sm_gitdir);
	free(p);
	free(to_free);
	return 0;
}