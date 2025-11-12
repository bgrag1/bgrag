static int qxl_add_mode(struct drm_connector *connector,
			unsigned int width,
			unsigned int height,
			bool preferred)
{
	struct drm_device *dev = connector->dev;
	struct qxl_device *qdev = to_qxl(dev);
	struct drm_display_mode *mode = NULL;
	int rc;

	rc = qxl_check_mode(qdev, width, height);
	if (rc != 0)
		return 0;

	mode = drm_cvt_mode(dev, width, height, 60, false, false, false);
	if (preferred)
		mode->type |= DRM_MODE_TYPE_PREFERRED;
	mode->hdisplay = width;
	mode->vdisplay = height;
	drm_mode_set_name(mode);
	drm_mode_probed_add(connector, mode);
	return 1;
}
