static int nv17_tv_get_hd_modes(struct drm_encoder *encoder,
				struct drm_connector *connector)
{
	struct nv17_tv_norm_params *tv_norm = get_tv_norm(encoder);
	struct drm_display_mode *output_mode = &tv_norm->ctv_enc_mode.mode;
	struct drm_display_mode *mode;
	const struct {
		int hdisplay;
		int vdisplay;
	} modes[] = {
		{ 640, 400 },
		{ 640, 480 },
		{ 720, 480 },
		{ 720, 576 },
		{ 800, 600 },
		{ 1024, 768 },
		{ 1280, 720 },
		{ 1280, 1024 },
		{ 1920, 1080 }
	};
	int i, n = 0;

	for (i = 0; i < ARRAY_SIZE(modes); i++) {
		if (modes[i].hdisplay > output_mode->hdisplay ||
		    modes[i].vdisplay > output_mode->vdisplay)
			continue;

		if (modes[i].hdisplay == output_mode->hdisplay &&
		    modes[i].vdisplay == output_mode->vdisplay) {
			mode = drm_mode_duplicate(encoder->dev, output_mode);
			mode->type |= DRM_MODE_TYPE_PREFERRED;

		} else {
			mode = drm_cvt_mode(encoder->dev, modes[i].hdisplay,
					    modes[i].vdisplay, 60, false,
					    (output_mode->flags &
					     DRM_MODE_FLAG_INTERLACE), false);
		}

		/* CVT modes are sometimes unsuitable... */
		if (output_mode->hdisplay <= 720
		    || output_mode->hdisplay >= 1920) {
			mode->htotal = output_mode->htotal;
			mode->hsync_start = (mode->hdisplay + (mode->htotal
					     - mode->hdisplay) * 9 / 10) & ~7;
			mode->hsync_end = mode->hsync_start + 8;
		}

		if (output_mode->vdisplay >= 1024) {
			mode->vtotal = output_mode->vtotal;
			mode->vsync_start = output_mode->vsync_start;
			mode->vsync_end = output_mode->vsync_end;
		}

		mode->type |= DRM_MODE_TYPE_DRIVER;
		drm_mode_probed_add(connector, mode);
		n++;
	}

	return n;
}
