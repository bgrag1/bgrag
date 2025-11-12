void amdgpu_mes_remove_ring(struct amdgpu_device *adev,
			    struct amdgpu_ring *ring)
{
	if (!ring)
		return;

	amdgpu_mes_remove_hw_queue(adev, ring->hw_queue_id);
	del_timer_sync(&ring->fence_drv.fallback_timer);
	amdgpu_ring_fini(ring);
	kfree(ring);
}
