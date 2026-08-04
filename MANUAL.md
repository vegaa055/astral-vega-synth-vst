# Astral Vega — User Manual

*A wavetable synthesizer for Synthwave and Hip-hop. VST3 + Standalone, Windows.*

---

## 1. At a Glance

Astral Vega is a 16-voice wavetable synthesizer built around four sound sources
per voice (two wavetable oscillators, a sine sub, and a noise source), a
morphing low-pass/band-pass/high-pass filter with drive, a 6-slot modulation
matrix fed by two LFOs and an assignable envelope, and a 7-effect master bus.

```
 PER VOICE                                          MASTER BUS
┌─────────────────────────────────────────────┐   ┌──────────────────────────┐
│  Osc A (wavetable, unison) ─┐               │   │  Distortion              │
│  Osc B (wavetable, unison) ─┤               │   │  Bitcrusher              │
│  Sub  (sine, −1/−2 oct)    ─┼─► Drive ─►    │   │  Phaser                  │
│  Noise (white, stereo)     ─┘   Filter ─►   ├──►│  Chorus                  │
│                                 Amp Env     │   │  Delay                   │
│  Mod: LFO 1/2 · Env 2 · Vel · Wheel         │   │  Reverb                  │
│       └─► 6-slot matrix + glide + bend      │   │  Pump ─► Master Gain     │
└─────────────────────────────────────────────┘   └──────────────────────────┘
```

**Layout**: preset bar and wavetable loader in the header; scope/spectrum
display below it; the **left column** is the synth engine (oscillators, filter,
mix/envelope, performance); the **right column** is modulation and FX; a
playable keyboard runs along the bottom.

---

## 2. Installation and First Sound

