/*
 * LevelGuard Core
 * Copyright (c) 2025 VirtualSoundBox
 *
 * This software is released under the MIT License.
 * https://opensource.org/licenses/MIT
 */

#pragma once

#include "logger.hpp"
#include <obs-module.h>
#include <plugin-support.h>

namespace levelguard {
namespace core {

/**
 * OBS Logger
 *
 * ILogger を実装し、obs_log() に出力する。
 */
class ObsLogger : public ILogger {
public:
    void log_lifecycle(
        CoreEvent event,
        CoreState from_state,
        CoreState to_state
    ) override {
        obs_log(LOG_INFO, "[LevelGuard] lifecycle: %s -> %s (event=%d)",
                state_to_string(from_state),
                state_to_string(to_state),
                static_cast<int>(event));
    }

    void log_decision(
        LogLevel level,
        const std::string& decision_type,
        const std::string& reason
    ) override {
        int obs_level = (level == LogLevel::WARNING || level == LogLevel::ERR)
                        ? LOG_WARNING : LOG_INFO;
        obs_log(obs_level, "[LevelGuard] decision: %s - %s",
                decision_type.c_str(),
                reason.c_str());
    }

    void log_error(
        const std::string& error_message
    ) override {
        obs_log(LOG_ERROR, "[LevelGuard] error: %s",
                error_message.c_str());
    }

private:
    static const char* state_to_string(CoreState state) {
        switch (state) {
            case CoreState::IDLE:        return "IDLE";
            case CoreState::MONITORING:  return "MONITORING";
            case CoreState::INTERVENING: return "INTERVENING";
            case CoreState::SUSPENDED:   return "SUSPENDED";
            case CoreState::ERROR:       return "ERROR";
            default:                     return "UNKNOWN";
        }
    }
};

} // namespace core
} // namespace levelguard
