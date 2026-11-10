// DLSSNR.MaskBlendMode and DLSSNR.UICorrection through the production PassProfiles.h and
// DlssNr_ModelParameters.h: what each pass resolves to, and which keys reach the NGX parameter map.
#include <Config.h>
#include "../../OptiScaler/dlssnr/PassProfiles.h"

#include <cstdio>
#include <iterator>
#include <map>
#include <string>
#include <variant>

namespace
{
struct Params : NVSDK_NGX_Parameter
{
    std::map<std::string, std::variant<unsigned long long, float, double, unsigned int, int, void*>> values;
#define PARAMETER_OVERLOAD(Type)                                                                                       \
    void Set(const char* key, Type value) override { values[key] = value; }                                            \
    NVSDK_NGX_Result Get(const char* key, Type* value) const override                                                  \
    {                                                                                                                  \
        auto it = values.find(key);                                                                                    \
        if (it == values.end() || !std::holds_alternative<Type>(it->second))                                           \
            return NVSDK_NGX_Result_Fail;                                                                              \
        *value = std::get<Type>(it->second);                                                                           \
        return NVSDK_NGX_Result_Success;                                                                               \
    }
    PARAMETER_OVERLOAD(unsigned long long)
    PARAMETER_OVERLOAD(float)
    PARAMETER_OVERLOAD(double)
    PARAMETER_OVERLOAD(unsigned int)
    PARAMETER_OVERLOAD(int)
    PARAMETER_OVERLOAD(void*)
#undef PARAMETER_OVERLOAD
    void Set(const char* key, ID3D11Resource* value) override { values[key] = (void*) value; }
    void Set(const char* key, ID3D12Resource* value) override { values[key] = (void*) value; }
    NVSDK_NGX_Result Get(const char*, ID3D11Resource**) const override { return NVSDK_NGX_Result_Fail; }
    NVSDK_NGX_Result Get(const char*, ID3D12Resource**) const override { return NVSDK_NGX_Result_Fail; }
    void Reset() override { values.clear(); }

    bool Holds(const char* key, unsigned int expected) const
    {
        auto it = values.find(key);
        return it != values.end() && std::holds_alternative<unsigned int>(it->second) &&
               std::get<unsigned int>(it->second) == expected;
    }
};

bool defaultsSendNoMaskBlendModeAndKeepUiCorrection()
{
    Config cfg;
    const auto settings = DlssNr::Profiles::PassSettings(cfg, 0);
    Params params;
    DlssNr::SetModelTuning(&params, settings);
    return !settings.maskBlendMode.has_value() && settings.uiCorrection &&
           !params.values.contains("DLSSNR.MaskBlendMode") && params.Holds("DLSSNR.UICorrection", 1u);
}

bool chosenValuesAreSent()
{
    Config cfg;
    cfg.DlssNrMaskBlendMode = 1u;
    cfg.DlssNrUiCorrection = false;
    Params params;
    DlssNr::SetModelTuning(&params, DlssNr::Profiles::PassSettings(cfg, 0));
    return params.Holds("DLSSNR.MaskBlendMode", 1u) && params.Holds("DLSSNR.UICorrection", 0u);
}

bool laterPassesInheritAndOverride()
{
    Config cfg;
    cfg.DlssNrMaskBlendMode = 2u;
    cfg.DlssNrPassOverrides[1].maskBlendMode = 0u; // pass 3
    cfg.DlssNrPassOverrides[1].uiCorrection = false;
    const auto pass2 = DlssNr::Profiles::PassSettings(cfg, 1);
    const auto pass3 = DlssNr::Profiles::PassSettings(cfg, 2);
    return pass2.maskBlendMode == 2u && pass2.uiCorrection && pass3.maskBlendMode == 0u && !pass3.uiCorrection;
}

bool outOfRangeModeIsClamped()
{
    Config cfg;
    cfg.DlssNrMaskBlendMode = 9u;
    return DlssNr::Profiles::PassSettings(cfg, 0).maskBlendMode == 3u;
}

bool changingEitherValueRebuildsThePass()
{
    Config cfg;
    const auto before = DlssNr::Profiles::PassSettings(cfg, 0);
    cfg.DlssNrMaskBlendMode = 1u;
    const auto mode = DlssNr::Profiles::PassSettings(cfg, 0);
    cfg.DlssNrUiCorrection = false;
    const auto correction = DlssNr::Profiles::PassSettings(cfg, 0);
    return before != mode && mode != correction;
}
} // namespace

int main()
{
    struct Case
    {
        const char* name;
        bool (*run)();
    };
    const Case cases[] {
        { "defaults send no MaskBlendMode and keep UICorrection on", defaultsSendNoMaskBlendModeAndKeepUiCorrection },
        { "chosen mask blend mode and UI correction are sent", chosenValuesAreSent },
        { "later passes inherit pass 1 and take their own override", laterPassesInheritAndOverride },
        { "out-of-range mask blend mode is clamped to 3", outOfRangeModeIsClamped },
        { "changing either value rebuilds the pass", changingEitherValueRebuildsThePass },
    };
    unsigned failures = 0;
    for (const auto& test : cases)
    {
        const bool pass = test.run();
        std::printf("%s %s\n", pass ? "PASS" : "FAIL", test.name);
        failures += !pass;
    }
    std::printf("Mask blend mode: %zu cases, %u failures\n", std::size(cases), failures);
    return failures ? 1 : 0;
}
