int proc_cpuset_show(struct seq_file *m, struct pid_namespace *ns,
		     struct pid *pid, struct task_struct *tsk)
{
	char *buf;
	struct cgroup_subsys_state *css;
	int retval;

	retval = -ENOMEM;
	buf = kmalloc(PATH_MAX, GFP_KERNEL);
	if (!buf)
		goto out;

	css = task_get_css(tsk, cpuset_cgrp_id);
	retval = cgroup_path_ns(css->cgroup, buf, PATH_MAX,
				current->nsproxy->cgroup_ns);
	css_put(css);
	if (retval == -E2BIG)
		retval = -ENAMETOOLONG;
	if (retval < 0)
		goto out_free;
	seq_puts(m, buf);
	seq_putc(m, '\n');
	retval = 0;
out_free:
	kfree(buf);
out:
	return retval;
}