- **VST3**: copy `Astral Vega.vst3` to `C:\Program Files\Common Files\VST3`
  (or add the build folder to your DAW's plugin search path), then rescan.
- **Standalone**: run `Astral Vega.exe`. Choose your audio device and MIDI
  input under **Options**. No DAW required.

Play a note three ways: click the on-screen keyboard, use your QWERTY keyboard
(see [Typing Keyboard](#12-typing-keyboard)), or send MIDI from a controller or
your DAW. The default patch is a light-unison saw — if you hear that, you're
in business.

---

## 3. The Header

| Control | What it does |
|---|---|
| `<` / `>` | Step backward/forward through all presets (factory, then user) |
| Preset dropdown | Jump to any preset by name |
| **Save** | Save the current patch as a user preset (XML file you can share) |
| **Load WT** | Import a wavetable `.wav` into the **User** table slot |

User presets live in `Documents\Astral Vega\Presets`, one `.xml` file per
sound. Factory presets are built in and can't be overwritten — saving under a
factory name creates a user copy.

---

## 4. The Visualizer

The display strip shows the final output (after all effects):

- **SCOPE** — the output waveform, triggered on zero crossings so pitched
  material holds still. Great for *seeing* what WT Pos, Drive, and the filter
  do to the shape.
- **SPECTRUM** — click the display to switch. Log-frequency spectrum from
  20 Hz to 20 kHz with peak-hold decay. Watch harmonics appear as you raise
  Drive or open the filter.

---

## 5. Oscillators A and B

Both oscillators are identical. A **wavetable** is a stack of single-cycle
waveforms ("frames"); the **WT Pos** knob sweeps through them, morphing the
timbre continuously. All tables are band-limited internally (mip-mapped per
octave), so high notes never alias.

| Knob | Range (default) | What it does |
|---|---|---|
| **Table** | Basic / PWM / Spectra / User | Selects the wavetable (see below) |
| **WT Pos** | 0–1 (A: 0.66, B: 0) | Position along the table's frames. The single most powerful timbre control — modulate it! |
| **Coarse** | ±24 semis (0) | Transposes this oscillator. −12 = octave down, +7 = a fifth |
| **Unison** | 1–7 (1) | Stacks detuned copies of the oscillator per note |
| **Detune** | 0–100 cents (15) | How far the unison copies spread in pitch |
| **Spread** | 0–1 (0.5) | Fans the unison copies across the stereo field |
| **Level** | 0–1 (A: 0.8, B: 0) | This oscillator's volume in the voice mix |

**The display**: each oscillator panel has its own wavetable window. The bright
cyan curve is the exact frame you're hearing (interpolated between frames, just
like the engine); the faint curves behind it are frames sampled from across the
whole table, so you can see where a sweep will travel. Each frame is drawn
normalized, so you're comparing *shape*, not loudness.

While you hold a note the display goes **live**: it follows that voice's
*modulated* position, so an LFO or Env 2 aimed at WT Pos animates the waveform
in real time. The cursor bar underneath then shows two marks — a bright one for
where modulation has pushed the position, and a dim one for the knob itself, so
you can see how far the modulation is actually travelling. Release the note and
it settles back to the knob position. With several notes down it follows the
most recently played one, and the readout top-right shows the current frame out
of the total.

**The factory tables**

- **Basic** — sine → triangle → saw → square. Pos ≈ 0.66 is the classic saw.
- **PWM** — a square whose pulse width narrows from 50% down to 5%. Sweep the
  position (ideally with an LFO) for the classic analog "PWM movement."
- **Spectra** — a harmonic tilt from dark (almost sine) to screaming bright.
  Great for leads that need to cut.
- **User** — whatever you last imported with **Load WT**
  (see [User Wavetables](#11-presets-and-user-wavetables)).

**Unison notes**: each copy starts at a random phase (so stacks shimmer
instead of flanging), copies are gain-compensated (7 voices won't clip harder
than 1), and Level 0 costs essentially no CPU — Osc B is free until you use it.

---

## 6. Sub and Noise

| Knob | Range (default) | What it does |
|---|---|---|
| **Sub Oct** | −1 / −2 octaves | How far below the played note the sub sits |
| **Sub Level** | 0–1 (0) | Pure sine sub-oscillator level. The 808 foundation — sines stay clean on club systems where filtered saws turn to mud |
| **Noise** | 0–1 (0) | Stereo white noise. Adds breath to plucks and air to pads |

Both run through the filter and amp envelope like the oscillators.

---

## 7. Filter

A state-variable filter (12 dB/oct) whose character morphs continuously — no
mode switch, one knob.

| Knob | Range (default) | What it does |
|---|---|---|
| **Cutoff** | 20 Hz–20 kHz (12 kHz) | The filter frequency |
| **Resonance** | 0.5–8 (0.7) | Emphasis at the cutoff. Above ~4 it whistles; classic acid territory |
| **Morph** | 0–1 (0) | 0 = low-pass → 0.5 = band-pass → 1 = high-pass, crossfaded continuously. Also a mod target — LFO → Morph is a built-in phaser-ish effect |
| **Drive** | 0–1 (0) | Analog-style saturation *into* the filter with level compensation. 0 is a bit-exact clean bypass |
| **Keytrack** | 0–1 (0) | Cutoff follows the note you play, centered on middle C. At 1.0, one octave per octave — keeps high notes bright and low notes controlled |
| **Env Amt** | ±1 (0) | Dedicated Env 2 → cutoff amount (bipolar). At full, ±6 octaves of sweep. The most-used routing on any synth, so it gets its own knob |

---

## 8. Amp Envelope and Output

The main ADSR shapes each note's volume.

| Knob | Range (default) | Tip |
|---|---|---|
| **Attack** | 1 ms–5 s (5 ms) | >100 ms = pads; minimum = clicky plucks |
| **Decay** | 1 ms–5 s (0.2 s) | How fast it falls to the sustain level |
| **Sustain** | 0–1 (0.8) | Level while a key is held. 0 turns notes into one-shots |
| **Release** | 1 ms–8 s (0.3 s) | Tail after key-up. Long release + reverb = wash |
| **Gain** | −60…+6 dB (−6) | Master output, after all FX. Leave headroom — unison + drive + FX add up |

Velocity always scales note volume, and is also a matrix source for anything
else (cutoff is the classic).

---

## 9. Perform and Pump

| Control | What it does |
|---|---|
| **Voice** | **Poly** — 16 voices. **Mono** — one voice, every new note retriggers the envelopes. **Legato** — one voice, overlapping notes *don't* retrigger; the pitch just slides. Both mono modes remember held keys: release the top note and the pitch falls back to the one still held |
| **Glide** | 0–2 s portamento between notes (Mono/Legato only). Constant-time: an octave slide takes as long as a semitone slide, like classic hardware |
| **Bend Range** | 0–24 semis (2) | How far your pitch wheel bends |
| **Pump** | On / Sync / Division / Amount / Rate | A rhythmic volume duck on the master bus — instant dip, smooth recovery — the "sidechained to a kick" feel. Sync it to 1/4 notes and it locks to your DAW's grid |

**The 808 recipe lives here**: Mono + Glide ≈ 80 ms + Sub Level up = sliding
808 bass. Overlap notes to slide.

---

## 10. Modulation

### LFO 1 / LFO 2

| Control | What it does |
|---|---|
| **Shape** | Sine, Triangle, Saw Down, Square, S&H (random step per cycle) |
| **Free** | Off: each note gets its own LFO, restarted at note-on (right for vibrato). On: one global LFO shared by all voices, locked to the song timeline — chords wobble as a unit and renders are identical every time |
| **Sync** + **Division** | Lock the rate to host tempo, from 4 bars to 1/32, including triplet (T) and dotted (D) values |
| **Rate** | 0.02–20 Hz, used when Sync is off |

### Env 2

A second ADSR that doesn't control volume — you aim it at things. It feeds the
filter's **Env Amt** knob directly and is available in the matrix for
everything else. For plucks: fast attack, short decay, sustain 0.

### The Mod Matrix

Six slots, each: **Source → Target × Amount** (bipolar ±1).

- **Sources**: LFO 1, LFO 2, Env 2, Velocity, Mod Wheel.
  LFOs are bipolar (swing both ways); Env 2 / Velocity / Wheel go 0→1.
- **Targets** and what a *full* amount means:

| Target | Full amount (±1) equals |
|---|---|
| A Pos / B Pos | The entire wavetable position range |
| A Level / B Level | The entire level range |
| Pitch | ±1 octave (use ~0.03 for vibrato) |
| Cutoff | ±6 octaves of sweep |
| Reso | Most of the resonance range |
| Morph | The full LP→HP morph |

**Drag and drop**: grab a chip (**LFO 1 · LFO 2 · ENV 2 · VEL · WHEEL**) and
drop it on any knob that lights up cyan as you hover (WT Pos, Level, Cutoff,
Resonance, Morph). The routing lands in the first free slot with a starting
amount of +0.5. Use the slot's combos to fine-tune, invert, or clear
(set Source or Target to None). Pitch routings are set via the combos only.

---

## 11. Presets and User Wavetables

- **Factory bank** (10 sounds) doubles as a feature tour: *Supersaw Lead*,
  *Retro Pump Pad*, *PWM Strings*, *Neon Vibrato Lead* (synthwave), *808
  Slide*, *Lo-fi Keys*, *Wobble Bass*, *Dark PWM Bass* (hip-hop), plus *Init*
  and *Acid Pluck*.
- **Init** is your blank canvas — always start there when designing from
  scratch.
- **User wavetables**: **Load WT** imports any Serum-format `.wav` — audio
  consisting of concatenated **2048-sample** single-cycle frames (up to 256
  frames). The thousands of free Serum wavetables online load directly.
  Imported tables are band-limited like the factory ones, the file path is
  saved with your session and presets, and Osc A switches to the User slot
  automatically on load. A demo table ships at
  `Documents\Astral Vega\Wavetables\VegaFM.wav`.

---

## 12. Typing Keyboard

Your computer keyboard is a two-octave controller using the FL Studio layout:

```
octave +1:   2 3   5 6 7   9 0        (black keys)
            Q W E R T Y U I O P       (white keys, C→E)
base:        S D   G H J   L ;        (black keys)
            Z X C V B N M , . /       (white keys, C→E)
```

- The **− C4 +** buttons left of the keyboard shift the base octave (C0–C8) —
  the equivalent of FL's typing-keyboard octave switch.
- Typing works whatever you last clicked; it pauses automatically while you're
  typing a number into a value box.
- Note naming follows FL's convention (middle C = C5), on-screen and off.

---

## 13. FX Rack

Effects run in a fixed order on the master bus. Each has an LED toggle —
off means truly bypassed (zero CPU).

| Effect | Controls | Character |
|---|---|---|
| **Dist** | Drive, Mix | tanh saturation, gentle warmth → full grit. Mix < 1 for parallel drive |
| **Crush** | Bits (1–16), Downsample (1–40) | Lo-fi. Bits = crunch, Downsample = grainy aliasing. Bits 10 + DS 6 = tasteful lo-fi keys |
| **Phaser** | Rate, Depth, Mix | Sweeping notches, centered ~800 Hz |
| **Chorus** | Rate, Depth, Mix | Widens and thickens. Almost always good on pads |
| **Delay** | Sync + Division, Time (10–1500 ms), Feedback, Mix | Feedback echo. Changing time smears pitch like tape — musical, not a bug. Synced dotted-1/8 *is* the synthwave echo |
| **Reverb** | Size, Damp, Mix | Space. Damp tames the top end of the tail |
| **Pump** | (in the Perform panel) | Runs last — ducks everything, reverb tails included |

---

## 14. Designing a Sound from Scratch

The reliable order of operations — each step narrows the next:

1. **Start at Init** (and turn Gain down a touch if you'll stack unison + FX).
2. **Pick the raw material**: choose tables and WT Pos for A (and B) — get the
   *static* timbre roughly right first. Add Coarse −12 on B for weight, or +7
   for a fifth. Add Sub for foundation, Noise for air.
3. **Shape the note**: amp ADSR. Is this a pad (slow A, long R), a pluck
   (instant A, short D, S=0), a bass (fast A, medium R)?
4. **Carve with the filter**: bring Cutoff down until it hurts, then back off.
   Add Resonance for focus, Drive for muscle, Keytrack if high notes dull out.
5. **Add motion** — this is what separates a patch from a preset:
   - *Per-note motion*: Env 2 → Cutoff (Env Amt knob) or Env 2 → WT Pos.
   - *Continuous motion*: LFO → WT Pos (slow) or → Cutoff (synced).
   - *Expression*: Velocity → Cutoff, Wheel → anything.
6. **Perform setup**: Mono/Legato + Glide for basses and leads; bend range.
7. **Finish with FX**: usually Chorus (width) → Delay (depth) → Reverb
   (space). Dist/Crush go *before* those conceptually — they're in the chain
   that way. Pump if the track needs to breathe.
8. **Save it.** Name it something you'll recognize at 2 AM.

### Recipes

**Supersaw Lead** (the synthwave sound)
Basic table, Pos 0.66 · Unison 7 · Detune 25–30 · Spread 1.0 → Osc B on,
Coarse −12, Level 0.45 → Chorus on → Delay synced 1/8D, mix 0.3 → Reverb
size 0.7. Play fifths and octaves.

**Sliding 808**
Voice Mono · Glide 80 ms · Osc A Level 0 · Sub Level 0.95 (−1 Oct) · Dist on,
Drive 0.35 · Release 0.45 · Cutoff ~2 kHz. Overlap low notes to slide.

**Filter Pluck**
Init → Sustain 0.25, Decay 0.25, Release 0.2 · Cutoff 300 Hz, Reso 4,
Drive 0.45 · Env Amt +0.55 · Env 2: A 1 ms, D 0.18 s, S 0 → play staccato.
Add Velocity → Cutoff +0.3 so hard hits snap brighter.

**Wobble Bass**
Voice Mono · Osc B −12, Level 0.5 · Cutoff 900 Hz, Reso 2.5 · LFO 1 Sync 1/8
→ drag **LFO 1** onto **Cutoff**, set amount ≈ −0.55 · Dist on. Change the
LFO division live for rhythm variations; try S&H for random stutter.

**Moving Pad**
Basic or PWM table · Unison 5, Detune 18, Spread 0.9 · Attack 0.3 s, Release
1.2 s · LFO 2 Free + slow (0.3 Hz) → A Pos +0.35 · Chorus + big Reverb ·
Pump synced 1/4, amount 0.5 for the retrowave chug.

**Lo-fi Keys**
Basic Pos 0.15 · Decay 0.5, Sustain 0.6 · Crush on: Bits 10, Downsample 6 ·
small Reverb · Velocity → Cutoff. Play 7th chords, swing the timing.

**FM-ish Digital Lead**
Load `VegaFM.wav` (Load WT) · sweep WT Pos by hand, then give it to
**ENV 2 → A Pos** (+0.4, decay 0.3 s) so every note blooms from bright to pure.

---

## 15. Tips and Troubleshooting

**Tips**
- Watch the **spectrum** while you tweak — Drive, Morph, and WT Pos make more
  sense when you can see them.
- Stack mod routings: the same LFO into Cutoff *and* Morph at different
  amounts sounds far more alive than either alone.
- Free-running LFOs + Sync = renders that sit perfectly on the grid, take
  after take.
- Unison costs CPU (up to 7 oscillators per note per osc). If you're piling
  up voices in a dense project, freeze/bounce like you would with any synth.

**No sound?** Check in order: master Gain up? · Osc A **Level** up (or Sub)? ·
Cutoff not at 20 Hz with Env Amt negative? · amp Sustain not 0 with long
notes? · Standalone: audio device selected under Options?

**Stuck note?** All typing-keyboard notes release when the window loses
focus. In a DAW, the transport stop button (MIDI panic) clears everything.

**Preset didn't recall my imported wavetable?** The preset stores the *path*
to the `.wav` — if the file moved, re-import it via Load WT and re-save.

---

*Astral Vega is built with JUCE 8. Source, presets, and this manual:*
*github.com/vegaa055/astral-vega-synth-vst*
