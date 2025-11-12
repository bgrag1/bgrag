int usb_ep_enable(struct usb_ep *ep)
{
	int ret = 0;

	if (ep->enabled)
		goto out;

	/* UDC drivers can't handle endpoints with maxpacket size 0 */
	if (usb_endpoint_maxp(ep->desc) == 0) {
		/*
		 * We should log an error message here, but we can't call
		 * dev_err() because there's no way to find the gadget
		 * given only ep.
		 */
		ret = -EINVAL;
		goto out;
	}

	ret = ep->ops->enable(ep, ep->desc);
	if (ret)
		goto out;

	ep->enabled = true;

out:
	trace_usb_ep_enable(ep, ret);

	return ret;
}
