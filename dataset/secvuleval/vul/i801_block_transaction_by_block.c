static int i801_block_transaction_by_block(struct i801_priv *priv,
					   union i2c_smbus_data *data,
					   char read_write, int command)
{
	int i, len, status, xact;

	switch (command) {
	case I2C_SMBUS_BLOCK_PROC_CALL:
		xact = I801_BLOCK_PROC_CALL;
		break;
	case I2C_SMBUS_BLOCK_DATA:
		xact = I801_BLOCK_DATA;
		break;
	default:
		return -EOPNOTSUPP;
	}

	/* Set block buffer mode */
	outb_p(inb_p(SMBAUXCTL(priv)) | SMBAUXCTL_E32B, SMBAUXCTL(priv));

	inb_p(SMBHSTCNT(priv)); /* reset the data buffer index */

	if (read_write == I2C_SMBUS_WRITE) {
		len = data->block[0];
		outb_p(len, SMBHSTDAT0(priv));
		for (i = 0; i < len; i++)
			outb_p(data->block[i+1], SMBBLKDAT(priv));
	}

	status = i801_transaction(priv, xact);
	if (status)
		goto out;

	if (read_write == I2C_SMBUS_READ ||
	    command == I2C_SMBUS_BLOCK_PROC_CALL) {
		len = inb_p(SMBHSTDAT0(priv));
		if (len < 1 || len > I2C_SMBUS_BLOCK_MAX) {
			status = -EPROTO;
			goto out;
		}

		data->block[0] = len;
		for (i = 0; i < len; i++)
			data->block[i + 1] = inb_p(SMBBLKDAT(priv));
	}
out:
	outb_p(inb_p(SMBAUXCTL(priv)) & ~SMBAUXCTL_E32B, SMBAUXCTL(priv));
	return status;
}
