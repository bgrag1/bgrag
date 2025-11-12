protoop_arg_t get_checksum_length(picoquic_cnx_t *cnx)
{
    int is_cleartext_mode = (int) cnx->protoop_inputv[0];
    uint32_t ret = 16;

    if (is_cleartext_mode || cnx->crypto_context[2].aead_encrypt == NULL) {
        ret = picoquic_aead_get_checksum_length(cnx->crypto_context[0].aead_encrypt);
    } else {
        ret = picoquic_aead_get_checksum_length(cnx->crypto_context[2].aead_encrypt);
    }

    return (protoop_arg_t) ret;
}