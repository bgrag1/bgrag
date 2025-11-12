static void tasdev_load_calibrated_data(struct tasdevice_priv *priv, int i)
{
	struct tasdevice_calibration *cal;
	struct tasdevice_fw *cal_fmw;

	cal_fmw = priv->tasdevice[i].cali_data_fmw;

	/* No calibrated data for current devices, playback will go ahead. */
	if (!cal_fmw)
		return;

	cal = cal_fmw->calibrations;
	if (!cal)
		return;

	load_calib_data(priv, &cal->dev_data);
}
