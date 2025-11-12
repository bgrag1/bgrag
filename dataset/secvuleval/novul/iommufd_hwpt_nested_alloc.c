static struct iommufd_hwpt_nested *
iommufd_hwpt_nested_alloc(struct iommufd_ctx *ictx,
			  struct iommufd_hwpt_paging *parent,
			  struct iommufd_device *idev, u32 flags,
			  const struct iommu_user_data *user_data)
{
	const struct iommu_ops *ops = dev_iommu_ops(idev->dev);
	struct iommufd_hwpt_nested *hwpt_nested;
	struct iommufd_hw_pagetable *hwpt;
	int rc;

	if ((flags & ~IOMMU_HWPT_FAULT_ID_VALID) ||
	    !user_data->len || !ops->domain_alloc_user)
		return ERR_PTR(-EOPNOTSUPP);
	if (parent->auto_domain || !parent->nest_parent)
		return ERR_PTR(-EINVAL);

	hwpt_nested = __iommufd_object_alloc(
		ictx, hwpt_nested, IOMMUFD_OBJ_HWPT_NESTED, common.obj);
	if (IS_ERR(hwpt_nested))
		return ERR_CAST(hwpt_nested);
	hwpt = &hwpt_nested->common;

	refcount_inc(&parent->common.obj.users);
	hwpt_nested->parent = parent;

	hwpt->domain = ops->domain_alloc_user(idev->dev,
					      flags & ~IOMMU_HWPT_FAULT_ID_VALID,
					      parent->common.domain, user_data);
	if (IS_ERR(hwpt->domain)) {
		rc = PTR_ERR(hwpt->domain);
		hwpt->domain = NULL;
		goto out_abort;
	}
	hwpt->domain->owner = ops;

	if (WARN_ON_ONCE(hwpt->domain->type != IOMMU_DOMAIN_NESTED ||
			 !hwpt->domain->ops->cache_invalidate_user)) {
		rc = -EINVAL;
		goto out_abort;
	}
	return hwpt_nested;

out_abort:
	iommufd_object_abort_and_destroy(ictx, &hwpt->obj);
	return ERR_PTR(rc);
}
