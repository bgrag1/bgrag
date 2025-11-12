static int sdma_v4_0_process_trap_irq(struct amdgpu_device *adev,
				      struct amdgpu_irq_src *source,
				      struct amdgpu_iv_entry *entry)
{
	uint32_t instance;

	DRM_DEBUG("IH: SDMA trap\n");
	instance = sdma_v4_0_irq_id_to_seq(entry->client_id);
	switch (entry->ring_id) {
	case 0:
		amdgpu_fence_process(&adev->sdma.instance[instance].ring);
		break;
	case 1:
		if (amdgpu_ip_version(adev, SDMA0_HWIP, 0) ==
		    IP_VERSION(4, 2, 0))
			amdgpu_fence_process(&adev->sdma.instance[instance].page);
		break;
	case 2:
		/* XXX compute */
		break;
	case 3:
		if (amdgpu_ip_version(adev, SDMA0_HWIP, 0) !=
		    IP_VERSION(4, 2, 0))
			amdgpu_fence_process(&adev->sdma.instance[instance].page);
		break;
	}
	return 0;
}
