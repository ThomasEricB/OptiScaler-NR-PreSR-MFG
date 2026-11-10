#pragma once
#include <Config.h>
#include "DlssNr_ModelParameters.h"
#include <algorithm>
#include <cmath>

namespace DlssNr::Profiles
{
inline ModelSettings PassSettings(const Config& cfg, unsigned int pass)
{
    ModelSettings result { cfg.DlssNrPreset.value_or_default(),
                           cfg.DlssNrStyle.value_or_default(),
                           cfg.DlssNrIntensity.value_or_default(),
                           cfg.DlssNrLocalStructure.value_or_default(),
                           pass == 0 ? cfg.DlssNrLocalTone.value_or_default() : 0.0f,
                           cfg.DlssNrSkinStructure.value_or_default(),
                           cfg.DlssNrAutoMask.value_or_default(),
                           cfg.DlssNrMaskBlendMode,
                           cfg.DlssNrUiCorrection.value_or_default() };
    if (pass > 0 && pass <= std::size(cfg.DlssNrPassOverrides))
    {
        const auto& extra = cfg.DlssNrPassOverrides[pass - 1];
        result.preset = extra.preset.value_or(result.preset);
        result.style = extra.style.value_or(result.style);
        result.intensity = extra.intensity.value_or(result.intensity);
        result.localStructure = extra.structure.value_or(result.localStructure);
        result.localTone = extra.tone.value_or(result.localTone);
        result.skinStructure = extra.skin.value_or(result.skinStructure);
        result.autoMask = extra.autoMask.value_or(result.autoMask);
        if (extra.maskBlendMode.has_value())
            result.maskBlendMode = extra.maskBlendMode.value();
        result.uiCorrection = extra.uiCorrection.value_or(result.uiCorrection);
    }
    result.preset = std::min(result.preset, 3u);
    result.style = std::min(result.style, 2u);
    if (result.maskBlendMode.has_value())
        result.maskBlendMode = std::min(*result.maskBlendMode, 3u);
    // Strengths go up to 4, as other NR hosts allow; any limit of its own is the runtime's.
    // Skin structure keeps its -1..2 range.
    const auto bounded = [](float value, float fallback, float minimum, float maximum)
    { return std::isfinite(value) ? std::clamp(value, minimum, maximum) : fallback; };
    result.intensity = bounded(result.intensity, 1.0f, 0.0f, 4.0f);
    result.localStructure = bounded(result.localStructure, 1.0f, 0.0f, 4.0f);
    result.localTone = bounded(result.localTone, pass == 0 ? 1.0f : 0.0f, 0.0f, 4.0f);
    result.skinStructure = bounded(result.skinStructure, -1.0f, -1.0f, 2.0f);
    return result;
}
} // namespace DlssNr::Profiles
