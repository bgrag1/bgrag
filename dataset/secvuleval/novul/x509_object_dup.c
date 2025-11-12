static X509_OBJECT *x509_object_dup(const X509_OBJECT *obj)
{
    int ok;
    X509_OBJECT *ret = X509_OBJECT_new();
    if (ret == NULL) {
        return NULL;
    }
    switch (X509_OBJECT_get_type(obj)) {
        case X509_LU_X509:
            ok = X509_OBJECT_set1_X509(ret, X509_OBJECT_get0_X509(obj));
            break;
        case X509_LU_CRL:
            /* X509_OBJECT_get0_X509_CRL was not const-correct prior to 3.0.*/
            ok = X509_OBJECT_set1_X509_CRL(
                ret, X509_OBJECT_get0_X509_CRL((X509_OBJECT *)obj));
            break;
        default:
            /* We cannot duplicate unrecognized types in a polyfill, but it is
             * safe to leave an empty object. The caller will ignore it. */
            ok = 1;
            break;
    }
    if (!ok) {
        X509_OBJECT_free(ret);
        return NULL;
    }
    return ret;
}