static int v4l2_async_create_ancillary_links(struct v4l2_async_notifier *n,
					     struct v4l2_subdev *sd)
{
// #if IS_ENABLED(CONFIG_MEDIA_CONTROLLER)
	struct media_link *link;

	if (sd->entity.function != MEDIA_ENT_F_LENS &&
	    sd->entity.function != MEDIA_ENT_F_FLASH)
		return 0;

	link = media_create_ancillary_link(&n->sd->entity, &sd->entity);

	return IS_ERR(link) ? PTR_ERR(link) : 0;
#else
	return 0;
#endif
}
