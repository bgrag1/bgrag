_ssl__SSLContext_get_ca_certs_impl(PySSLContext *self, int binary_form)
/*[clinic end generated code: output=0d58f148f37e2938 input=6887b5a09b7f9076]*/
{
    X509_STORE *store;
    STACK_OF(X509_OBJECT) *objs;
    PyObject *ci = NULL, *rlist = NULL;
    int i;

    if ((rlist = PyList_New(0)) == NULL) {
        return NULL;
    }

    store = SSL_CTX_get_cert_store(self->ctx);
    objs = X509_STORE_get1_objects(store);
    if (objs == NULL) {
        PyErr_SetString(PyExc_MemoryError, "failed to query cert store");
        goto error;
    }

    for (i = 0; i < sk_X509_OBJECT_num(objs); i++) {
        X509_OBJECT *obj;
        X509 *cert;

        obj = sk_X509_OBJECT_value(objs, i);
        if (X509_OBJECT_get_type(obj) != X509_LU_X509) {
            /* not a x509 cert */
            continue;
        }
        /* CA for any purpose */
        cert = X509_OBJECT_get0_X509(obj);
        if (!X509_check_ca(cert)) {
            continue;
        }
        if (binary_form) {
            ci = _certificate_to_der(get_state_ctx(self), cert);
        } else {
            ci = _decode_certificate(get_state_ctx(self), cert);
        }
        if (ci == NULL) {
            goto error;
        }
        if (PyList_Append(rlist, ci) == -1) {
            goto error;
        }
        Py_CLEAR(ci);
    }
    sk_X509_OBJECT_pop_free(objs, X509_OBJECT_free);
    return rlist;

  error:
    sk_X509_OBJECT_pop_free(objs, X509_OBJECT_free);
    Py_XDECREF(ci);
    Py_XDECREF(rlist);
    return NULL;
}