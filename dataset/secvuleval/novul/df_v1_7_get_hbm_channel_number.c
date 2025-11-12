static u32 df_v1_7_get_hbm_channel_number(struct amdgpu_device *adev)
{
	int fb_channel_number;

	fb_channel_number = adev->df.funcs->get_fb_channel_number(adev);
	if (fb_channel_number >= ARRAY_SIZE(df_v1_7_channel_number))
		fb_channel_number = 0;

	return df_v1_7_channel_number[fb_channel_number];
}
