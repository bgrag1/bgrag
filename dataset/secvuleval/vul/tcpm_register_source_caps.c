static int tcpm_register_source_caps(struct tcpm_port *port)
{
	struct usb_power_delivery_desc desc = { port->negotiated_rev };
	struct usb_power_delivery_capabilities_desc caps = { };
	struct usb_power_delivery_capabilities *cap = port->partner_source_caps;

	if (!port->partner_pd)
		port->partner_pd = usb_power_delivery_register(NULL, &desc);
	if (IS_ERR(port->partner_pd))
		return PTR_ERR(port->partner_pd);

	memcpy(caps.pdo, port->source_caps, sizeof(u32) * port->nr_source_caps);
	caps.role = TYPEC_SOURCE;

	if (cap)
		usb_power_delivery_unregister_capabilities(cap);

	cap = usb_power_delivery_register_capabilities(port->partner_pd, &caps);
	if (IS_ERR(cap))
		return PTR_ERR(cap);

	port->partner_source_caps = cap;

	return 0;
}
