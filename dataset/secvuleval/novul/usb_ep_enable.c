int usb_ep_enable(struct usb_ep *ep)
{
	int ret = 0;

	if (ep->enabled)
		goto out;

	/* UDC drivers can't handle endpoints with maxpacket size 0 */
	if (!ep->desc || usb_endpoint_maxp(ep->desc) == 0) {
		WARN_ONCE(1, "%s: ep%d (%s) has %s\n", __func__, ep->address, ep->name,
			  (!ep->desc) ? "NULL descriptor" : "maxpacket 0");

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
