int bpf_uprobe_multi_link_attach(const union bpf_attr *attr, struct bpf_prog *prog)
{
	struct bpf_uprobe_multi_link *link = NULL;
	unsigned long __user *uref_ctr_offsets;
	struct bpf_link_primer link_primer;
	struct bpf_uprobe *uprobes = NULL;
	struct task_struct *task = NULL;
	unsigned long __user *uoffsets;
	u64 __user *ucookies;
	void __user *upath;
	u32 flags, cnt, i;
	struct path path;
	char *name;
	pid_t pid;
	int err;

	/* no support for 32bit archs yet */
	if (sizeof(u64) != sizeof(void *))
		return -EOPNOTSUPP;

	if (prog->expected_attach_type != BPF_TRACE_UPROBE_MULTI)
		return -EINVAL;

	flags = attr->link_create.uprobe_multi.flags;
	if (flags & ~BPF_F_UPROBE_MULTI_RETURN)
		return -EINVAL;

	/*
	 * path, offsets and cnt are mandatory,
	 * ref_ctr_offsets and cookies are optional
	 */
	upath = u64_to_user_ptr(attr->link_create.uprobe_multi.path);
	uoffsets = u64_to_user_ptr(attr->link_create.uprobe_multi.offsets);
	cnt = attr->link_create.uprobe_multi.cnt;
	pid = attr->link_create.uprobe_multi.pid;

	if (!upath || !uoffsets || !cnt || pid < 0)
		return -EINVAL;
	if (cnt > MAX_UPROBE_MULTI_CNT)
		return -E2BIG;

	uref_ctr_offsets = u64_to_user_ptr(attr->link_create.uprobe_multi.ref_ctr_offsets);
	ucookies = u64_to_user_ptr(attr->link_create.uprobe_multi.cookies);

	name = strndup_user(upath, PATH_MAX);
	if (IS_ERR(name)) {
		err = PTR_ERR(name);
		return err;
	}

	err = kern_path(name, LOOKUP_FOLLOW, &path);
	kfree(name);
	if (err)
		return err;

	if (!d_is_reg(path.dentry)) {
		err = -EBADF;
		goto error_path_put;
	}

	if (pid) {
		task = get_pid_task(find_vpid(pid), PIDTYPE_TGID);
		if (!task) {
			err = -ESRCH;
			goto error_path_put;
		}
	}

	err = -ENOMEM;

	link = kzalloc(sizeof(*link), GFP_KERNEL);
	uprobes = kvcalloc(cnt, sizeof(*uprobes), GFP_KERNEL);

	if (!uprobes || !link)
		goto error_free;

	for (i = 0; i < cnt; i++) {
		if (__get_user(uprobes[i].offset, uoffsets + i)) {
			err = -EFAULT;
			goto error_free;
		}
		if (uprobes[i].offset < 0) {
			err = -EINVAL;
			goto error_free;
		}
		if (uref_ctr_offsets && __get_user(uprobes[i].ref_ctr_offset, uref_ctr_offsets + i)) {
			err = -EFAULT;
			goto error_free;
		}
		if (ucookies && __get_user(uprobes[i].cookie, ucookies + i)) {
			err = -EFAULT;
			goto error_free;
		}

		uprobes[i].link = link;

		if (flags & BPF_F_UPROBE_MULTI_RETURN)
			uprobes[i].consumer.ret_handler = uprobe_multi_link_ret_handler;
		else
			uprobes[i].consumer.handler = uprobe_multi_link_handler;

		if (pid)
			uprobes[i].consumer.filter = uprobe_multi_link_filter;
	}

	link->cnt = cnt;
	link->uprobes = uprobes;
	link->path = path;
	link->task = task;
	link->flags = flags;

	bpf_link_init(&link->link, BPF_LINK_TYPE_UPROBE_MULTI,
		      &bpf_uprobe_multi_link_lops, prog);

	for (i = 0; i < cnt; i++) {
		uprobes[i].uprobe = uprobe_register(d_real_inode(link->path.dentry),
						    uprobes[i].offset,
						    uprobes[i].ref_ctr_offset,
						    &uprobes[i].consumer);
		if (IS_ERR(uprobes[i].uprobe)) {
			err = PTR_ERR(uprobes[i].uprobe);
			link->cnt = i;
			goto error_unregister;
		}
	}

	err = bpf_link_prime(&link->link, &link_primer);
	if (err)
		goto error_unregister;

	return bpf_link_settle(&link_primer);

error_unregister:
	bpf_uprobe_unregister(uprobes, link->cnt);

error_free:
	kvfree(uprobes);
	kfree(link);
	if (task)
		put_task_struct(task);
error_path_put:
	path_put(&path);
	return err;
}
