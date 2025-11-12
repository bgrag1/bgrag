static int
nouveau_connector_get_modes(struct drm_connector *connector)
{
	struct drm_device *dev = connector->dev;
	struct nouveau_drm *drm = nouveau_drm(dev);
	struct nouveau_connector *nv_connector = nouveau_connector(connector);
	struct nouveau_encoder *nv_encoder = nv_connector->detected_encoder;
	struct drm_encoder *encoder = to_drm_encoder(nv_encoder);
	int ret = 0;

	/* destroy the native mode, the attached monitor could have changed.
	 */
	if (nv_connector->native_mode) {
		drm_mode_destroy(dev, nv_connector->native_mode);
		nv_connector->native_mode = NULL;
	}

	if (nv_connector->edid)
		ret = drm_add_edid_modes(connector, nv_connector->edid);
	else
	if (nv_encoder->dcb->type == DCB_OUTPUT_LVDS &&
	    (nv_encoder->dcb->lvdsconf.use_straps_for_mode ||
	     drm->vbios.fp_no_ddc) && nouveau_bios_fp_mode(dev, NULL)) {
		struct drm_display_mode mode;

		nouveau_bios_fp_mode(dev, &mode);
		nv_connector->native_mode = drm_mode_duplicate(dev, &mode);
	}

	/* Determine display colour depth for everything except LVDS now,
	 * DP requires this before mode_valid() is called.
	 */
	if (connector->connector_type != DRM_MODE_CONNECTOR_LVDS)
		nouveau_connector_detect_depth(connector);

	/* Find the native mode if this is a digital panel, if we didn't
	 * find any modes through DDC previously add the native mode to
	 * the list of modes.
	 */
	if (!nv_connector->native_mode)
		nv_connector->native_mode = nouveau_conn_native_mode(connector);
	if (ret == 0 && nv_connector->native_mode) {
		struct drm_display_mode *mode;

		mode = drm_mode_duplicate(dev, nv_connector->native_mode);
		drm_mode_probed_add(connector, mode);
		ret = 1;
	}

	/* Determine LVDS colour depth, must happen after determining
	 * "native" mode as some VBIOS tables require us to use the
	 * pixel clock as part of the lookup...
	 */
	if (connector->connector_type == DRM_MODE_CONNECTOR_LVDS && nv_connector->native_mode)
		nouveau_connector_detect_depth(connector);

	if (nv_encoder->dcb->type == DCB_OUTPUT_TV)
		ret = get_slave_funcs(encoder)->get_modes(encoder, connector);

	if (nv_connector->type == DCB_CONNECTOR_LVDS ||
	    nv_connector->type == DCB_CONNECTOR_LVDS_SPWG ||
	    nv_connector->type == DCB_CONNECTOR_eDP)
		ret += nouveau_connector_scaler_modes_add(connector);

	return ret;
}
