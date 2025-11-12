TEST(core_close_range)
{
	int i, ret;
	int open_fds[101];

	for (i = 0; i < ARRAY_SIZE(open_fds); i++) {
		int fd;

		fd = open("/dev/null", O_RDONLY | O_CLOEXEC);
		ASSERT_GE(fd, 0) {
			if (errno == ENOENT)
				SKIP(return, "Skipping test since /dev/null does not exist");
		}

		open_fds[i] = fd;
	}

	EXPECT_EQ(-1, sys_close_range(open_fds[0], open_fds[100], -1)) {
		if (errno == ENOSYS)
			SKIP(return, "close_range() syscall not supported");
	}

	for (i = 0; i < 100; i++) {
		ret = fcntl(open_fds[i], F_DUPFD_QUERY, open_fds[i + 1]);
		if (ret < 0) {
			EXPECT_EQ(errno, EINVAL);
		} else {
			EXPECT_EQ(ret, 0);
		}
	}

	EXPECT_EQ(0, sys_close_range(open_fds[0], open_fds[50], 0));

	for (i = 0; i <= 50; i++)
		EXPECT_EQ(-1, fcntl(open_fds[i], F_GETFL));

	for (i = 51; i <= 100; i++)
		EXPECT_GT(fcntl(open_fds[i], F_GETFL), -1);

	/* create a couple of gaps */
	close(57);
	close(78);
	close(81);
	close(82);
	close(84);
	close(90);

	EXPECT_EQ(0, sys_close_range(open_fds[51], open_fds[92], 0));

	for (i = 51; i <= 92; i++)
		EXPECT_EQ(-1, fcntl(open_fds[i], F_GETFL));

	for (i = 93; i <= 100; i++)
		EXPECT_GT(fcntl(open_fds[i], F_GETFL), -1);

	/* test that the kernel caps and still closes all fds */
	EXPECT_EQ(0, sys_close_range(open_fds[93], open_fds[99], 0));

	for (i = 93; i <= 99; i++)
		EXPECT_EQ(-1, fcntl(open_fds[i], F_GETFL));

	EXPECT_GT(fcntl(open_fds[i], F_GETFL), -1);

	EXPECT_EQ(0, sys_close_range(open_fds[100], open_fds[100], 0));

	EXPECT_EQ(-1, fcntl(open_fds[100], F_GETFL));
}
