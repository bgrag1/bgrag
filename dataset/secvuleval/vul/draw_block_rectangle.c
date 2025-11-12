static void draw_block_rectangle(uint8_t *buf, int sx, int sy, int w, int h, ptrdiff_t stride, int color)
{
    for (int x = sx; x < sx + w; x++)
        buf[x] = color;

    for (int y = sy; y < sy + h; y++) {
        buf[sx] = color;
        buf[sx + w - 1] = color;
        buf += stride;
    }

    for (int x = sx; x < sx + w; x++)
        buf[x] = color;
}