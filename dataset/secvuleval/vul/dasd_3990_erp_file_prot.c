static struct dasd_ccw_req *
dasd_3990_erp_file_prot(struct dasd_ccw_req * erp)
{

	struct dasd_device *device = erp->startdev;

	/*
	 * In some cases the 'File Protected' error might be expected and
	 * log messages shouldn't be written then.
	 * Check if the according suppress bit is set.
	 */
	if (!test_bit(DASD_CQR_SUPPRESS_FP, &erp->flags))
		dev_err(&device->cdev->dev,
			"Accessing the DASD failed because of a hardware error\n");

	return dasd_3990_erp_cleanup(erp, DASD_CQR_FAILED);

}				/* end dasd_3990_erp_file_prot */
