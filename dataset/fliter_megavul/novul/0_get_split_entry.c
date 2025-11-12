static int
get_split_entry(const char **start,
                size_t size,
                coap_str_const_t *keyword,
                oscore_value_t *value) {
  const char *begin = *start;
  const char *end;
  const char *kend;
  const char *split;
  size_t i;

retry:
  kend = end = memchr(begin, '\n', size);
  if (end == NULL)
    return 0;

  /* Track beginning of next line */
  *start = end + 1;
  if (end > begin && end[-1] == '\r')
    end--;

  if (begin[0] == '#' || (end - begin) == 0) {
    /* Skip comment / blank line */
    size -= kend - begin + 1;
    begin = *start;
    goto retry;
  }

  /* Get in the keyword */
  split = memchr(begin, ',', end - begin);
  if (split == NULL)
    goto bad_entry;

  keyword->s = (const uint8_t *)begin;
  keyword->length = split - begin;

  begin = split + 1;
  if ((end - begin) == 0)
    goto bad_entry;
  /* Get in the encoding */
  split = memchr(begin, ',', end - begin);
  if (split == NULL)
    goto bad_entry;

  for (i = 0; oscore_encoding[i].name.s; i++) {
    coap_str_const_t temp = { split - begin, (const uint8_t *)begin };

    if (coap_string_equal(&temp, &oscore_encoding[i].name)) {
      value->encoding = oscore_encoding[i].encoding;
      value->encoding_name = (const char *)oscore_encoding[i].name.s;
      break;
    }
  }
  if (oscore_encoding[i].name.s == NULL)
    goto bad_entry;

  begin = split + 1;
  if ((end - begin) == 0)
    goto bad_entry;
  /* Get in the keyword's value */
  if (begin[0] == '"') {
    split = memchr(&begin[1], '"', end - split - 1);
    if (split == NULL)
      goto bad_entry;
    end = split;
    begin++;
  }
  switch (value->encoding) {
  case COAP_ENC_ASCII:
    value->u.value_bin =
        coap_new_bin_const((const uint8_t *)begin, end - begin);
    break;
  case COAP_ENC_HEX:
    /* Parse the hex into binary */
    value->u.value_bin = parse_hex_bin(begin, end);
    if (value->u.value_bin == NULL)
      goto bad_entry;
    break;
  case COAP_ENC_INTEGER:
    value->u.value_int = atoi(begin);
    break;
  case COAP_ENC_TEXT:
    value->u.value_str.s = (const uint8_t *)begin;
    value->u.value_str.length = end - begin;
    break;
  case COAP_ENC_BOOL:
    if (memcmp("true", begin, end - begin) == 0)
      value->u.value_int = 1;
    else if (memcmp("false", begin, end - begin) == 0)
      value->u.value_int = 0;
    else
      goto bad_entry;
    break;
  case COAP_ENC_LAST:
  default:
    goto bad_entry;
  }
  return 1;

bad_entry:
  coap_log_warn("oscore_conf: Unrecognized configuration entry '%.*s'\n",
                (int)(end - begin),
                begin);
  return 0;
}