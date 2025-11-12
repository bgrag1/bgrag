static int vdec_close(struct file *file)
{
	struct venus_inst *inst = to_inst(file);

	vdec_pm_get(inst);

	v4l2_m2m_ctx_release(inst->m2m_ctx);
	v4l2_m2m_release(inst->m2m_dev);
	vdec_ctrl_deinit(inst);
	ida_destroy(&inst->dpb_ids);
	hfi_session_destroy(inst);
	mutex_destroy(&inst->lock);
	mutex_destroy(&inst->ctx_q_lock);
	v4l2_fh_del(&inst->fh);
	v4l2_fh_exit(&inst->fh);

	vdec_pm_put(inst, false);

	kfree(inst);
	return 0;
}
