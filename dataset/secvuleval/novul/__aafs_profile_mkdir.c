int __aafs_profile_mkdir(struct aa_profile *profile, struct dentry *parent)
{
	struct aa_profile *child;
	struct dentry *dent = NULL, *dir;
	int error;

	AA_BUG(!profile);
	AA_BUG(!mutex_is_locked(&profiles_ns(profile)->lock));

	if (!parent) {
		struct aa_profile *p;
		p = aa_deref_parent(profile);
		dent = prof_dir(p);
		if (!dent) {
			error = -ENOENT;
			goto fail2;
		}
		/* adding to parent that previously didn't have children */
		dent = aafs_create_dir("profiles", dent);
		if (IS_ERR(dent))
			goto fail;
		prof_child_dir(p) = parent = dent;
	}

	if (!profile->dirname) {
		int len, id_len;
		len = mangle_name(profile->base.name, NULL);
		id_len = snprintf(NULL, 0, ".%ld", profile->ns->uniq_id);

		profile->dirname = kmalloc(len + id_len + 1, GFP_KERNEL);
		if (!profile->dirname) {
			error = -ENOMEM;
			goto fail2;
		}

		mangle_name(profile->base.name, profile->dirname);
		sprintf(profile->dirname + len, ".%ld", profile->ns->uniq_id++);
	}

	dent = aafs_create_dir(profile->dirname, parent);
	if (IS_ERR(dent))
		goto fail;
	prof_dir(profile) = dir = dent;

	dent = create_profile_file(dir, "name", profile,
				   &seq_profile_name_fops);
	if (IS_ERR(dent))
		goto fail;
	profile->dents[AAFS_PROF_NAME] = dent;

	dent = create_profile_file(dir, "mode", profile,
				   &seq_profile_mode_fops);
	if (IS_ERR(dent))
		goto fail;
	profile->dents[AAFS_PROF_MODE] = dent;

	dent = create_profile_file(dir, "attach", profile,
				   &seq_profile_attach_fops);
	if (IS_ERR(dent))
		goto fail;
	profile->dents[AAFS_PROF_ATTACH] = dent;

	if (profile->hash) {
		dent = create_profile_file(dir, "sha256", profile,
					   &seq_profile_hash_fops);
		if (IS_ERR(dent))
			goto fail;
		profile->dents[AAFS_PROF_HASH] = dent;
	}

// #ifdef CONFIG_SECURITY_APPARMOR_EXPORT_BINARY
	if (profile->rawdata) {
		if (aa_g_hash_policy) {
			dent = aafs_create("raw_sha256", S_IFLNK | 0444, dir,
					   profile->label.proxy, NULL, NULL,
					   &rawdata_link_sha256_iops);
			if (IS_ERR(dent))
				goto fail;
			aa_get_proxy(profile->label.proxy);
			profile->dents[AAFS_PROF_RAW_HASH] = dent;
		}
		dent = aafs_create("raw_abi", S_IFLNK | 0444, dir,
				   profile->label.proxy, NULL, NULL,
				   &rawdata_link_abi_iops);
		if (IS_ERR(dent))
			goto fail;
		aa_get_proxy(profile->label.proxy);
		profile->dents[AAFS_PROF_RAW_ABI] = dent;

		dent = aafs_create("raw_data", S_IFLNK | 0444, dir,
				   profile->label.proxy, NULL, NULL,
				   &rawdata_link_data_iops);
		if (IS_ERR(dent))
			goto fail;
		aa_get_proxy(profile->label.proxy);
		profile->dents[AAFS_PROF_RAW_DATA] = dent;
	}
#endif /*CONFIG_SECURITY_APPARMOR_EXPORT_BINARY */

	list_for_each_entry(child, &profile->base.profiles, base.list) {
		error = __aafs_profile_mkdir(child, prof_child_dir(profile));
		if (error)
			goto fail2;
	}

	return 0;

fail:
	error = PTR_ERR(dent);

fail2:
	__aafs_profile_rmdir(profile);

	return error;
}
