void test_refs_revparse__parses_at_head(void)
{
	test_id("HEAD", "a65fedf39aefe402d3bb6e24df4d4f5fe4547750", NULL, GIT_REVSPEC_SINGLE);
	test_id("@{0}", "a65fedf39aefe402d3bb6e24df4d4f5fe4547750", NULL, GIT_REVSPEC_SINGLE);
	test_id("@", "a65fedf39aefe402d3bb6e24df4d4f5fe4547750", NULL, GIT_REVSPEC_SINGLE);
}