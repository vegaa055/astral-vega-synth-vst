# Astral Vega

A wavetable-style software synthesizer (VST3 + Standalone) built with C++ and JUCE 8,
aimed at Synthwave and Hip-hop sound design. Inspired by Serum.

## Requirements

- Visual Studio 2022 with the **"Desktop development with C++"** workload
- CMake 3.22+
- JUCE 8 checked out at `./JUCE` (already in place; consider converting to a git
  submodule later). JUCE bundles the VST3 SDK, so the separate `./vst3sdk` download
  is optional/reference-only.

## Build

```powershell
cmake -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
```

Outputs land in `build/AstralVega_artefacts/Release/`:

- `VST3/Astral Vega.vst3` — copy to `C:\Program Files\Common Files\VST3` (or point
  your DAW's plugin path at the build folder)
- `Standalone/Astral Vega.exe` — run directly, no DAW needed (audio/MIDI settings
  in the Options menu; on-screen keyboard is playable with the mouse)

## Roadmap

- [x] **M1 — Playable skeleton**: 16-voice poly, sine/saw/square oscillator, ADSR,
      resonant low-pass filter, master gain, basic UI with on-screen keyboard
- [x] **M2a — Wavetable engine core**: band-limited mip-mapped wavetables (built via
      inverse FFT from spectral recipes), position morphing, unison up to 7 voices
      with detune + stereo spread, factory tables (Basic, PWM, Spectra)
- [x] **M2b — Full oscillator section**: 2nd wavetable oscillator with independent
      table/position/coarse-tune/unison, sine sub oscillator (-1/-2 oct), white
      noise source, per-oscillator levels
- [ ] **M2c — User wavetables**: import .wav wavetables (2048-sample frames,
      forward-FFT mipmapping), thread-safe table swapping
- [x] **M3 — Modulation**: 2 poly LFOs (5 shapes, retriggered), assignable Env 2,
      6-slot mod matrix (sources: LFOs/Env 2/velocity/mod wheel; targets: osc
      positions/levels, pitch, cutoff, resonance), control-rate voice rendering
      (32-sample chunks)
- [ ] **M3b — Modulation extras**: tempo-synced LFO rates, global (free-running)
      LFO mode, drag-and-drop routing in the custom UI (M8)
- [x] **M4 — Filter section**: custom TPT state-variable filter with continuous
      LP→BP→HP morph (also a mod target), tanh drive stage, key tracking,
      dedicated Env 2 amount knob
- [x] **M5 — FX chain**: distortion (tanh) → bitcrusher → phaser → chorus →
      feedback delay → reverb, each with bypass toggle; two-column editor layout
      (rack reordering deferred to the M8 custom UI)
- [x] **M6 — Musts for the genres**: Poly/Mono/Legato voice modes with held-note
      stack, portamento glide (exponential, constant-time), pitch-bend range
      control (0–24 semis), pump effect on the master bus (sidechain feel)
- [x] **M7 — Presets**: header preset bar (prev/next/browse/save), 10 factory
      presets defined in code as overrides-on-defaults, user presets as XML in
      `Documents/Astral Vega/Presets`
- [ ] **M8 — Custom UI**: full skinned interface, oscilloscope + spectrum display
