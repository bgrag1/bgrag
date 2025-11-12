static int scmi_optee_chan_free(int id, void *p, void *data)
{
	struct scmi_chan_info *cinfo = p;
	struct scmi_optee_channel *channel = cinfo->transport_info;

	mutex_lock(&scmi_optee_private->mu);
	list_del(&channel->link);
	mutex_unlock(&scmi_optee_private->mu);

	close_session(scmi_optee_private, channel->tee_session);

	if (channel->tee_shm) {
		tee_shm_free(channel->tee_shm);
		channel->tee_shm = NULL;
	}

	cinfo->transport_info = NULL;
	channel->cinfo = NULL;

	return 0;
}
