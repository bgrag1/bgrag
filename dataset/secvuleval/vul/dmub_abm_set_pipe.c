bool dmub_abm_set_pipe(struct abm *abm,
		uint32_t otg_inst,
		uint32_t option,
		uint32_t panel_inst,
		uint32_t pwrseq_inst)
{
	union dmub_rb_cmd cmd;
	struct dc_context *dc = abm->ctx;
	uint32_t ramping_boundary = 0xFFFF;

	memset(&cmd, 0, sizeof(cmd));
	cmd.abm_set_pipe.header.type = DMUB_CMD__ABM;
	cmd.abm_set_pipe.header.sub_type = DMUB_CMD__ABM_SET_PIPE;
	cmd.abm_set_pipe.abm_set_pipe_data.otg_inst = otg_inst;
	cmd.abm_set_pipe.abm_set_pipe_data.pwrseq_inst = pwrseq_inst;
	cmd.abm_set_pipe.abm_set_pipe_data.set_pipe_option = option;
	cmd.abm_set_pipe.abm_set_pipe_data.panel_inst = panel_inst;
	cmd.abm_set_pipe.abm_set_pipe_data.ramping_boundary = ramping_boundary;
	cmd.abm_set_pipe.header.payload_bytes = sizeof(struct dmub_cmd_abm_set_pipe_data);

	dc_wake_and_execute_dmub_cmd(dc, &cmd, DM_DMUB_WAIT_TYPE_WAIT);

	return true;
}
