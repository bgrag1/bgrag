void f2fs_handle_critical_error(struct f2fs_sb_info *sbi, unsigned char reason,
							bool irq_context)
{
	struct super_block *sb = sbi->sb;
	bool shutdown = reason == STOP_CP_REASON_SHUTDOWN;
	bool continue_fs = !shutdown &&
			F2FS_OPTION(sbi).errors == MOUNT_ERRORS_CONTINUE;

	set_ckpt_flags(sbi, CP_ERROR_FLAG);

	if (!f2fs_hw_is_readonly(sbi)) {
		save_stop_reason(sbi, reason);

		if (irq_context && !shutdown)
			schedule_work(&sbi->s_error_work);
		else
			f2fs_record_stop_reason(sbi);
	}

	/*
	 * We force ERRORS_RO behavior when system is rebooting. Otherwise we
	 * could panic during 'reboot -f' as the underlying device got already
	 * disabled.
	 */
	if (F2FS_OPTION(sbi).errors == MOUNT_ERRORS_PANIC &&
				!shutdown && !system_going_down() &&
				!is_sbi_flag_set(sbi, SBI_IS_SHUTDOWN))
		panic("F2FS-fs (device %s): panic forced after error\n",
							sb->s_id);

	if (shutdown)
		set_sbi_flag(sbi, SBI_IS_SHUTDOWN);

	/*
	 * Continue filesystem operators if errors=continue. Should not set
	 * RO by shutdown, since RO bypasses thaw_super which can hang the
	 * system.
	 */
	if (continue_fs || f2fs_readonly(sb) || shutdown) {
		f2fs_warn(sbi, "Stopped filesystem due to reason: %d", reason);
		return;
	}

	f2fs_warn(sbi, "Remounting filesystem read-only");
	/*
	 * Make sure updated value of ->s_mount_flags will be visible before
	 * ->s_flags update
	 */
	smp_wmb();
	sb->s_flags |= SB_RDONLY;
}
