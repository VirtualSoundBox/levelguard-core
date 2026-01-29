#pragma once

namespace levelguard::core {

struct CoreConfig {
    float sample_rate = 48000.0f;
    bool enabled = true;

    bool validate() const {
        return sample_rate == 44100.0f || sample_rate == 48000.0f || sample_rate == 96000.0f;
    }
};

} // namespace levelguard::core
