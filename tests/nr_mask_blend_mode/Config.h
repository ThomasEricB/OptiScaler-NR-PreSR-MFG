#pragma once
// The minimal Config that PassProfiles.h reads. CustomOptional keeps the production semantics that
// matter here: NoDefault starts empty, WithDefault starts at its default, both are std::optional.
#include <cstdint>
#include <optional>

enum HasDefaultValue
{
    NoDefault,
    WithDefault,
};

template <class T, HasDefaultValue defaultState = WithDefault> class CustomOptional : public std::optional<T>
{
    T _defaultValue {};

  public:
    CustomOptional() = default;
    CustomOptional(T defaultValue) : _defaultValue(defaultValue)
    {
        if constexpr (defaultState == WithDefault)
            std::optional<T>::operator=(defaultValue);
    }
    CustomOptional& operator=(const T& value)
    {
        std::optional<T>::operator=(value);
        return *this;
    }
    T value_or_default() const { return this->has_value() ? this->value() : _defaultValue; }
};

class Config
{
  public:
    CustomOptional<uint32_t> DlssNrPreset { 0 };
    CustomOptional<float> DlssNrIntensity { 1.0f };
    CustomOptional<uint32_t> DlssNrStyle { 0 };
    CustomOptional<float> DlssNrLocalStructure { 1.0f };
    CustomOptional<float> DlssNrLocalTone { 1.0f };
    CustomOptional<float> DlssNrSkinStructure { -1.0f };
    CustomOptional<bool> DlssNrAutoMask { true };
    CustomOptional<uint32_t, NoDefault> DlssNrMaskBlendMode;
    CustomOptional<bool> DlssNrUiCorrection { true };
    struct NrPassOverrides
    {
        CustomOptional<uint32_t, NoDefault> preset, style, maskBlendMode;
        CustomOptional<float, NoDefault> intensity, structure, tone, skin;
        CustomOptional<bool, NoDefault> autoMask, uiCorrection;
    };
    NrPassOverrides DlssNrPassOverrides[29];
};
