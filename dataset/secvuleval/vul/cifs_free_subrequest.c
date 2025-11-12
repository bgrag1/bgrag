static void cifs_free_subrequest(struct netfs_io_subrequest *subreq)
{
	struct cifs_io_subrequest *rdata =
		container_of(subreq, struct cifs_io_subrequest, subreq);
	int rc = subreq->error;

	if (rdata->subreq.source == NETFS_DOWNLOAD_FROM_SERVER) {
// #ifdef CONFIG_CIFS_SMB_DIRECT
		if (rdata->mr) {
			smbd_deregister_mr(rdata->mr);
			rdata->mr = NULL;
		}
#endif
	}

	if (rdata->credits.value != 0)
		trace_smb3_rw_credits(rdata->rreq->debug_id,
				      rdata->subreq.debug_index,
				      rdata->credits.value,
				      rdata->server ? rdata->server->credits : 0,
				      rdata->server ? rdata->server->in_flight : 0,
				      -rdata->credits.value,
				      cifs_trace_rw_credits_free_subreq);

	add_credits_and_wake_if(rdata->server, &rdata->credits, 0);
	if (rdata->have_xid)
		free_xid(rdata->xid);
}
