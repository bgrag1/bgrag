STACK_OF(PKCS7) *PKCS12_unpack_authsafes(const PKCS12 *p12)
{
    STACK_OF(PKCS7) *p7s;
    PKCS7_CTX *p7ctx;
    PKCS7 *p7;
    int i;

    if (!PKCS7_type_is_data(p12->authsafes)) {
        ERR_raise(ERR_LIB_PKCS12, PKCS12_R_CONTENT_TYPE_NOT_DATA);
        return NULL;
    }

    if (p12->authsafes->d.data == NULL) {
        ERR_raise(ERR_LIB_PKCS12, PKCS12_R_DECODE_ERROR);
        return NULL;
    }

    p7ctx = &p12->authsafes->ctx;
    p7s = ASN1_item_unpack_ex(p12->authsafes->d.data,
                              ASN1_ITEM_rptr(PKCS12_AUTHSAFES),
                              ossl_pkcs7_ctx_get0_libctx(p7ctx),
                              ossl_pkcs7_ctx_get0_propq(p7ctx));
    if (p7s != NULL) {
        for (i = 0; i < sk_PKCS7_num(p7s); i++) {
            p7 = sk_PKCS7_value(p7s, i);
            if (!ossl_pkcs7_ctx_propagate(p12->authsafes, p7))
                goto err;
        }
    }
    return p7s;
err:
    sk_PKCS7_free(p7s);
    return NULL;
}STACK_OF(PKCS7) *PKCS12_unpack_authsafes(const PKCS12 *p12)
{
    STACK_OF(PKCS7) *p7s;
    PKCS7 *p7;
    int i;

    if (!PKCS7_type_is_data(p12->authsafes)) {
        ERR_raise(ERR_LIB_PKCS12, PKCS12_R_CONTENT_TYPE_NOT_DATA);
        return NULL;
    }

    if (p12->authsafes->d.data == NULL) {
        ERR_raise(ERR_LIB_PKCS12, PKCS12_R_DECODE_ERROR);
        return NULL;
    }

    p7s = ASN1_item_unpack(p12->authsafes->d.data,
                           ASN1_ITEM_rptr(PKCS12_AUTHSAFES));
    if (p7s != NULL) {
        for (i = 0; i < sk_PKCS7_num(p7s); i++) {
            p7 = sk_PKCS7_value(p7s, i);
            if (!ossl_pkcs7_ctx_propagate(p12->authsafes, p7))
                goto err;
        }
    }
    return p7s;
err:
    sk_PKCS7_free(p7s);
    return NULL;
}STACK_OF(PKCS7) *PKCS12_unpack_authsafes(const PKCS12 *p12)
{
    STACK_OF(PKCS7) *p7s;
    PKCS7 *p7;
    int i;

    if (!PKCS7_type_is_data(p12->authsafes)) {
        ERR_raise(ERR_LIB_PKCS12, PKCS12_R_CONTENT_TYPE_NOT_DATA);
        return NULL;
    }

    if (p12->authsafes->d.data == NULL) {
        ERR_raise(ERR_LIB_PKCS12, PKCS12_R_DECODE_ERROR);
        return NULL;
    }

    p7s = ASN1_item_unpack(p12->authsafes->d.data,
                           ASN1_ITEM_rptr(PKCS12_AUTHSAFES));
    if (p7s != NULL) {
        for (i = 0; i < sk_PKCS7_num(p7s); i++) {
            p7 = sk_PKCS7_value(p7s, i);
            if (!ossl_pkcs7_ctx_propagate(p12->authsafes, p7))
                goto err;
        }
    }
    return p7s;
err:
    sk_PKCS7_free(p7s);
    return NULL;
}