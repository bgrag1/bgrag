void ocfs2_journal_dirty(handle_t *handle, struct buffer_head *bh)
{
	int status;

	trace_ocfs2_journal_dirty((unsigned long long)bh->b_blocknr);

	status = jbd2_journal_dirty_metadata(handle, bh);
	if (status) {
		mlog_errno(status);
		if (!is_handle_aborted(handle)) {
			journal_t *journal = handle->h_transaction->t_journal;

			mlog(ML_ERROR, "jbd2_journal_dirty_metadata failed. "
					"Aborting transaction and journal.\n");
			handle->h_err = status;
			jbd2_journal_abort_handle(handle);
			jbd2_journal_abort(journal, status);
			ocfs2_abort(bh->b_assoc_map->host->i_sb,
				    "Journal already aborted.\n");
		}
	}
}
