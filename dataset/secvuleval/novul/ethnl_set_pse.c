static int
ethnl_set_pse(struct ethnl_req_info *req_info, struct genl_info *info)
{
	struct net_device *dev = req_info->dev;
	struct pse_control_config config = {};
	struct nlattr **tb = info->attrs;
	struct phy_device *phydev;

	phydev = dev->phydev;
	/* These values are already validated by the ethnl_pse_set_policy */
	if (tb[ETHTOOL_A_PODL_PSE_ADMIN_CONTROL])
		config.podl_admin_control = nla_get_u32(tb[ETHTOOL_A_PODL_PSE_ADMIN_CONTROL]);
	if (tb[ETHTOOL_A_C33_PSE_ADMIN_CONTROL])
		config.c33_admin_control = nla_get_u32(tb[ETHTOOL_A_C33_PSE_ADMIN_CONTROL]);

	/* Return errno directly - PSE has no notification
	 * pse_ethtool_set_config() will do nothing if the config is null
	 */
	return pse_ethtool_set_config(phydev->psec, info->extack, &config);
}
