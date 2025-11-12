CBORTag_hash(CBORTagObject *self)
{
    if (!_CBOR2_thread_locals && _CBOR2_init_thread_locals() == -1)
        return -1;

    Py_hash_t ret = -1;
    PyObject *running_hashes = NULL;
    PyObject *tmp = NULL;
    PyObject *self_id = PyLong_FromVoidPtr(self);
    if (!self_id)
        goto exit;

    running_hashes = PyObject_GetAttrString(_CBOR2_thread_locals, "running_hashes");
    if (!running_hashes) {
        PyErr_Clear();
        running_hashes = PySet_New(NULL);
        if (PyObject_SetAttrString(_CBOR2_thread_locals, "running_hashes", running_hashes) == -1)
            goto exit;
    } else {
        // Check if __hash__() is already being run against this object in this thread
        switch (PySet_Contains(running_hashes, self_id)) {
            case -1:  // error
                goto exit;
            case 1:  // this object is already in the set
                PyErr_SetString(
                    PyExc_RuntimeError,
                    "This CBORTag is not hashable because it contains a reference to itself"
                );
                goto exit;
        }
    }

    // Add id(self) to thread_locals.running_hashes
    if (PySet_Add(running_hashes, self_id) == -1)
        goto exit;

    tmp = Py_BuildValue("(KO)", self->tag, self->value);
    if (!tmp)
        goto exit;

    ret = PyObject_Hash(tmp);

    // Remove id(self) from thread_locals.running_hashes
    if (PySet_Discard(running_hashes, self_id) == -1) {
        ret = -1;
        goto exit;
    }

    // Check how many more references there are in running_hashes
    Py_ssize_t length = PySequence_Length(running_hashes);
    if (length == -1) {
        ret = -1;
        goto exit;
    }

    // If this was the last reference, delete running_hashes from the thread-local variable
    if (length == 0 && PyObject_DelAttrString(_CBOR2_thread_locals, "running_hashes") == -1) {
        ret = -1;
        goto exit;
    }
exit:
    Py_CLEAR(self_id);
    Py_CLEAR(running_hashes);
    Py_CLEAR(tmp);
    return ret;
}