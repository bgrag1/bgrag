static void remove_device_compression_modes(struct iaa_device *iaa_device)
{
	struct iaa_device_compression_mode *device_mode;
	int i;

	for (i = 0; i < IAA_COMP_MODES_MAX; i++) {
		device_mode = iaa_device->compression_modes[i];
		if (!device_mode)
			continue;

		free_device_compression_mode(iaa_device, device_mode);
		iaa_device->compression_modes[i] = NULL;
		if (iaa_compression_modes[i]->free)
			iaa_compression_modes[i]->free(device_mode);
	}
}
