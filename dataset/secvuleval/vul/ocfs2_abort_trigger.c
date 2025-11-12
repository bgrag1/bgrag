static void ocfs2_abort_trigger(struct jbd2_buffer_trigger_type *triggers,
				struct buffer_head *bh)
{
	mlog(ML_ERROR,
	     "ocfs2_abort_trigger called by JBD2.  bh = 0x%lx, "
	     "bh->b_blocknr = %llu\n",
	     (unsigned long)bh,
	     (unsigned long long)bh->b_blocknr);

	ocfs2_error(bh->b_assoc_map->host->i_sb,
		    "JBD2 has aborted our journal, ocfs2 cannot continue\n");
}
