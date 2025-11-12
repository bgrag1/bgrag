STACK_OF(PKCS12_SAFEBAG) *PKCS12_unpack_p7data(PKCS7 *p7)
{
    if (!PKCS7_type_is_data(p7)) {
        ERR_raise(ERR_LIB_PKCS12, PKCS12_R_CONTENT_TYPE_NOT_DATA);
        return NULL;
    }

    if (p7->d.data == NULL) {
        ERR_raise(ERR_LIB_PKCS12, PKCS12_R_DECODE_ERROR);
        return NULL;
    }

    return ASN1_item_unpack_ex(p7->d.data, ASN1_ITEM_rptr(PKCS12_SAFEBAGS),
                               ossl_pkcs7_ctx_get0_libctx(&p7->ctx),
                               ossl_pkcs7_ctx_get0_propq(&p7->ctx));
}STACK_OF(PKCS12_SAFEBAG) *PKCS12_unpack_p7data(PKCS7 *p7)
{
    if (!PKCS7_type_is_data(p7)) {
        ERR_raise(ERR_LIB_PKCS12, PKCS12_R_CONTENT_TYPE_NOT_DATA);
        return NULL;
    }

    if (p7->d.data == NULL) {
        ERR_raise(ERR_LIB_PKCS12, PKCS12_R_DECODE_ERROR);
        return NULL;
    }

    return ASN1_item_unpack(p7->d.data, ASN1_ITEM_rptr(PKCS12_SAFEBAGS));
}STACK_OF(PKCS12_SAFEBAG) *PKCS12_unpack_p7data(PKCS7 *p7)
{
    if (!PKCS7_type_is_data(p7)) {
        ERR_raise(ERR_LIB_PKCS12, PKCS12_R_CONTENT_TYPE_NOT_DATA);
        return NULL;
    }

    if (p7->d.data == NULL) {
        ERR_raise(ERR_LIB_PKCS12, PKCS12_R_DECODE_ERROR);
        return NULL;
    }

    return ASN1_item_unpack(p7->d.data, ASN1_ITEM_rptr(PKCS12_SAFEBAGS));
}