_ssl__SSLContext_cert_store_stats_impl(PySSLContext *self)
/*[clinic end generated code: output=5f356f4d9cca874d input=eb40dd0f6d0e40cf]*/
{
    X509_STORE *store;
    STACK_OF(X509_OBJECT) *objs;
    X509_OBJECT *obj;
    int x509 = 0, crl = 0, ca = 0, i;

    store = SSL_CTX_get_cert_store(self->ctx);
    objs = X509_STORE_get1_objects(store);
    if (objs == NULL) {
        PyErr_SetString(PyExc_MemoryError, "failed to query cert store");
        return NULL;
    }

    for (i = 0; i < sk_X509_OBJECT_num(objs); i++) {
        obj = sk_X509_OBJECT_value(objs, i);
        switch (X509_OBJECT_get_type(obj)) {
            case X509_LU_X509:
                x509++;
                if (X509_check_ca(X509_OBJECT_get0_X509(obj))) {
                    ca++;
                }
                break;
            case X509_LU_CRL:
                crl++;
                break;
            default:
                /* Ignore unrecognized types. */
                break;
        }
    }
    sk_X509_OBJECT_pop_free(objs, X509_OBJECT_free);
    return Py_BuildValue("{sisisi}", "x509", x509, "crl", crl,
        "x509_ca", ca);
}