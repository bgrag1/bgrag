static VALUE iodine_mustache_new(int argc, VALUE *argv, VALUE self) {
  VALUE filename = Qnil, template = Qnil;
  if (argc == 1 && RB_TYPE_P(argv[0], T_HASH)) {
    /* named arguments */
    filename = rb_hash_aref(argv[0], filename_id);
    template = rb_hash_aref(argv[0], template_id);
  } else {
    /* regular arguments */
    if (argc == 0 || argc > 2)
      rb_raise(rb_eArgError, "expecting 1..2 arguments or named arguments.");
    filename = argv[0];
    if (argc > 1) {
      template = argv[1];
    }
  }
  if (filename == Qnil && template == Qnil)
    rb_raise(rb_eArgError, "need either template contents or file name.");

  if (template != Qnil)
    Check_Type(template, T_STRING);
  if (filename != Qnil)
    Check_Type(filename, T_STRING);

  mustache_s **m = NULL;
  TypedData_Get_Struct(self, mustache_s *, &iodine_mustache_data_type, m);
  if (!m) {
    rb_raise(rb_eRuntimeError, "Iodine::Mustache allocation error.");
  }

  mustache_error_en err;
  *m = mustache_load(.filename =
                         (filename == Qnil ? NULL : RSTRING_PTR(filename)),
                     .filename_len =
                         (filename == Qnil ? 0 : RSTRING_LEN(filename)),
                     .data = (template == Qnil ? NULL : RSTRING_PTR(template)),
                     .data_len = (template == Qnil ? 0 : RSTRING_LEN(template)),
                     .err = &err);

  if (!*m)
    goto error;
  return self;
error:
  switch (err) {
  case MUSTACHE_OK:
    rb_raise(rb_eRuntimeError, "Iodine::Mustache template ok, unknown error.");
    break;
  case MUSTACHE_ERR_TOO_DEEP:
    rb_raise(rb_eRuntimeError, "Iodine::Mustache element nesting too deep.");
    break;
  case MUSTACHE_ERR_CLOSURE_MISMATCH:
    rb_raise(rb_eRuntimeError,
             "Iodine::Mustache template error, closure mismatch.");
    break;
  case MUSTACHE_ERR_FILE_NOT_FOUND:
    rb_raise(rb_eLoadError, "Iodine::Mustache template not found.");
    break;
  case MUSTACHE_ERR_FILE_TOO_BIG:
    rb_raise(rb_eLoadError, "Iodine::Mustache template too big.");
    break;
  case MUSTACHE_ERR_FILE_NAME_TOO_LONG:
    rb_raise(rb_eRuntimeError, "Iodine::Mustache template name too long.");
    break;
  case MUSTACHE_ERR_EMPTY_TEMPLATE:
    rb_raise(rb_eRuntimeError, "Iodine::Mustache template is empty.");
    break;
  case MUSTACHE_ERR_UNKNOWN:
    rb_raise(rb_eRuntimeError, "Iodine::Mustache unknown error.");
    break;
  case MUSTACHE_ERR_USER_ERROR:
    rb_raise(rb_eRuntimeError, "Iodine::Mustache internal error.");
    break;
  case MUSTACHE_ERR_FILE_NAME_TOO_SHORT:
    rb_raise(rb_eRuntimeError, "Iodine::Mustache template file name too long.");

    break;
  case MUSTACHE_ERR_DELIMITER_TOO_LONG:
    rb_raise(rb_eRuntimeError, "Iodine::Mustache new delimiter is too long.");

    break;
  case MUSTACHE_ERR_NAME_TOO_LONG:
    rb_raise(rb_eRuntimeError,
             "Iodine::Mustache section name in template is too long.");
  default:
    break;
  }
  return self;
}