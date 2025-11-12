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

	return !!(sense[1] & SNS1_NO_REC_FOUND) ||
		!!(sense[1] & SNS1_FILE_PROTECTED) ||
		scsw_cstat(&irb->scsw) == SCHN_STAT_INCORR_LEN;
}
