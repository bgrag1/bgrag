int dtInsert(tid_t tid, struct inode *ip,
	 struct component_name * name, ino_t * fsn, struct btstack * btstack)
{
	int rc = 0;
	struct metapage *mp;	/* meta-page buffer */
	dtpage_t *p;		/* base B+-tree index page */
	s64 bn;
	int index;
	struct dtsplit split;	/* split information */
	ddata_t data;
	struct dt_lock *dtlck;
	int n;
	struct tlock *tlck;
	struct lv *lv;

	/*
	 *	retrieve search result
	 *
	 * dtSearch() returns (leaf page pinned, index at which to insert).
	 * n.b. dtSearch() may return index of (maxindex + 1) of
	 * the full page.
	 */
	DT_GETSEARCH(ip, btstack->top, bn, mp, p, index);

	/*
	 *	insert entry for new key
	 */
	if (DO_INDEX(ip)) {
		if (JFS_IP(ip)->next_index == DIREND) {
			DT_PUTPAGE(mp);
			return -EMLINK;
		}
		n = NDTLEAF(name->namlen);
		data.leaf.tid = tid;
		data.leaf.ip = ip;
	} else {
		n = NDTLEAF_LEGACY(name->namlen);
		data.leaf.ip = NULL;	/* signifies legacy directory format */
	}
	data.leaf.ino = *fsn;

	/*
	 *	leaf page does not have enough room for new entry:
	 *
	 *	extend/split the leaf page;
	 *
	 * dtSplitUp() will insert the entry and unpin the leaf page.
	 */
	if (n > p->header.freecnt) {
		split.mp = mp;
		split.index = index;
		split.nslot = n;
		split.key = name;
		split.data = &data;
		rc = dtSplitUp(tid, ip, &split, btstack);
		return rc;
	}

	/*
	 *	leaf page does have enough room for new entry:
	 *
	 *	insert the new data entry into the leaf page;
	 */
	BT_MARK_DIRTY(mp, ip);
	/*
	 * acquire a transaction lock on the leaf page
	 */
	tlck = txLock(tid, ip, mp, tlckDTREE | tlckENTRY);
	dtlck = (struct dt_lock *) & tlck->lock;
	ASSERT(dtlck->index == 0);
	lv = & dtlck->lv[0];

	/* linelock header */
	lv->offset = 0;
	lv->length = 1;
	dtlck->index++;

	dtInsertEntry(p, index, name, &data, &dtlck);

	/* linelock stbl of non-root leaf page */
	if (!(p->header.flag & BT_ROOT)) {
		if (dtlck->index >= dtlck->maxcnt)
			dtlck = (struct dt_lock *) txLinelock(dtlck);
		lv = & dtlck->lv[dtlck->index];
		n = index >> L2DTSLOTSIZE;
		lv->offset = p->header.stblindex + n;
		lv->length =
		    ((p->header.nextindex - 1) >> L2DTSLOTSIZE) - n + 1;
		dtlck->index++;
	}

	/* unpin the leaf page */
	DT_PUTPAGE(mp);

	return 0;
}
