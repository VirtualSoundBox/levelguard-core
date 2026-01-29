/*
 * LevelGuard Core
 * Copyright (c) 2025 VirtualSoundBox
 *
 * This software is released under the MIT License.
 * https://opensource.org/licenses/MIT
 */

#include <obs-module.h>
#include <plugin-support.h>
#include <media-io/audio-io.h>

#include "core/core_config.hpp"
#include "core/core_interface.hpp"

#include <memory>

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE(PLUGIN_NAME, "ja-JP")

// =============================================================================
// フィルターコンテキスト
// =============================================================================

struct levelguard_filter_data {
	obs_source_t *source;
	std::unique_ptr<levelguard::core::CoreInterface> core;
};

// =============================================================================
// フィルターコールバック
// =============================================================================

static const char *levelguard_filter_get_name(void *)
{
	return "LevelGuard Core";
}

static void *levelguard_filter_create(obs_data_t *settings, obs_source_t *source)
{
	auto *filter = new levelguard_filter_data();
	filter->source = source;

	// OBS のサンプルレートを取得
	audio_t *audio = obs_get_audio();
	const struct audio_output_info *aoi = audio_output_get_info(audio);
	float sample_rate = aoi ? static_cast<float>(aoi->samples_per_sec) : 48000.0f;

	// CoreConfig を生成
	levelguard::core::CoreConfig config;
	config.sample_rate = sample_rate;
	config.enabled = obs_data_get_bool(settings, "enabled");

	// CoreInterface を生成
	filter->core = std::make_unique<levelguard::core::CoreInterface>(config);

	// 監視開始
	filter->core->start_monitor();

	obs_log(LOG_INFO, "LevelGuard filter created (sample_rate=%.0f)", sample_rate);

	return filter;
}

static void levelguard_filter_destroy(void *data)
{
	auto *filter = static_cast<levelguard_filter_data *>(data);

	if (filter->core) {
		filter->core->stop_monitor();
	}

	obs_log(LOG_INFO, "LevelGuard filter destroyed");

	delete filter;
}

static struct obs_audio_data *levelguard_filter_audio(void *data, struct obs_audio_data *audio)
{
	auto *filter = static_cast<levelguard_filter_data *>(data);

	if (!filter->core || !audio) {
		return audio;
	}

	float *left = reinterpret_cast<float *>(audio->data[0]);
	float *right = reinterpret_cast<float *>(audio->data[1]);

	// モノラルの場合は left のみ処理
	if (!right) {
		for (uint32_t i = 0; i < audio->frames; i++) {
			auto [out_l, out_r] = filter->core->process_audio(left[i], left[i]);
			left[i] = out_l;
		}
	} else {
		for (uint32_t i = 0; i < audio->frames; i++) {
			auto [out_l, out_r] = filter->core->process_audio(left[i], right[i]);
			left[i] = out_l;
			right[i] = out_r;
		}
	}

	return audio;
}

// =============================================================================
// デフォルト設定
// =============================================================================

static void levelguard_filter_get_defaults(obs_data_t *settings)
{
	obs_data_set_default_bool(settings, "enabled", true);
}

// =============================================================================
// フィルター登録
// =============================================================================

static struct obs_source_info levelguard_filter_info = {};

const char *obs_module_description(void)
{
	return "LevelGuard Core Module";
}

bool obs_module_load(void)
{
	levelguard_filter_info.id = "levelguard_core_filter";
	levelguard_filter_info.type = OBS_SOURCE_TYPE_FILTER;
	levelguard_filter_info.output_flags = OBS_SOURCE_AUDIO;
	levelguard_filter_info.get_name = levelguard_filter_get_name;
	levelguard_filter_info.create = levelguard_filter_create;
	levelguard_filter_info.destroy = levelguard_filter_destroy;
	levelguard_filter_info.filter_audio = levelguard_filter_audio;
	levelguard_filter_info.get_defaults = levelguard_filter_get_defaults;

	obs_register_source(&levelguard_filter_info);

	obs_log(LOG_INFO, "plugin loaded successfully (version %s)", PLUGIN_VERSION);
	return true;
}

void obs_module_unload(void)
{
	obs_log(LOG_INFO, "plugin unloaded");
}
