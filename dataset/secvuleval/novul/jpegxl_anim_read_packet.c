static int jpegxl_anim_read_packet(AVFormatContext *s, AVPacket *pkt)
{
    JXLAnimDemuxContext *ctx = s->priv_data;
    AVIOContext *pb = s->pb;
    int ret;
    int64_t size;
    size_t offset = 0;

    size = avio_size(pb);
    if (size < 0)
        return size;
    if (size > INT_MAX)
        return AVERROR(EDOM);
    if (size == 0)
        size = 4096;

    if (ctx->initial && size < ctx->initial->size)
        size = ctx->initial->size;

    ret = av_new_packet(pkt, size);
    if (ret < 0)
        return ret;

    if (ctx->initial) {
        offset = ctx->initial->size;
        memcpy(pkt->data, ctx->initial->data, offset);
        av_buffer_unref(&ctx->initial);
    }

    ret = avio_read(pb, pkt->data + offset, size - offset);
    if (ret < 0)
        return ret;
    if (ret < size - offset)
        pkt->size = ret + offset;

    return 0;
}