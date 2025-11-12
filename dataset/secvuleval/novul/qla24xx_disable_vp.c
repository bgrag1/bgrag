int
qla24xx_disable_vp(scsi_qla_host_t *vha)
{
	unsigned long flags;
	int ret = QLA_SUCCESS;
	fc_port_t *fcport;

	if (vha->hw->flags.edif_enabled) {
		if (DBELL_ACTIVE(vha))
			qla2x00_post_aen_work(vha, FCH_EVT_VENDOR_UNIQUE,
			    FCH_EVT_VENDOR_UNIQUE_VPORT_DOWN);
		/* delete sessions and flush sa_indexes */
		qla2x00_wait_for_sess_deletion(vha);
	}

	if (vha->hw->flags.fw_started)
		ret = qla24xx_control_vp(vha, VCE_COMMAND_DISABLE_VPS_LOGO_ALL);

	atomic_set(&vha->loop_state, LOOP_DOWN);
	atomic_set(&vha->loop_down_timer, LOOP_DOWN_TIME);
	list_for_each_entry(fcport, &vha->vp_fcports, list)
		fcport->logout_on_delete = 1;

	if (!vha->hw->flags.edif_enabled)
		qla2x00_wait_for_sess_deletion(vha);

	/* Remove port id from vp target map */
	spin_lock_irqsave(&vha->hw->hardware_lock, flags);
	qla_update_vp_map(vha, RESET_AL_PA);
	spin_unlock_irqrestore(&vha->hw->hardware_lock, flags);

	qla2x00_mark_vp_devices_dead(vha);
	atomic_set(&vha->vp_state, VP_FAILED);
	vha->flags.management_server_logged_in = 0;
	if (ret == QLA_SUCCESS) {
		fc_vport_set_state(vha->fc_vport, FC_VPORT_DISABLED);
	} else {
		fc_vport_set_state(vha->fc_vport, FC_VPORT_FAILED);
		return -1;
	}
	return 0;
}
