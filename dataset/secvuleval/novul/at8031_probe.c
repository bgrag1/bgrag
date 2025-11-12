static int at8031_probe(struct phy_device *phydev)
{
	struct at803x_priv *priv;
	int mode_cfg;
	int ccr;
	int ret;

	ret = at803x_probe(phydev);
	if (ret)
		return ret;

	priv = phydev->priv;

	/* Only supported on AR8031/AR8033, the AR8030/AR8035 use strapping
	 * options.
	 */
	ret = at8031_parse_dt(phydev);
	if (ret)
		return ret;

	ccr = phy_read(phydev, AT803X_REG_CHIP_CONFIG);
	if (ccr < 0)
		return ccr;
	mode_cfg = ccr & AT803X_MODE_CFG_MASK;

	switch (mode_cfg) {
	case AT803X_MODE_CFG_BX1000_RGMII_50OHM:
	case AT803X_MODE_CFG_BX1000_RGMII_75OHM:
		priv->is_1000basex = true;
		fallthrough;
	case AT803X_MODE_CFG_FX100_RGMII_50OHM:
	case AT803X_MODE_CFG_FX100_RGMII_75OHM:
		priv->is_fiber = true;
		break;
	}

	/* Disable WoL in 1588 register which is enabled
	 * by default
	 */
	return phy_modify_mmd(phydev, MDIO_MMD_PCS,
			      AT803X_PHY_MMD3_WOL_CTRL,
			      AT803X_WOL_EN, 0);
}
