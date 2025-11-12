void amdtp_hid_remove(struct amdtp_cl_data *cli_data)
{
	int i;
	struct amdtp_hid_data *hid_data;

	for (i = 0; i < cli_data->num_hid_devices; ++i) {
		if (cli_data->hid_sensor_hubs[i]) {
			hid_data = cli_data->hid_sensor_hubs[i]->driver_data;
			hid_destroy_device(cli_data->hid_sensor_hubs[i]);
			kfree(hid_data);
			cli_data->hid_sensor_hubs[i] = NULL;
		}
	}
}
