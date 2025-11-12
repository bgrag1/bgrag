static int nv17_tv_get_ld_modes(struct drm_encoder *encoder,
				struct drm_connector *connector)
{
	struct nv17_tv_norm_params *tv_norm = get_tv_norm(encoder);
	const struct drm_display_mode *tv_mode;
	int n = 0;

	for (tv_mode = nv17_tv_modes; tv_mode->hdisplay; tv_mode++) {
		struct drm_display_mode *mode;

		mode = drm_mode_duplicate(encoder->dev, tv_mode);
		if (!mode)
			continue;

		mode->clock = tv_norm->tv_enc_mode.vrefresh *
			mode->htotal / 1000 *
			mode->vtotal / 1000;

		if (mode->flags & DRM_MODE_FLAG_DBLSCAN)
			mode->clock *= 2;

		if (mode->hdisplay == tv_norm->tv_enc_mode.hdisplay &&
		    mode->vdisplay == tv_norm->tv_enc_mode.vdisplay)
			mode->type |= DRM_MODE_TYPE_PREFERRED;

		drm_mode_probed_add(connector, mode);
		n++;
	}

	return n;
}
