void ocfs2_journal_dirty(handle_t *handle, struct buffer_head *bh)
{
	int status;

	trace_ocfs2_journal_dirty((unsigned long long)bh->b_blocknr);

	status = jbd2_journal_dirty_metadata(handle, bh);
	if (status) {
		mlog_errno(status);
		if (!is_handle_aborted(handle)) {
			journal_t *journal = handle->h_transaction->t_journal;

			mlog(ML_ERROR, "jbd2_journal_dirty_metadata failed: "
			     "handle type %u started at line %u, credits %u/%u "
			     "errcode %d. Aborting transaction and journal.\n",
			     handle->h_type, handle->h_line_no,
			     handle->h_requested_credits,
			     jbd2_handle_buffer_credits(handle), status);
			handle->h_err = status;
			jbd2_journal_abort_handle(handle);
			jbd2_journal_abort(journal, status);
		}
	}
}
