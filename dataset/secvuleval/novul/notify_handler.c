static void notify_handler(acpi_handle handle, u32 event, void *context)
{
	struct platform_device *device = context;
	struct intel_vbtn_priv *priv = dev_get_drvdata(&device->dev);
	unsigned int val = !(event & 1); /* Even=press, Odd=release */
	const struct key_entry *ke, *ke_rel;
	struct input_dev *input_dev;
	bool autorelease;
	int ret;

	guard(mutex)(&priv->mutex);

	if ((ke = sparse_keymap_entry_from_scancode(priv->buttons_dev, event))) {
		if (!priv->has_buttons) {
			dev_warn(&device->dev, "Warning: received 0x%02x button event on a device without buttons, please report this.\n",
				 event);
			return;
		}
		input_dev = priv->buttons_dev;
	} else if ((ke = sparse_keymap_entry_from_scancode(priv->switches_dev, event))) {
		if (!priv->has_switches) {
			/* See dual_accel_detect.h for more info */
			if (priv->dual_accel)
				return;

			dev_info(&device->dev, "Registering Intel Virtual Switches input-dev after receiving a switch event\n");
			ret = input_register_device(priv->switches_dev);
			if (ret)
				return;

			priv->has_switches = true;
		}
		input_dev = priv->switches_dev;
	} else {
		dev_dbg(&device->dev, "unknown event index 0x%x\n", event);
		return;
	}

	if (priv->wakeup_mode) {
		pm_wakeup_hard_event(&device->dev);

		/*
		 * Skip reporting an evdev event for button wake events,
		 * mirroring how the drivers/acpi/button.c code skips this too.
		 */
		if (ke->type == KE_KEY)
			return;
	}

	/*
	 * Even press events are autorelease if there is no corresponding odd
	 * release event, or if the odd event is KE_IGNORE.
	 */
	ke_rel = sparse_keymap_entry_from_scancode(input_dev, event | 1);
	autorelease = val && (!ke_rel || ke_rel->type == KE_IGNORE);

	sparse_keymap_report_event(input_dev, event, val, autorelease);
}
