static int snd_soc_dai_link_event(struct snd_soc_dapm_widget *w,
				  struct snd_kcontrol *kcontrol, int event)
{
	struct snd_soc_dapm_path *path;
	struct snd_soc_dai *source, *sink;
	struct snd_pcm_substream *substream = w->priv;
	int ret = 0, saved_stream = substream->stream;

	if (WARN_ON(list_empty(&w->edges[SND_SOC_DAPM_DIR_OUT]) ||
		    list_empty(&w->edges[SND_SOC_DAPM_DIR_IN])))
		return -EINVAL;

	switch (event) {
	case SND_SOC_DAPM_PRE_PMU:
		ret = snd_soc_dai_link_event_pre_pmu(w, substream);
		if (ret < 0)
			goto out;

		break;

	case SND_SOC_DAPM_POST_PMU:
		snd_soc_dapm_widget_for_each_sink_path(w, path) {
			sink = path->sink->priv;

			snd_soc_dai_digital_mute(sink, 0, SNDRV_PCM_STREAM_PLAYBACK);
			ret = 0;
		}
		break;

	case SND_SOC_DAPM_PRE_PMD:
		snd_soc_dapm_widget_for_each_sink_path(w, path) {
			sink = path->sink->priv;

			snd_soc_dai_digital_mute(sink, 1, SNDRV_PCM_STREAM_PLAYBACK);
			ret = 0;
		}

		substream->stream = SNDRV_PCM_STREAM_CAPTURE;
		snd_soc_dapm_widget_for_each_source_path(w, path) {
			source = path->source->priv;
			snd_soc_dai_hw_free(source, substream, 0);
		}

		substream->stream = SNDRV_PCM_STREAM_PLAYBACK;
		snd_soc_dapm_widget_for_each_sink_path(w, path) {
			sink = path->sink->priv;
			snd_soc_dai_hw_free(sink, substream, 0);
		}

		substream->stream = SNDRV_PCM_STREAM_CAPTURE;
		snd_soc_dapm_widget_for_each_source_path(w, path) {
			source = path->source->priv;
			snd_soc_dai_deactivate(source, substream->stream);
			snd_soc_dai_shutdown(source, substream, 0);
		}

		substream->stream = SNDRV_PCM_STREAM_PLAYBACK;
		snd_soc_dapm_widget_for_each_sink_path(w, path) {
			sink = path->sink->priv;
			snd_soc_dai_deactivate(sink, substream->stream);
			snd_soc_dai_shutdown(sink, substream, 0);
		}
		break;

	case SND_SOC_DAPM_POST_PMD:
		kfree(substream->runtime);
		break;

	default:
		WARN(1, "Unknown event %d\n", event);
		ret = -EINVAL;
	}

out:
	/* Restore the substream direction */
	substream->stream = saved_stream;
	return ret;
}
