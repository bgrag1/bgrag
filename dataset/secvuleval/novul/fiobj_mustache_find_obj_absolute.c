static inline VALUE fiobj_mustache_find_obj_absolute(VALUE udata,
                                                     const char *name,
                                                     uint32_t name_len) {
  VALUE tmp;
  if (!RB_TYPE_P(udata, T_HASH)) {
    if (name_len == 1 && name[0] == '.')
      return udata;
    /* search by method */
    ID name_id = rb_intern2(name, name_len);
    if (rb_respond_to(udata, name_id)) {
      return IodineCaller.call(udata, name_id);
    }
    return Qnil;
  }
  /* search by Symbol */
  ID name_id = rb_intern2(name, name_len);
  VALUE key = ID2SYM(name_id);
  tmp = rb_hash_lookup2(udata, key, Qundef);
  if (tmp != Qundef)
    return tmp;
  /* search by String */
  key = rb_sym2str(key);
  tmp = rb_hash_lookup2(udata, key, Qundef);
  rb_str_free(key);
  if (tmp != Qundef)
    return tmp;
  /* search by method */
  tmp = Qnil;
  if (rb_respond_to(udata, name_id)) {
    tmp = IodineCaller.call(udata, name_id);
  }

  return tmp;
}