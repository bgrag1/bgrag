initializeMkdir700SecurityAttributes(
    PSECURITY_ATTRIBUTES *securityAttributes,
    struct _Py_SECURITY_ATTRIBUTE_DATA *data
) {
    assert(securityAttributes);
    assert(data);
    *securityAttributes = NULL;
    memset(data, 0, sizeof(*data));

    if (!InitializeSecurityDescriptor(&data->sd, SECURITY_DESCRIPTOR_REVISION)
        || !SetSecurityDescriptorGroup(&data->sd, NULL, TRUE)) {
        return GetLastError();
    }

    int use_alias = 0;
    DWORD cbSid = sizeof(data->sid);
    if (!CreateWellKnownSid(WinCreatorOwnerRightsSid, NULL, (PSID)data->sid, &cbSid)) {
        use_alias = 1;
    }

    data->securityAttributes.nLength = sizeof(SECURITY_ATTRIBUTES);
    data->ea[0].grfAccessPermissions = GENERIC_ALL;
    data->ea[0].grfAccessMode = SET_ACCESS;
    data->ea[0].grfInheritance = SUB_CONTAINERS_AND_OBJECTS_INHERIT;
    if (use_alias) {
        data->ea[0].Trustee.TrusteeForm = TRUSTEE_IS_NAME;
        data->ea[0].Trustee.TrusteeType = TRUSTEE_IS_ALIAS;
        data->ea[0].Trustee.ptstrName = L"CURRENT_USER";
    } else {
        data->ea[0].Trustee.TrusteeForm = TRUSTEE_IS_SID;
        data->ea[0].Trustee.TrusteeType = TRUSTEE_IS_WELL_KNOWN_GROUP;
        data->ea[0].Trustee.ptstrName = (LPWCH)(SID*)data->sid;
    }

    data->ea[1].grfAccessPermissions = GENERIC_ALL;
    data->ea[1].grfAccessMode = SET_ACCESS;
    data->ea[1].grfInheritance = SUB_CONTAINERS_AND_OBJECTS_INHERIT;
    data->ea[1].Trustee.TrusteeForm = TRUSTEE_IS_NAME;
    data->ea[1].Trustee.TrusteeType = TRUSTEE_IS_ALIAS;
    data->ea[1].Trustee.ptstrName = L"SYSTEM";

    data->ea[2].grfAccessPermissions = GENERIC_ALL;
    data->ea[2].grfAccessMode = SET_ACCESS;
    data->ea[2].grfInheritance = SUB_CONTAINERS_AND_OBJECTS_INHERIT;
    data->ea[2].Trustee.TrusteeForm = TRUSTEE_IS_NAME;
    data->ea[2].Trustee.TrusteeType = TRUSTEE_IS_ALIAS;
    data->ea[2].Trustee.ptstrName = L"ADMINISTRATORS";

    int r = SetEntriesInAclW(3, data->ea, NULL, &data->acl);
    if (r) {
        return r;
    }
    if (!SetSecurityDescriptorDacl(&data->sd, TRUE, data->acl, FALSE)) {
        return GetLastError();
    }
    data->securityAttributes.lpSecurityDescriptor = &data->sd;
    *securityAttributes = &data->securityAttributes;
    return 0;
}