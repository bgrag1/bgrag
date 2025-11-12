void compute_intercept_slope(struct tsens_priv *priv, u32 *p1,
			     u32 *p2, u32 mode)
{
	int i;
	int num, den;

	for (i = 0; i < priv->num_sensors; i++) {
		dev_dbg(priv->dev,
			"%s: sensor%d - data_point1:%#x data_point2:%#x\n",
			__func__, i, p1[i], p2 ? p2[i] : 0);

		if (!priv->sensor[i].slope)
			priv->sensor[i].slope = SLOPE_DEFAULT;
		if (mode == TWO_PT_CALIB || mode == TWO_PT_CALIB_NO_OFFSET) {
			/*
			 * slope (m) = adc_code2 - adc_code1 (y2 - y1)/
			 *	temp_120_degc - temp_30_degc (x2 - x1)
			 */
			num = p2[i] - p1[i];
			num *= SLOPE_FACTOR;
			den = CAL_DEGC_PT2 - CAL_DEGC_PT1;
			priv->sensor[i].slope = num / den;
		}

		priv->sensor[i].offset = (p1[i] * SLOPE_FACTOR) -
				(CAL_DEGC_PT1 *
				priv->sensor[i].slope);
		dev_dbg(priv->dev, "%s: offset:%d\n", __func__,
			priv->sensor[i].offset);
	}
}
