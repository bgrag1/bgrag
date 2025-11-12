static int psb_intel_lvds_get_modes(struct drm_connector *connector)
{
	struct drm_device *dev = connector->dev;
	struct drm_psb_private *dev_priv = to_drm_psb_private(dev);
	struct psb_intel_mode_device *mode_dev = &dev_priv->mode_dev;
	int ret = 0;

	if (!IS_MRST(dev))
		ret = psb_intel_ddc_get_modes(connector, connector->ddc);

	if (ret)
		return ret;

	if (mode_dev->panel_fixed_mode != NULL) {
		struct drm_display_mode *mode =
		    drm_mode_duplicate(dev, mode_dev->panel_fixed_mode);
		drm_mode_probed_add(connector, mode);
		return 1;
	}

	return 0;
}
