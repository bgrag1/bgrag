void poly_frommsg(poly *r, const uint8_t msg[KYBER_INDCPA_MSGBYTES])
{
  unsigned int i,j;

#if (KYBER_INDCPA_MSGBYTES != KYBER_N/8)
#error "KYBER_INDCPA_MSGBYTES must be equal to KYBER_N/8 bytes!"
#endif

  for(i=0;i<KYBER_N/8;i++) {
    for(j=0;j<8;j++) {
      r->coeffs[8*i+j] = 0;
      cmov_int16(r->coeffs+8*i+j, ((KYBER_Q+1)/2), (msg[i] >> j)&1);
    }
  }
}