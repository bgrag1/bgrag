void cmov_int16(int16_t *r, int16_t v, uint16_t b)
{
  b = -b;
  *r ^= b & ((*r) ^ v);
}