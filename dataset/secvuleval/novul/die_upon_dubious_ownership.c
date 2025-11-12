void die_upon_dubious_ownership(const char *gitfile, const char *worktree,
				const char *gitdir)
{
	struct strbuf report = STRBUF_INIT, quoted = STRBUF_INIT;
	const char *path;

	if (ensure_valid_ownership(gitfile, worktree, gitdir, &report))
		return;

	strbuf_complete(&report, '\n');
	path = gitfile ? gitfile : gitdir;
	sq_quote_buf_pretty(&quoted, path);

	die(_("detected dubious ownership in repository at '%s'\n"
	      "%s"
	      "To add an exception for this directory, call:\n"
	      "\n"
	      "\tgit config --global --add safe.directory %s"),
	    path, report.buf, quoted.buf);
}