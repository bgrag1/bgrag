static __u8 *cougar_report_fixup(struct hid_device *hdev, __u8 *rdesc,
				 unsigned int *rsize)
{
	if (rdesc[2] == 0x09 && rdesc[3] == 0x02 &&
	    (rdesc[115] | rdesc[116] << 8) >= HID_MAX_USAGES) {
		hid_info(hdev,
			"usage count exceeds max: fixing up report descriptor\n");
		rdesc[115] = ((HID_MAX_USAGES-1) & 0xff);
		rdesc[116] = ((HID_MAX_USAGES-1) >> 8);
	}
	return rdesc;
}
