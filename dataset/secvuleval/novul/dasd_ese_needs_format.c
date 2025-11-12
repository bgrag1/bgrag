static int dasd_ese_needs_format(struct dasd_block *block, struct irb *irb)
{
	struct dasd_device *device = NULL;
	u8 *sense = NULL;

	if (!block)
		return 0;
	device = block->base;
	if (!device || !device->discipline->is_ese)
		return 0;
	if (!device->discipline->is_ese(device))
		return 0;

	sense = dasd_get_sense(irb);
	if (!sense)
		return 0;

	if (sense[1] & SNS1_NO_REC_FOUND)
		return 1;

	if ((sense[1] & SNS1_INV_TRACK_FORMAT) &&
	    scsw_is_tm(&irb->scsw) &&
	    !(sense[2] & SNS2_ENV_DATA_PRESENT))
		return 1;

	return 0;
}
