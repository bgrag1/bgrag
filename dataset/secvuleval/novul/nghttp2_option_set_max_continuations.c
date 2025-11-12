void nghttp2_option_set_max_continuations(nghttp2_option *option, size_t val) {
  option->opt_set_mask |= NGHTTP2_OPT_MAX_CONTINUATIONS;
  option->max_continuations = val;
}