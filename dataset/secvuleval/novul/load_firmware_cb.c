static void load_firmware_cb(const struct firmware *fw,
			     void *context)
{
	struct dvb_frontend *fe = context;
	struct xc2028_data *priv;
	int rc;

	if (!fe) {
		pr_warn("xc2028: No frontend in %s\n", __func__);
		return;
	}

	priv = fe->tuner_priv;

	tuner_dbg("request_firmware_nowait(): %s\n", fw ? "OK" : "error");
	if (!fw) {
		tuner_err("Could not load firmware %s.\n", priv->fname);
		priv->state = XC2028_NODEV;
		return;
	}

	rc = load_all_firmwares(fe, fw);

	release_firmware(fw);

	if (rc < 0)
		return;
	priv->state = XC2028_ACTIVE;
}
