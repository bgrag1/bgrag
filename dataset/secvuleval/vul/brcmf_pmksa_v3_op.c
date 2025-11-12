static s32
brcmf_pmksa_v3_op(struct brcmf_if *ifp, struct cfg80211_pmksa *pmksa,
		  bool alive)
{
	struct brcmf_pmk_op_v3_le *pmk_op;
	int length = offsetof(struct brcmf_pmk_op_v3_le, pmk);
	int ret;

	pmk_op = kzalloc(sizeof(*pmk_op), GFP_KERNEL);
	if (!pmk_op)
		return -ENOMEM;

	pmk_op->version = cpu_to_le16(BRCMF_PMKSA_VER_3);

	if (!pmksa) {
		/* Flush operation, operate on entire list */
		pmk_op->count = cpu_to_le16(0);
	} else {
		/* Single PMK operation */
		pmk_op->count = cpu_to_le16(1);
		length += sizeof(struct brcmf_pmksa_v3);
		memcpy(pmk_op->pmk[0].bssid, pmksa->bssid, ETH_ALEN);
		memcpy(pmk_op->pmk[0].pmkid, pmksa->pmkid, WLAN_PMKID_LEN);
		pmk_op->pmk[0].pmkid_len = WLAN_PMKID_LEN;
		pmk_op->pmk[0].time_left = cpu_to_le32(alive ? BRCMF_PMKSA_NO_EXPIRY : 0);
	}

	pmk_op->length = cpu_to_le16(length);

	ret = brcmf_fil_iovar_data_set(ifp, "pmkid_info", pmk_op, sizeof(*pmk_op));
	kfree(pmk_op);
	return ret;
}
