TEST_IMPL(utf8_decode1_overrun) {
  const char* p;
  char b[1];
  char c[1];

  /* Single byte. */
  p = b;
  b[0] = 0x7F;
  ASSERT_EQ(0x7F, uv__utf8_decode1(&p, b + 1));
  ASSERT_PTR_EQ(p, b + 1);

  /* Multi-byte. */
  p = b;
  b[0] = 0xC0;
  ASSERT_EQ((unsigned) -1, uv__utf8_decode1(&p, b + 1));
  ASSERT_PTR_EQ(p, b + 1);

  b[0] = 0x7F;
  ASSERT_EQ(UV_EINVAL, uv__idna_toascii(b, b + 0, c, c + 1));
  ASSERT_EQ(UV_EINVAL, uv__idna_toascii(b, b + 1, c, c + 1));

  return 0;
}