int qla_nvme_register_remote(struct scsi_qla_host *vha, struct fc_port *fcport)
{
	struct qla_nvme_rport *rport;
	struct nvme_fc_port_info req;
	int ret;

	if (!IS_ENABLED(CONFIG_NVME_FC))
		return 0;

	if (!vha->flags.nvme_enabled) {
		ql_log(ql_log_info, vha, 0x2100,
		    "%s: Not registering target since Host NVME is not enabled\n",
		    __func__);
		return 0;
	}

	if (qla_nvme_register_hba(vha))
		return 0;

	if (!vha->nvme_local_port)
		return 0;

	if (!(fcport->nvme_prli_service_param &
	    (NVME_PRLI_SP_TARGET | NVME_PRLI_SP_DISCOVERY)) ||
		(fcport->nvme_flag & NVME_FLAG_REGISTERED))
		return 0;

	fcport->nvme_flag &= ~NVME_FLAG_RESETTING;

	memset(&req, 0, sizeof(struct nvme_fc_port_info));
	req.port_name = wwn_to_u64(fcport->port_name);
	req.node_name = wwn_to_u64(fcport->node_name);
	req.port_role = 0;
	req.dev_loss_tmo = fcport->dev_loss_tmo;

	if (fcport->nvme_prli_service_param & NVME_PRLI_SP_INITIATOR)
		req.port_role = FC_PORT_ROLE_NVME_INITIATOR;

	if (fcport->nvme_prli_service_param & NVME_PRLI_SP_TARGET)
		req.port_role |= FC_PORT_ROLE_NVME_TARGET;

	if (fcport->nvme_prli_service_param & NVME_PRLI_SP_DISCOVERY)
		req.port_role |= FC_PORT_ROLE_NVME_DISCOVERY;

	req.port_id = fcport->d_id.b24;

	ql_log(ql_log_info, vha, 0x2102,
	    "%s: traddr=nn-0x%016llx:pn-0x%016llx PortID:%06x\n",
	    __func__, req.node_name, req.port_name,
	    req.port_id);

	ret = nvme_fc_register_remoteport(vha->nvme_local_port, &req,
	    &fcport->nvme_remote_port);
	if (ret) {
		ql_log(ql_log_warn, vha, 0x212e,
		    "Failed to register remote port. Transport returned %d\n",
		    ret);
		return ret;
	}

	nvme_fc_set_remoteport_devloss(fcport->nvme_remote_port,
				       fcport->dev_loss_tmo);

	if (fcport->nvme_prli_service_param & NVME_PRLI_SP_SLER)
		ql_log(ql_log_info, vha, 0x212a,
		       "PortID:%06x Supports SLER\n", req.port_id);

	if (fcport->nvme_prli_service_param & NVME_PRLI_SP_PI_CTRL)
		ql_log(ql_log_info, vha, 0x212b,
		       "PortID:%06x Supports PI control\n", req.port_id);

	rport = fcport->nvme_remote_port->private;
	rport->fcport = fcport;

	fcport->nvme_flag |= NVME_FLAG_REGISTERED;
	return 0;
}
