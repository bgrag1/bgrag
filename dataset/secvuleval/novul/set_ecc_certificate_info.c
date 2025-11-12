static int set_ecc_certificate_info(X509_SCHANNEL_HANDLE_DATA* x509_handle, unsigned char* x509privatekeyBlob)
{
    int result;
#if _MSC_VER > 1500
    BCRYPT_ECCKEY_BLOB* pKeyBlob;
    SECURITY_STATUS status;
    CRYPT_BIT_BLOB* pPubKeyBlob = &x509_handle->x509certificate_context->pCertInfo->SubjectPublicKeyInfo.PublicKey;
    CRYPT_ECC_PRIVATE_KEY_INFO* pPrivKeyInfo = (CRYPT_ECC_PRIVATE_KEY_INFO*)x509privatekeyBlob;
    DWORD pubSize = pPubKeyBlob->cbData - 1;
    DWORD privSize = pPrivKeyInfo->PrivateKey.cbData;
    size_t keyBlobSize = safe_add_size_t(safe_add_size_t(sizeof(BCRYPT_ECCKEY_BLOB), pubSize), privSize);
    BYTE* pubKeyBuf = pPubKeyBlob->pbData + 1;
    BYTE* privKeyBuf = pPrivKeyInfo->PrivateKey.pbData;

    if (keyBlobSize == SIZE_MAX ||
        (pKeyBlob = (BCRYPT_ECCKEY_BLOB*)malloc(keyBlobSize)) == NULL)
    {
        /*Codes_SRS_X509_SCHANNEL_02_010: [ Otherwise, x509_schannel_create shall fail and return a NULL X509_SCHANNEL_HANDLE. ]*/
        LogError("Failed to malloc NCrypt private key blob, size:%zu", keyBlobSize);
        result = MU_FAILURE;
    }
    else
    {
        pKeyBlob->dwMagic = privSize == ECC_256_MAGIC_NUMBER ? BCRYPT_ECDSA_PRIVATE_P256_MAGIC
            : privSize == ECC_384_MAGIC_NUMBER ? BCRYPT_ECDSA_PRIVATE_P384_MAGIC
            : BCRYPT_ECDSA_PRIVATE_P521_MAGIC;
        pKeyBlob->cbKey = privSize;
        memcpy((BYTE*)(pKeyBlob + 1), pubKeyBuf, pubSize);
        memcpy((BYTE*)(pKeyBlob + 1) + pubSize, privKeyBuf, privSize);

        /* Codes_SRS_X509_SCHANNEL_02_005: [ x509_schannel_create shall call CryptAcquireContext. ] */
        /* at this moment, both the private key and the certificate are decoded for further usage */
        /* NOTE: As no WinCrypt key storage provider supports ECC keys, NCrypt is used instead */
        status = NCryptOpenStorageProvider(&x509_handle->hProv, MS_KEY_STORAGE_PROVIDER, 0);
        if (status != ERROR_SUCCESS)
        {
            /* Codes_SRS_X509_SCHANNEL_02_010: [ Otherwise, x509_schannel_create shall fail and return a NULL X509_SCHANNEL_HANDLE. ]*/
            LogError("NCryptOpenStorageProvider failed with error 0x%08X", status);
            result = MU_FAILURE;
        }
        else
        {
            SECURITY_STATUS status2;
            NCryptBuffer ncBuf = { sizeof(KEY_NAME), NCRYPTBUFFER_PKCS_KEY_NAME, KEY_NAME };
            NCryptBufferDesc ncBufDesc;
            ncBufDesc.ulVersion = 0;
            ncBufDesc.cBuffers = 1;
            ncBufDesc.pBuffers = &ncBuf;

            CRYPT_KEY_PROV_INFO keyProvInfo = { KEY_NAME, MS_KEY_STORAGE_PROVIDER, 0, 0, 0, NULL, 0 };

            /*Codes_SRS_X509_SCHANNEL_02_006: [ x509_schannel_create shall import the private key by calling CryptImportKey. ] */
            /*NOTE: As no WinCrypt key storage provider supports ECC keys, NCrypt is used instead*/
            status = NCryptImportKey(x509_handle->hProv, 0, BCRYPT_ECCPRIVATE_BLOB, &ncBufDesc, &x509_handle->x509hcryptkey, (BYTE*)pKeyBlob, (DWORD)keyBlobSize, NCRYPT_OVERWRITE_KEY_FLAG);
            if (status == ERROR_SUCCESS)
            {
                status2 = NCryptFreeObject(x509_handle->x509hcryptkey);
                if (status2 != ERROR_SUCCESS)
                {
                    LogError("NCryptFreeObject for key handle failed with error 0x%08X", status2);
                }
                else
                {
                    x509_handle->x509hcryptkey = 0;
                }
            }

            status2 = NCryptFreeObject(x509_handle->hProv);
            if (status2 != ERROR_SUCCESS)
            {
                LogError("NCryptFreeObject for provider handle failed with error 0x%08X", status2);
            }
            else
            {
                x509_handle->hProv = 0;
            }

            if (status != ERROR_SUCCESS)
            {
                /*Codes_SRS_X509_SCHANNEL_02_010: [ Otherwise, x509_schannel_create shall fail and return a NULL X509_SCHANNEL_HANDLE. ]*/
                LogError("NCryptImportKey failed with error 0x%08X", status);
                result = MU_FAILURE;
            }
            else if (!CertSetCertificateContextProperty(x509_handle->x509certificate_context, CERT_KEY_PROV_INFO_PROP_ID, 0, &keyProvInfo))
            {
                /*Codes_SRS_X509_SCHANNEL_02_010: [ Otherwise, x509_schannel_create shall fail and return a NULL X509_SCHANNEL_HANDLE. ]*/
                LogErrorWinHTTPWithGetLastErrorAsString("CertSetCertificateContextProperty failed to set NCrypt key handle property");
                result = MU_FAILURE;
            }
            else
            {
                result = 0;
            }
        }
        free(pKeyBlob);
    }
#else
    (void)x509_handle;
    (void)x509privatekeyBlob;
    LogError("SChannel ECC is not supported in this compliation");
    result = MU_FAILURE;
#endif
    return result;
}