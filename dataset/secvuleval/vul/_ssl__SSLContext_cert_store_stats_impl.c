_ssl__SSLContext_cert_store_stats_impl(PySSLContext *self)
/*[clinic end generated code: output=5f356f4d9cca874d input=eb40dd0f6d0e40cf]*/
{
    X509_STORE *store;
    STACK_OF(X509_OBJECT) *objs;
    X509_OBJECT *obj;
    int x509 = 0, crl = 0, ca = 0, i;

    store = SSL_CTX_get_cert_store(self->ctx);
    objs = X509_STORE_get0_objects(store);
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
                /* Ignore X509_LU_FAIL, X509_LU_RETRY, X509_LU_PKEY.
                 * As far as I can tell they are internal states and never
                 * stored in a cert store */
                break;
        }
    }
    return Py_BuildValue("{sisisi}", "x509", x509, "crl", crl,
        "x509_ca", ca);
}