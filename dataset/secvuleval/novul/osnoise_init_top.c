struct osnoise_tool *osnoise_init_top(struct osnoise_top_params *params)
{
	struct osnoise_tool *tool;
	int nr_cpus;

	nr_cpus = sysconf(_SC_NPROCESSORS_CONF);

	tool = osnoise_init_tool("osnoise_top");
	if (!tool)
		return NULL;

	tool->data = osnoise_alloc_top(nr_cpus);
	if (!tool->data) {
		osnoise_destroy_tool(tool);
		return NULL;
	}

	tool->params = params;

	tep_register_event_handler(tool->trace.tep, -1, "ftrace", "osnoise",
				   osnoise_top_handler, NULL);

	return tool;
}
