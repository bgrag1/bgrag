int pmic_glink_send(struct pmic_glink_client *client, void *data, size_t len)
{
	struct pmic_glink *pg = client->pg;

	return rpmsg_send(pg->ept, data, len);
}
