int SMIME_write_PKCS7(BIO *bio, PKCS7 *p7, BIO *data, int flags)
{
    STACK_OF(X509_ALGOR) *mdalgs;
    int ctype_nid = OBJ_obj2nid(p7->type);
    const PKCS7_CTX *ctx = ossl_pkcs7_get0_ctx(p7);

    if (ctype_nid == NID_pkcs7_signed) {
        if (p7->d.sign == NULL)
            return 0;
        mdalgs = p7->d.sign->md_algs;
    } else {
        mdalgs = NULL;
    }

    flags ^= SMIME_OLDMIME;

    return SMIME_write_ASN1_ex(bio, (ASN1_VALUE *)p7, data, flags, ctype_nid,
                               NID_undef, mdalgs, ASN1_ITEM_rptr(PKCS7),
                               ossl_pkcs7_ctx_get0_libctx(ctx),
                               ossl_pkcs7_ctx_get0_propq(ctx));
}int SMIME_write_PKCS7(BIO *bio, PKCS7 *p7, BIO *data, int flags)
{
    STACK_OF(X509_ALGOR) *mdalgs;
    int ctype_nid = OBJ_obj2nid(p7->type);
    const PKCS7_CTX *ctx = ossl_pkcs7_get0_ctx(p7);

    if (ctype_nid == NID_pkcs7_signed) {
        if (p7->d.sign == NULL)
            return 0;
        mdalgs = p7->d.sign->md_algs;
    } else {
        mdalgs = NULL;
    }

    flags ^= SMIME_OLDMIME;

    return SMIME_write_ASN1_ex(bio, (ASN1_VALUE *)p7, data, flags, ctype_nid,
                               NID_undef, mdalgs, ASN1_ITEM_rptr(PKCS7),
                               ossl_pkcs7_ctx_get0_libctx(ctx),
                               ossl_pkcs7_ctx_get0_propq(ctx));
}int SMIME_write_PKCS7(BIO *bio, PKCS7 *p7, BIO *data, int flags)
{
    STACK_OF(X509_ALGOR) *mdalgs;
    int ctype_nid = OBJ_obj2nid(p7->type);
    const PKCS7_CTX *ctx = ossl_pkcs7_get0_ctx(p7);

    if (ctype_nid == NID_pkcs7_signed) {
        if (p7->d.sign == NULL)
            return 0;
        mdalgs = p7->d.sign->md_algs;
    } else {
        mdalgs = NULL;
    }

    flags ^= SMIME_OLDMIME;

    return SMIME_write_ASN1_ex(bio, (ASN1_VALUE *)p7, data, flags, ctype_nid,
                               NID_undef, mdalgs, ASN1_ITEM_rptr(PKCS7),
                               ossl_pkcs7_ctx_get0_libctx(ctx),
                               ossl_pkcs7_ctx_get0_propq(ctx));
}