STACK_OF(PKCS12_SAFEBAG) *PKCS12_unpack_p7encdata(PKCS7 *p7, const char *pass,
                                                  int passlen)
{
    if (!PKCS7_type_is_encrypted(p7))
        return NULL;

    if (p7->d.encrypted == NULL) {
        ERR_raise(ERR_LIB_PKCS12, PKCS12_R_DECODE_ERROR);
        return NULL;
    }

    return PKCS12_item_decrypt_d2i_ex(p7->d.encrypted->enc_data->algorithm,
                                   ASN1_ITEM_rptr(PKCS12_SAFEBAGS),
                                   pass, passlen,
                                   p7->d.encrypted->enc_data->enc_data, 1,
                                   p7->ctx.libctx, p7->ctx.propq);
}STACK_OF(PKCS12_SAFEBAG) *PKCS12_unpack_p7encdata(PKCS7 *p7, const char *pass,
                                                  int passlen)
{
    if (!PKCS7_type_is_encrypted(p7))
        return NULL;

    if (p7->d.encrypted == NULL) {
        ERR_raise(ERR_LIB_PKCS12, PKCS12_R_DECODE_ERROR);
        return NULL;
    }

    return PKCS12_item_decrypt_d2i_ex(p7->d.encrypted->enc_data->algorithm,
                                   ASN1_ITEM_rptr(PKCS12_SAFEBAGS),
                                   pass, passlen,
                                   p7->d.encrypted->enc_data->enc_data, 1,
                                   p7->ctx.libctx, p7->ctx.propq);
}STACK_OF(PKCS12_SAFEBAG) *PKCS12_unpack_p7encdata(PKCS7 *p7, const char *pass,
                                                  int passlen)
{
    if (!PKCS7_type_is_encrypted(p7))
        return NULL;

    if (p7->d.encrypted == NULL) {
        ERR_raise(ERR_LIB_PKCS12, PKCS12_R_DECODE_ERROR);
        return NULL;
    }

    return PKCS12_item_decrypt_d2i_ex(p7->d.encrypted->enc_data->algorithm,
                                   ASN1_ITEM_rptr(PKCS12_SAFEBAGS),
                                   pass, passlen,
                                   p7->d.encrypted->enc_data->enc_data, 1,
                                   p7->ctx.libctx, p7->ctx.propq);
}