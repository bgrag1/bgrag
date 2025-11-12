int pmic_glink_send(struct pmic_glink_client *client, void *data, size_t len)
{
	struct pmic_glink *pg = client->pg;
	int ret;

	mutex_lock(&pg->state_lock);
	if (!pg->ept)
		ret = -ECONNRESET;
	else
		ret = rpmsg_send(pg->ept, data, len);
	mutex_unlock(&pg->state_lock);

	return ret;
}
