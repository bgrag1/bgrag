static int max310x_i2c_probe(struct i2c_client *client)
{
	const struct max310x_devtype *devtype =
			device_get_match_data(&client->dev);
	struct i2c_client *port_client;
	struct regmap *regmaps[4];
	unsigned int i;
	u8 port_addr;

	if (client->addr < devtype->slave_addr.min ||
		client->addr > devtype->slave_addr.max)
		return dev_err_probe(&client->dev, -EINVAL,
				     "Slave addr 0x%x outside of range [0x%x, 0x%x]\n",
				     client->addr, devtype->slave_addr.min,
				     devtype->slave_addr.max);

	regmaps[0] = devm_regmap_init_i2c(client, &regcfg_i2c);

	for (i = 1; i < devtype->nr; i++) {
		port_addr = max310x_i2c_slave_addr(client->addr, i);
		port_client = devm_i2c_new_dummy_device(&client->dev,
							client->adapter,
							port_addr);

		regmaps[i] = devm_regmap_init_i2c(port_client, &regcfg_i2c);
	}

	return max310x_probe(&client->dev, devtype, &max310x_i2c_if_cfg,
			     regmaps, client->irq);
}
