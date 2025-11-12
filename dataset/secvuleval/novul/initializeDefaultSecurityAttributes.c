initializeDefaultSecurityAttributes(
    PSECURITY_ATTRIBUTES *securityAttributes,
    struct _Py_SECURITY_ATTRIBUTE_DATA *data
) {
    assert(securityAttributes);
    assert(data);
    *securityAttributes = NULL;
    memset(data, 0, sizeof(*data));
    return 0;
}