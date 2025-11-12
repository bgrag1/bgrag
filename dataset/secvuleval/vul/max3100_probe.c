static int max3100_probe(struct spi_device *spi)
{
	int i, retval;
	struct plat_max3100 *pdata;
	u16 tx, rx;

	mutex_lock(&max3100s_lock);

	if (!uart_driver_registered) {
		uart_driver_registered = 1;
		retval = uart_register_driver(&max3100_uart_driver);
		if (retval) {
			printk(KERN_ERR "Couldn't register max3100 uart driver\n");
			mutex_unlock(&max3100s_lock);
			return retval;
		}
	}

	for (i = 0; i < MAX_MAX3100; i++)
		if (!max3100s[i])
			break;
	if (i == MAX_MAX3100) {
		dev_warn(&spi->dev, "too many MAX3100 chips\n");
		mutex_unlock(&max3100s_lock);
		return -ENOMEM;
	}

	max3100s[i] = kzalloc(sizeof(struct max3100_port), GFP_KERNEL);
	if (!max3100s[i]) {
		dev_warn(&spi->dev,
			 "kmalloc for max3100 structure %d failed!\n", i);
		mutex_unlock(&max3100s_lock);
		return -ENOMEM;
	}
	max3100s[i]->spi = spi;
	max3100s[i]->irq = spi->irq;
	spin_lock_init(&max3100s[i]->conf_lock);
	spi_set_drvdata(spi, max3100s[i]);
	pdata = dev_get_platdata(&spi->dev);
	max3100s[i]->crystal = pdata->crystal;
	max3100s[i]->loopback = pdata->loopback;
	max3100s[i]->poll_time = msecs_to_jiffies(pdata->poll_time);
	if (pdata->poll_time > 0 && max3100s[i]->poll_time == 0)
		max3100s[i]->poll_time = 1;
	max3100s[i]->max3100_hw_suspend = pdata->max3100_hw_suspend;
	max3100s[i]->minor = i;
	timer_setup(&max3100s[i]->timer, max3100_timeout, 0);

	dev_dbg(&spi->dev, "%s: adding port %d\n", __func__, i);
	max3100s[i]->port.irq = max3100s[i]->irq;
	max3100s[i]->port.uartclk = max3100s[i]->crystal ? 3686400 : 1843200;
	max3100s[i]->port.fifosize = 16;
	max3100s[i]->port.ops = &max3100_ops;
	max3100s[i]->port.flags = UPF_SKIP_TEST | UPF_BOOT_AUTOCONF;
	max3100s[i]->port.line = i;
	max3100s[i]->port.type = PORT_MAX3100;
	max3100s[i]->port.dev = &spi->dev;
	retval = uart_add_one_port(&max3100_uart_driver, &max3100s[i]->port);
	if (retval < 0)
		dev_warn(&spi->dev,
			 "uart_add_one_port failed for line %d with error %d\n",
			 i, retval);

	/* set shutdown mode to save power. Will be woken-up on open */
	if (max3100s[i]->max3100_hw_suspend)
		max3100s[i]->max3100_hw_suspend(1);
	else {
		tx = MAX3100_WC | MAX3100_SHDN;
		max3100_sr(max3100s[i], tx, &rx);
	}
	mutex_unlock(&max3100s_lock);
	return 0;
}
