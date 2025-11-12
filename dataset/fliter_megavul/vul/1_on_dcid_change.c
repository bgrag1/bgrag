static int
on_dcid_change (struct ietf_full_conn *conn, const lsquic_cid_t *dcid_in)
{
    struct lsquic_conn *const lconn = &conn->ifc_conn;  /* Shorthand */
    struct conn_cid_elem *cce;

    LSQ_DEBUG("peer switched its DCID, attempt to switch own SCID");

    for (cce = lconn->cn_cces; cce < END_OF_CCES(lconn); ++cce)
        if (cce - lconn->cn_cces != lconn->cn_cur_cce_idx
                && (lconn->cn_cces_mask & (1 << (cce - lconn->cn_cces)))
                    && LSQUIC_CIDS_EQ(&cce->cce_cid, dcid_in))
            break;

    if (cce >= END_OF_CCES(lconn))
    {
        ABORT_WARN("DCID not found");
        return -1;
    }

    if (cce->cce_flags & CCE_USED)
    {
        LSQ_DEBUGC("Current CID: %"CID_FMT, CID_BITS(CN_SCID(lconn)));
        LSQ_DEBUGC("DCID %"CID_FMT" has been used, not switching",
                                                            CID_BITS(dcid_in));
        return 0;
    }

    cce->cce_flags |= CCE_USED;
    lconn->cn_cur_cce_idx = cce - lconn->cn_cces;
    LSQ_DEBUGC("%s: set SCID to %"CID_FMT, __func__, CID_BITS(CN_SCID(lconn)));
    LOG_SCIDS(conn);

    return 0;
}