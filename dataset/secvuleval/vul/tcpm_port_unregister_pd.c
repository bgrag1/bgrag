static void tcpm_port_unregister_pd(struct tcpm_port *port)
{
	int i;

	port->port_sink_caps = NULL;
	port->port_source_caps = NULL;
	for (i = 0; i < port->pd_count; i++) {
		usb_power_delivery_unregister_capabilities(port->pd_list[i]->sink_cap);
		kfree(port->pd_list[i]->sink_cap);
		usb_power_delivery_unregister_capabilities(port->pd_list[i]->source_cap);
		kfree(port->pd_list[i]->source_cap);
		devm_kfree(port->dev, port->pd_list[i]);
		port->pd_list[i] = NULL;
		usb_power_delivery_unregister(port->pds[i]);
		port->pds[i] = NULL;
	}
}
