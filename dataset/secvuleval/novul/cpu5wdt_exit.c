static void cpu5wdt_exit(void)
{
	if (cpu5wdt_device.queue) {
		cpu5wdt_device.queue = 0;
		wait_for_completion(&cpu5wdt_device.stop);
		timer_shutdown_sync(&cpu5wdt_device.timer);
	}

	misc_deregister(&cpu5wdt_misc);

	release_region(port, CPU5WDT_EXTENT);

}
