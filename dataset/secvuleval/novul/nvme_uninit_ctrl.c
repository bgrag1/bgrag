void nvme_uninit_ctrl(struct nvme_ctrl *ctrl)
{
	nvme_stop_keep_alive(ctrl);
	nvme_hwmon_exit(ctrl);
	nvme_fault_inject_fini(&ctrl->fault_inject);
	dev_pm_qos_hide_latency_tolerance(ctrl->device);
	cdev_device_del(&ctrl->cdev, ctrl->device);
	nvme_put_ctrl(ctrl);
}
