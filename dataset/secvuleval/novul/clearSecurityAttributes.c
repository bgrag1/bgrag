clearSecurityAttributes(
    PSECURITY_ATTRIBUTES *securityAttributes,
    struct _Py_SECURITY_ATTRIBUTE_DATA *data
) {
    assert(securityAttributes);
    assert(data);
    *securityAttributes = NULL;
    if (data->acl) {
        if (LocalFree((void *)data->acl)) {
            return GetLastError();
        }
    }
    return 0;
}