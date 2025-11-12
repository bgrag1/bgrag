void i915_vma_revoke_fence(struct i915_vma *vma)
{
	struct i915_fence_reg *fence = vma->fence;
	intel_wakeref_t wakeref;

	lockdep_assert_held(&vma->vm->mutex);
	if (!fence)
		return;

	GEM_BUG_ON(fence->vma != vma);
	i915_active_wait(&fence->active);
	GEM_BUG_ON(!i915_active_is_idle(&fence->active));
	GEM_BUG_ON(atomic_read(&fence->pin_count));

	fence->tiling = 0;
	WRITE_ONCE(fence->vma, NULL);
	vma->fence = NULL;

	/*
	 * Skip the write to HW if and only if the device is currently
	 * suspended.
	 *
	 * If the driver does not currently hold a wakeref (if_in_use == 0),
	 * the device may currently be runtime suspended, or it may be woken
	 * up before the suspend takes place. If the device is not suspended
	 * (powered down) and we skip clearing the fence register, the HW is
	 * left in an undefined state where we may end up with multiple
	 * registers overlapping.
	 */
	with_intel_runtime_pm_if_active(fence_to_uncore(fence)->rpm, wakeref)
		fence_write(fence);
}
