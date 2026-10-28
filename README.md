# Chaz Velvet

**Neo-soul / quiet-storm / RnB keys** instrument by **Chazmaniac** — the melodic companion to [Chaz Trap](https://github.com/chazmaniac302/chaz-trap).

Pure synthesis (no copyrighted samples). Velvet EP, warm pads, soft sparkle, optional left-hand sub. Macros: **Velvet · Glow · Space · Dust · Width**.

| | |
|---|---|
| Product | Chaz Velvet |
| Vendor | Chazmaniac (`Chzm`) |
| Plugin code | `ChVl` |
| Bundle ID | `com.chazmaniac.chazvelvet` |
| Formats | VST3 + Standalone (Windows x64) |
| JUCE | `8.0.16-security` |
| License | GPL-3.0-or-later |
| Factory presets | 25 (signature **VELVET**) |

## Build (engine tests on Linux)

```bash
cmake -B build-engine -DCHAZ_BUILD_PLUGIN=OFF
cmake --build build-engine -j
ctest --test-dir build-engine --output-on-failure
```

## Build (full plugin)

```bash
cmake -B build -A x64 -DCHAZ_JUCE_TAG=8.0.16-security   # Windows MSVC
cmake --build build --config Release --target ChazVelvet_VST3
```

Or push to `main` and let `.github/workflows/build-windows-vst3.yml` produce the zip + pluginval.

## Install (FL Studio)

See [INSTALL-FL-STUDIO.md](INSTALL-FL-STUDIO.md). Quick path: copy `Chaz Velvet.vst3` into
`C:\Program Files\Common Files\VST3\`, Unblock-File if needed, rescan plugins.

## Sound layers

1. **EP** — FM/tine Rhodes-ish with tone, decay, drive, tremolo  
2. **Pad** — detuned saw/triangle + soft filter, slow attack  
3. **Sparkle** — short metallic/bell top  
4. **Sub** — optional left-hand soft bass (key-split)

Play a **Cmin7 – F – Bb** progression on preset **VELVET**.
