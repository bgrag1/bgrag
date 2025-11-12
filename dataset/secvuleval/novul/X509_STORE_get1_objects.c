X509_STORE_get1_objects(X509_STORE *store)
{
    STACK_OF(X509_OBJECT) *ret;
    if (!X509_STORE_lock(store)) {
        return NULL;
    }
    ret = sk_X509_OBJECT_deep_copy(X509_STORE_get0_objects(store),
                                   x509_object_dup, X509_OBJECT_free);
    X509_STORE_unlock(store);
    return ret;
}