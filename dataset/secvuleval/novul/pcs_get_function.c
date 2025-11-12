static int pcs_get_function(struct pinctrl_dev *pctldev, unsigned pin,
			    struct pcs_function **func)
{
	struct pcs_device *pcs = pinctrl_dev_get_drvdata(pctldev);
	struct pin_desc *pdesc = pin_desc_get(pctldev, pin);
	const struct pinctrl_setting_mux *setting;
	struct function_desc *function;
	unsigned fselector;

	/* If pin is not described in DTS & enabled, mux_setting is NULL. */
	setting = pdesc->mux_setting;
	if (!setting)
		return -ENOTSUPP;
	fselector = setting->func;
	function = pinmux_generic_get_function(pctldev, fselector);
	if (!function)
		return -EINVAL;
	*func = function->data;
	if (!(*func)) {
		dev_err(pcs->dev, "%s could not find function%i\n",
			__func__, fselector);
		return -ENOTSUPP;
	}
	return 0;
}
