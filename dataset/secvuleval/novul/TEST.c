TEST(close_range_bitmap_corruption)
{
	pid_t pid;
	int status;
	struct __clone_args args = {
		.flags = CLONE_FILES,
		.exit_signal = SIGCHLD,
	};

	/* get the first 128 descriptors open */
	for (int i = 2; i < 128; i++)
		EXPECT_GE(dup2(0, i), 0);

	/* get descriptor table shared */
	pid = sys_clone3(&args, sizeof(args));
	ASSERT_GE(pid, 0);

	if (pid == 0) {
		/* unshare and truncate descriptor table down to 64 */
		if (sys_close_range(64, ~0U, CLOSE_RANGE_UNSHARE))
			exit(EXIT_FAILURE);

		ASSERT_EQ(fcntl(64, F_GETFD), -1);
		/* ... and verify that the range 64..127 is not
		   stuck "fully used" according to secondary bitmap */
		EXPECT_EQ(dup(0), 64)
			exit(EXIT_FAILURE);
		exit(EXIT_SUCCESS);
	}

	EXPECT_EQ(waitpid(pid, &status, 0), pid);
	EXPECT_EQ(true, WIFEXITED(status));
	EXPECT_EQ(0, WEXITSTATUS(status));
}
