void nvme_stop_ctrl(struct nvme_ctrl *ctrl)
{
	nvme_mpath_stop(ctrl);
	nvme_auth_stop(ctrl);
	nvme_stop_keep_alive(ctrl);
	nvme_stop_failfast_work(ctrl);
	flush_work(&ctrl->async_event_work);
	cancel_work_sync(&ctrl->fw_act_work);
	if (ctrl->ops->stop_ctrl)
		ctrl->ops->stop_ctrl(ctrl);
}
