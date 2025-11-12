static const struct sof_ipc4_audio_format *
sof_ipc4_get_input_pin_audio_fmt(struct snd_sof_widget *swidget, int pin_index)
{
	struct sof_ipc4_base_module_cfg_ext *base_cfg_ext;
	struct sof_ipc4_process *process;
	int i;

	if (swidget->id != snd_soc_dapm_effect) {
		struct sof_ipc4_base_module_cfg *base = swidget->private;

		/* For non-process modules, base module config format is used for all input pins */
		return &base->audio_fmt;
	}

	process = swidget->private;

	/*
	 * For process modules without base config extension, base module config
	 * format is used for all input pins
	 */
	if (process->init_config != SOF_IPC4_MODULE_INIT_CONFIG_TYPE_BASE_CFG_WITH_EXT)
		return &process->base_config.audio_fmt;

	base_cfg_ext = process->base_config_ext;

	/*
	 * If there are multiple input formats available for a pin, the first available format
	 * is chosen.
	 */
	for (i = 0; i < base_cfg_ext->num_input_pin_fmts; i++) {
		struct sof_ipc4_pin_format *pin_format = &base_cfg_ext->pin_formats[i];

		if (pin_format->pin_index == pin_index)
			return &pin_format->audio_fmt;
	}

	return NULL;
}
