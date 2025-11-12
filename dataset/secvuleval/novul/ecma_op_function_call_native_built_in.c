ecma_op_function_call_native_built_in (ecma_object_t *func_obj_p, /**< Function object */
                                       ecma_value_t this_arg_value, /**< 'this' argument's value */
                                       const ecma_value_t *arguments_list_p, /**< arguments list */
                                       uint32_t arguments_list_len) /**< length of arguments list */
{
  JERRY_ASSERT (ecma_get_object_type (func_obj_p) == ECMA_OBJECT_TYPE_BUILT_IN_FUNCTION);

  ECMA_CHECK_STACK_USAGE ();

#if JERRY_BUILTIN_REALMS
  ecma_global_object_t *saved_global_object_p = JERRY_CONTEXT (global_object_p);

  ecma_extended_object_t *ext_func_obj_p = (ecma_extended_object_t *) func_obj_p;
  JERRY_CONTEXT (global_object_p) =
    ECMA_GET_INTERNAL_VALUE_POINTER (ecma_global_object_t, ext_func_obj_p->u.built_in.realm_value);
#endif /* JERRY_BUILTIN_REALMS */

  ecma_value_t ret_value =
    ecma_builtin_dispatch_call (func_obj_p, this_arg_value, arguments_list_p, arguments_list_len);

#if JERRY_BUILTIN_REALMS
  JERRY_CONTEXT (global_object_p) = saved_global_object_p;
#endif /* JERRY_BUILTIN_REALMS */
  return ret_value;
} /* ecma_op_function_call_native_built_in */