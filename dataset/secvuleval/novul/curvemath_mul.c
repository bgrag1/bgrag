static PyObject * curvemath_mul(PyObject *self, PyObject *args) {
    char * x, * y, * d, * p, * a, * b, * q, * gx, * gy;

    if (!PyArg_ParseTuple(args, "sssssssss", &x, &y, &d, &p, &a, &b, &q, &gx, &gy)) {
        return NULL;
    }

    PointZZ_p result;
    mpz_inits(result.x, result.y, NULL);
    mpz_t scalar;
    mpz_init_set_str(scalar, d, 10);
    CurveZZ_p * curve = buildCurveZZ_p(p, a, b, q, gx, gy, 10);;

    PointZZ_p * point = buildPointZZ_p(x, y, 10);
    pointZZ_pMul(&result, point, scalar, curve);
    destroyPointZZ_p(point);
    destroyCurveZZ_p(curve);

    char * resultX = mpz_get_str(NULL, 10, result.x);
    char * resultY = mpz_get_str(NULL, 10, result.y);
    mpz_clears(result.x, result.y, scalar, NULL);

    PyObject * ret = Py_BuildValue("ss", resultX, resultY);
    free(resultX);
    free(resultY);
    return ret;
}