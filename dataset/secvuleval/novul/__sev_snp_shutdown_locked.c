static int __sev_snp_shutdown_locked(int *error, bool panic)
{
	struct psp_device *psp = psp_master;
	struct sev_device *sev;
	struct sev_data_snp_shutdown_ex data;
	int ret;

	if (!psp || !psp->sev_data)
		return 0;

	sev = psp->sev_data;

	if (!sev->snp_initialized)
		return 0;

	memset(&data, 0, sizeof(data));
	data.len = sizeof(data);
	data.iommu_snp_shutdown = 1;

	/*
	 * If invoked during panic handling, local interrupts are disabled
	 * and all CPUs are stopped, so wbinvd_on_all_cpus() can't be called.
	 * In that case, a wbinvd() is done on remote CPUs via the NMI
	 * callback, so only a local wbinvd() is needed here.
	 */
	if (!panic)
		wbinvd_on_all_cpus();
	else
		wbinvd();

	ret = __sev_do_cmd_locked(SEV_CMD_SNP_SHUTDOWN_EX, &data, error);
	/* SHUTDOWN may require DF_FLUSH */
	if (*error == SEV_RET_DFFLUSH_REQUIRED) {
		ret = __sev_do_cmd_locked(SEV_CMD_SNP_DF_FLUSH, NULL, NULL);
		if (ret) {
			dev_err(sev->dev, "SEV-SNP DF_FLUSH failed\n");
			return ret;
		}
		/* reissue the shutdown command */
		ret = __sev_do_cmd_locked(SEV_CMD_SNP_SHUTDOWN_EX, &data,
					  error);
	}
	if (ret) {
		dev_err(sev->dev, "SEV-SNP firmware shutdown failed\n");
		return ret;
	}

	/*
	 * SNP_SHUTDOWN_EX with IOMMU_SNP_SHUTDOWN set to 1 disables SNP
	 * enforcement by the IOMMU and also transitions all pages
	 * associated with the IOMMU to the Reclaim state.
	 * Firmware was transitioning the IOMMU pages to Hypervisor state
	 * before version 1.53. But, accounting for the number of assigned
	 * 4kB pages in a 2M page was done incorrectly by not transitioning
	 * to the Reclaim state. This resulted in RMP #PF when later accessing
	 * the 2M page containing those pages during kexec boot. Hence, the
	 * firmware now transitions these pages to Reclaim state and hypervisor
	 * needs to transition these pages to shared state. SNP Firmware
	 * version 1.53 and above are needed for kexec boot.
	 */
	ret = amd_iommu_snp_disable();
	if (ret) {
		dev_err(sev->dev, "SNP IOMMU shutdown failed\n");
		return ret;
	}

	sev->snp_initialized = false;
	dev_dbg(sev->dev, "SEV-SNP firmware shutdown\n");

	return ret;
}
