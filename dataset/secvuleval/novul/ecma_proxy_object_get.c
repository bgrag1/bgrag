ecma_proxy_object_get (ecma_object_t *obj_p, /**< proxy object */
                       ecma_string_t *prop_name_p, /**< property name */
                       ecma_value_t receiver) /**< receiver to invoke getter function */
{
  JERRY_ASSERT (ECMA_OBJECT_IS_PROXY (obj_p));
  ECMA_CHECK_STACK_USAGE ();

  ecma_proxy_object_t *proxy_obj_p = (ecma_proxy_object_t *) obj_p;

  /* 2. */
  ecma_value_t handler = proxy_obj_p->handler;

  /* 3-6. */
  ecma_value_t trap = ecma_validate_proxy_object (handler, LIT_MAGIC_STRING_GET);

  /* 7. */
  if (ECMA_IS_VALUE_ERROR (trap))
  {
    return trap;
  }

  /* 8. */
  if (ecma_is_value_undefined (trap))
  {
    ecma_object_t *target_obj_p = ecma_get_object_from_value (proxy_obj_p->target);
    ecma_value_t result = ecma_op_object_get_with_receiver (target_obj_p, prop_name_p, receiver);
    JERRY_BLOCK_TAIL_CALL_OPTIMIZATION ();
    return result;
  }

  ecma_object_t *func_obj_p = ecma_get_object_from_value (trap);
  ecma_value_t prop_value = ecma_make_prop_name_value (prop_name_p);
  ecma_value_t args[] = { proxy_obj_p->target, prop_value, receiver };

  /* 9. */
  ecma_ref_object (obj_p);
  ecma_value_t trap_result = ecma_op_function_call (func_obj_p, handler, args, 3);
  ecma_deref_object (obj_p);

  ecma_deref_object (func_obj_p);

  /* 10. */
  if (ECMA_IS_VALUE_ERROR (trap_result) || (obj_p->u2.prototype_cp & JERRY_PROXY_SKIP_RESULT_VALIDATION))
  {
    return trap_result;
  }

  /* 11. */
  ecma_property_descriptor_t target_desc;
  ecma_value_t status = ecma_op_get_own_property_descriptor (proxy_obj_p->target, prop_name_p, &target_desc);

  /* 12. */
  if (ECMA_IS_VALUE_ERROR (status))
  {
    ecma_free_value (trap_result);
    return status;
  }

  /* 13. */
  if (ecma_is_value_true (status))
  {
    ecma_value_t ret_value = ECMA_VALUE_EMPTY;

    if ((target_desc.flags & JERRY_PROP_IS_VALUE_DEFINED) && !(target_desc.flags & JERRY_PROP_IS_CONFIGURABLE)
        && !(target_desc.flags & JERRY_PROP_IS_WRITABLE) && !ecma_op_same_value (trap_result, target_desc.value))
    {
      ret_value = ecma_raise_type_error (ECMA_ERR_INCORRECT_RETURN_PROXY_GET_TRAP);
    }
    else if (!(target_desc.flags & JERRY_PROP_IS_CONFIGURABLE)
             && (target_desc.flags & (JERRY_PROP_IS_GET_DEFINED | JERRY_PROP_IS_SET_DEFINED))
             && target_desc.get_p == NULL && !ecma_is_value_undefined (trap_result))
    {
      ret_value = ecma_raise_type_error (ECMA_ERR_PROXY_PROPERTY_NOT_CONFIGURABLE_NOT_HAVE_GETTER);
    }

    ecma_free_property_descriptor (&target_desc);

    if (ECMA_IS_VALUE_ERROR (ret_value))
    {
      ecma_free_value (trap_result);

      return ret_value;
    }
  }

  /* 14. */
  return trap_result;
} /* ecma_proxy_object_get */