# DLSS-NR: the face/skin effect can't be tuned (DLSSNR.MaskBlendMode never sent)

## Overview & Environment

- **Operating System**: Linux 7.3.3-1-cachyos-bore-zen5-nvidiaopen (CachyOS)
- **Proton**: proton-cachyos-native 11.0.20261015
- **GPU**: NVIDIA GeForce RTX 5090 (Blackwell, architecture 0x1B0)
- **NVIDIA Driver**: 620.70.06
- **NR runtime**: the game's own `bin64/nvngx_dlssnr.dll`, version 310.9.16.0, loaded by the direct
  compatibility runtime (the Wine driver answers `CreateFeature(18)` with `0xBAD0000B`)
- **Game**: Crimson Desert (D3D12)
- **Target Subsystem**: DLSS Neural Rendering, model creation parameters

## Reported symptoms

DLSS-NR runs in Crimson Desert, but its effect on faces can't be tuned. The model's skin controls
(Skin structure, Auto skin mask) don't give a usable handle on how faces come out.

## Validity assessment

The `OptiScaler.log` of the affected session shows OptiScaler's own NR path:

```
NR diagnostic CreateFeature(18): result=0xBAD0000B handle=0x0
NR compatibility: candidate ...\Crimson Desert\bin64\nvngx_dlssnr.dll CreateFeature(18) result=0x00000001
DLSS-NR: feature created at 5120x2160 through direct compatibility runtime
```

So the menu's model settings do reach the runtime at creation, and a change rebuilds the feature
(`DlssNr_Proxy.cpp`, `state.settings != settings`). The tuning values themselves arrive.

The parameter names the runtime reads are listed in its string table. For 310.9.16.0 these are:
the resources and subrects (`Color`, `Output`, `Depth`, `MVec`, `ControlMask`, `UI`, `UIAlpha`,
`Backbuffer`, `BidirectionalDistortionField`), `Width`, `Height`, `Enabled`, `Reset`,
`DepthInverted`, `MVecScaleX/Y`, `ScalingRatio`, `Hint.Render.Preset`, `Style`, `Intensity`,
`LocalToneStrength`, `LocalStructureStrength`, `SkinStructureStrength`, `UseAutoMask`,
`UICorrection` and **`MaskBlendMode`**, plus the `DLSSNRComputeScalingRatioCallback` and
`DLSSNRGetStatsCallback` hooks.

Against that list, OptiScaler sent every tuning value except two:

| Key | Before | Effect |
| :--- | :--- | :--- |
| `DLSSNR.MaskBlendMode` | never written | the runtime always used its unset default |
| `DLSSNR.UICorrection` | fixed at `1` on DX12 and Vulkan | not adjustable |

`MaskBlendMode` controls how the model combines its own skin/character mask (`UseAutoMask`,
`SkinStructureStrength`) with its edit, which is the result the user wants to tune. Other NR hosts
report modes 0 to 3 on 310.9.16, with mode 0 giving little visible change from the mask. Which
mode the runtime falls back to when the key is missing is not known.

Keys other hosts write but this runtime's string table does not contain (`Hdr`, `SDR`,
`AutoExposure`, `InputWidth`, `OutputWidth`, `Output.Width`, `Upscaling`, `Scale`,
`Jitter.Offset.X/Y`, `GlobalToneStrength`) are never read by it and are not added.

This is a genuine gap in what OptiScaler exposes, not a misconfiguration.

## Solution

- `DlssNr::ModelSettings` gains `maskBlendMode` (optional) and `uiCorrection` (default on).
  `SetModelTuning` writes `DLSSNR.MaskBlendMode` only when a mode is chosen, and always writes
  `DLSSNR.UICorrection`, at creation and at every evaluate, on DX12 and Vulkan.
- `PassProfiles::PassSettings` resolves both per pass: later passes inherit pass 1 unless they set
  their own value; modes above 3 clamp to 3.
- Because both are part of `ModelSettings`, a change rebuilds the pass with a new parameter map, so
  going back to "Runtime default" really removes the key.
- Config keys: `MaskBlendMode`, `UICorrection`, `Pass{N}MaskBlendMode`, `Pass{N}UICorrection`
  (all `auto` in the shipped `OptiScaler.ini`).
- Menu, Model section, per pass: "Mask blend mode" (Runtime default / Mode 0-3, or Auto for later
  passes) and "UI correction".

Default-identical: with both keys on `auto`, the parameter map reaching the runtime is the same as
before (`UICorrection` = 1, no `MaskBlendMode`).

## Verification

- `tests/nr_mask_blend_mode`: production `PassProfiles.h` + `SetModelTuning` against a minimal
  Config. 5 cases, all pass (built with clang-cl against the MSVC CRT and Windows SDK, run under
  Wine).
- `tests/nr_model_creation/ProxyCreationTests.cpp`: three new cases on the real DX12 proxy (default
  leaves the key out, chosen values reach creation, clearing the mode rebuilds without it). Not run
  yet: needs the `external/spdlog` submodule and `run.ps1` on Windows.
- In game: pending. Crimson Desert, each of Runtime default and Mode 0-3, with Skin structure and
  Auto skin mask changed, comparing faces.
