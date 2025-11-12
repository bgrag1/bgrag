static unsigned long shift_and_mask(unsigned long v, u32 shift, u32 bits)
{
	return (v >> shift) & ((1 << bits) - 1);
}
