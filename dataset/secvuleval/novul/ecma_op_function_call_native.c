ecma_op_function_call_native (ecma_object_t *func_obj_p, /**< Function object */
                              ecma_value_t this_arg_value, /**< 'this' argument's value */
                              const ecma_value_t *arguments_list_p, /**< arguments list */
                              uint32_t arguments_list_len) /**< length of arguments list */
{
  JERRY_ASSERT (ecma_get_object_type (func_obj_p) == ECMA_OBJECT_TYPE_NATIVE_FUNCTION);

  ECMA_CHECK_STACK_USAGE ();

  ecma_native_function_t *native_function_p = (ecma_native_function_t *) func_obj_p;

#if JERRY_BUILTIN_REALMS
  ecma_global_object_t *saved_global_object_p = JERRY_CONTEXT (global_object_p);
  JERRY_CONTEXT (global_object_p) =
    ECMA_GET_INTERNAL_VALUE_POINTER (ecma_global_object_t, native_function_p->realm_value);
#endif /* JERRY_BUILTIN_REALMS */

  jerry_call_info_t call_info;
  call_info.function = ecma_make_object_value (func_obj_p);
  call_info.this_value = this_arg_value;

  ecma_object_t *new_target_p = JERRY_CONTEXT (current_new_target_p);
  call_info.new_target = (new_target_p == NULL) ? ECMA_VALUE_UNDEFINED : ecma_make_object_value (new_target_p);

  JERRY_ASSERT (native_function_p->native_handler_cb != NULL);
  ecma_value_t ret_value = native_function_p->native_handler_cb (&call_info, arguments_list_p, arguments_list_len);
#if JERRY_BUILTIN_REALMS
  JERRY_CONTEXT (global_object_p) = saved_global_object_p;
#endif /* JERRY_BUILTIN_REALMS */

  if (JERRY_UNLIKELY (ecma_is_value_exception (ret_value)))
  {
    ecma_throw_exception (ret_value);
    return ECMA_VALUE_ERROR;
  }

#if JERRY_DEBUGGER
  JERRY_DEBUGGER_CLEAR_FLAGS (JERRY_DEBUGGER_VM_EXCEPTION_THROWN);
#endif /* JERRY_DEBUGGER */
  return ret_value;
} /* ecma_op_function_call_native */