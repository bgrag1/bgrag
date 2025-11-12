ecma_op_function_call_simple (ecma_object_t *func_obj_p, /**< Function object */
                              ecma_value_t this_binding, /**< 'this' argument's value */
                              const ecma_value_t *arguments_list_p, /**< arguments list */
                              uint32_t arguments_list_len) /**< length of arguments list */
{
  JERRY_ASSERT (ecma_get_object_type (func_obj_p) == ECMA_OBJECT_TYPE_FUNCTION);

  ECMA_CHECK_STACK_USAGE ();

  vm_frame_ctx_shared_args_t shared_args;
  shared_args.header.status_flags = VM_FRAME_CTX_SHARED_HAS_ARG_LIST;
  shared_args.header.function_object_p = func_obj_p;
  shared_args.arg_list_p = arguments_list_p;
  shared_args.arg_list_len = arguments_list_len;

  /* Entering Function Code (ECMA-262 v5, 10.4.3) */
  ecma_extended_object_t *ext_func_p = (ecma_extended_object_t *) func_obj_p;

  ecma_object_t *scope_p = ECMA_GET_NON_NULL_POINTER_FROM_POINTER_TAG (ecma_object_t, ext_func_p->u.function.scope_cp);

  /* 8. */
  const ecma_compiled_code_t *bytecode_data_p = ecma_op_function_get_compiled_code (ext_func_p);
  uint16_t status_flags = bytecode_data_p->status_flags;

  shared_args.header.bytecode_header_p = bytecode_data_p;

#if JERRY_BUILTIN_REALMS
  ecma_global_object_t *realm_p = ecma_op_function_get_realm (bytecode_data_p);
#endif /* JERRY_BUILTIN_REALMS */

  /* 5. */
  if (!(status_flags & CBC_CODE_FLAGS_LEXICAL_ENV_NOT_NEEDED))
  {
    shared_args.header.status_flags |= VM_FRAME_CTX_SHARED_FREE_LOCAL_ENV;
    scope_p = ecma_create_decl_lex_env (scope_p);
  }

  /* 1. */
  switch (CBC_FUNCTION_GET_TYPE (status_flags))
  {
    case CBC_FUNCTION_CONSTRUCTOR:
    {
      return ecma_op_function_call_constructor (&shared_args, scope_p, this_binding);
    }
    case CBC_FUNCTION_ARROW:
    {
      ecma_arrow_function_t *arrow_func_p = (ecma_arrow_function_t *) func_obj_p;

      if (ecma_is_value_undefined (arrow_func_p->new_target))
      {
        JERRY_CONTEXT (current_new_target_p) = NULL;
      }
      else
      {
        JERRY_CONTEXT (current_new_target_p) = ecma_get_object_from_value (arrow_func_p->new_target);
      }

      this_binding = arrow_func_p->this_binding;

      if (JERRY_UNLIKELY (this_binding == ECMA_VALUE_UNINITIALIZED))
      {
        ecma_environment_record_t *env_record_p = ecma_op_get_environment_record (scope_p);
        JERRY_ASSERT (env_record_p);
        this_binding = env_record_p->this_binding;
      }
      break;
    }
    default:
    {
      shared_args.header.status_flags |= VM_FRAME_CTX_SHARED_NON_ARROW_FUNC;

      if (status_flags & CBC_CODE_FLAGS_STRICT_MODE)
      {
        break;
      }

      if (ecma_is_value_undefined (this_binding) || ecma_is_value_null (this_binding))
      {
        /* 2. */
#if JERRY_BUILTIN_REALMS
        this_binding = realm_p->this_binding;
#else /* !JERRY_BUILTIN_REALMS */
        this_binding = ecma_make_object_value (ecma_builtin_get_global ());
#endif /* JERRY_BUILTIN_REALMS */
      }
      else if (!ecma_is_value_object (this_binding))
      {
        /* 3., 4. */
        this_binding = ecma_op_to_object (this_binding);
        shared_args.header.status_flags |= VM_FRAME_CTX_SHARED_FREE_THIS;

        JERRY_ASSERT (!ECMA_IS_VALUE_ERROR (this_binding));
      }
      break;
    }
  }

#if JERRY_BUILTIN_REALMS
  ecma_global_object_t *saved_global_object_p = JERRY_CONTEXT (global_object_p);
  JERRY_CONTEXT (global_object_p) = realm_p;
#endif /* JERRY_BUILTIN_REALMS */

  ecma_value_t ret_value = vm_run (&shared_args.header, this_binding, scope_p);

#if JERRY_BUILTIN_REALMS
  JERRY_CONTEXT (global_object_p) = saved_global_object_p;
#endif /* JERRY_BUILTIN_REALMS */

  if (JERRY_UNLIKELY (shared_args.header.status_flags & VM_FRAME_CTX_SHARED_FREE_LOCAL_ENV))
  {
    ecma_deref_object (scope_p);
  }

  if (JERRY_UNLIKELY (shared_args.header.status_flags & VM_FRAME_CTX_SHARED_FREE_THIS))
  {
    ecma_free_value (this_binding);
  }

  return ret_value;
} /* ecma_op_function_call_simple */