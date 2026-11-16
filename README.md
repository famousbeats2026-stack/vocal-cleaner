# Famous Beats Vocal Cleaner — VST3 / AU plugin for Mac

A vocal cleaner that removes background noise (room tone, fan, hum, hiss)
from a vocal track. It uses RNNoise, the neural denoiser behind Discord,
Zoom and OBS voice cleanup. Put it first on a dry vocal track and turn
the Amount knob up.

**What you get here is the full plugin project — not the plugin file
itself.** A Mac plugin file (.vst3 / .component) can only be built on a
Mac (or on GitHub's Mac computers). Pick one of the two ways below to
make the installable file. The easy one is Option A — it needs no Xcode
on your Mac.

---

## Option A — Let GitHub build it for you (easiest, no Xcode needed)

You need a free GitHub account. If you don't have one yet, make one at
https://github.com/signup — it takes about 5 minutes, just an email
and a password.

1. On GitHub, press the **+** at the top right, then **New repository**.
   Name it `vocal-cleaner`, leave it Public, and press **Create repository**.
2. On the next page, press **"uploading an existing file"**.
3. Drag this whole folder (everything in it) into the upload area, then
   press **Commit changes**.
4. Click the **Actions** tab at the top. You'll see a run called
   "Build macOS plugin" — wait for the green checkmark (a few minutes).
   If it hasn't started, press **Run workflow** on the right.
5. When it's done, click the finished run, scroll to the bottom, and
   download **FamousBeatsVocalCleaner-macOS**. Unzip it — inside are:
   - `Famous Beats Vocal Cleaner.vst3`
   - `Famous Beats Vocal Cleaner.component`

## Option B — Build it on your own Mac (needs Xcode)

1. Install Xcode free from the App Store (it's a big download, about
   12 GB — let it finish). Then open Terminal once and run:
   `xcode-select --install`
2. In Terminal, get this project (replace the link with your own if you
   put it on GitHub, or just copy this folder to your Mac):
   ```
   git clone https://github.com/YOURNAME/vocal-cleaner.git
   cd vocal-cleaner
   ```
3. Build it:
   ```
   cmake -B build -DCMAKE_BUILD_TYPE=Release
   cmake --build build --config Release
   ```
4. The finished files are in:
   - `build/FamousBeatsVocalCleaner_artefacts/Release/VST3/Famous Beats Vocal Cleaner.vst3`
   - `build/FamousBeatsVocalCleaner_artefacts/Release/AU/Famous Beats Vocal Cleaner.component`

---

## Installing the plugin

1. Copy the files into your plugin folders:
   - `.vst3` → `~/Library/Audio/Plug-Ins/VST3/`
   - `.component` → `~/Library/Audio/Plug-Ins/Components/`
   
   (To get there in Finder: press Cmd+Shift+G and paste the path.)
2. This plugin isn't signed with an Apple developer certificate, so
   macOS may block it the first time. If your DAW says it can't load
   it, run this in Terminal (it just clears the block flag):
   ```
   xattr -cr ~/Library/Audio/Plug-Ins/VST3/"Famous Beats Vocal Cleaner.vst3"
   xattr -cr ~/Library/Audio/Plug-Ins/Components/"Famous Beats Vocal Cleaner.component"
   ```
   Then restart your DAW and rescan plugins.
3. Works on macOS 10.15 (Catalina) and newer, Intel and Apple Silicon.

## How to use it

- Put it **first** on a vocal track — before reverb, delay, tuning,
  or compression. Clean the dry vocal, then add your effects after it.
- **Amount** — how much cleaning to apply. 100% is full cleanup.
  Turn it down if a soft or breathy part starts sounding thin.
- **Bypass** — turns the cleaning off to compare.
- **Output** — make-up gain if the cleaned vocal comes out quieter.
- Use it on **dry vocals only**. It can't tell music from noise, so
  don't put it on a finished mix or a vocal with music behind it.
- It adds about 10 ms of latency, but your DAW compensates for that
  automatically, so the vocal stays in time.

## What it does, technically

- RNNoise neural denoiser (xiph/rnnoise, BSD licence), with the
  "sh" model from GregorR/rnnoise-models, embedded in the plugin.
  If that model ever fails to load, it falls back to RNNoise's
  built-in model.
- Runs at 48 kHz internally and resamples to and from your session's
  sample rate, so it works at 44.1 kHz, 48 kHz, 96 kHz, and anything
  else your DAW uses. Stereo: one denoiser per channel.
- Tested on Linux before shipping: steady background noise dropped
  about 30 dB on noise-only material (pink/room noise), about 40 dB
  on white noise/hiss, with the voice itself untouched. Real results
  depend on the recording — a vocal tracked in a noisy room cleans
  up a lot.

## Files in this project

- `CMakeLists.txt` — the build file (JUCE is downloaded automatically)
- `Source/PluginProcessor.h` / `.cpp` — the denoising engine
- `Source/PluginEditor.h` / `.cpp` — the black-and-gold interface
- `Source/RNNoise/` — the RNNoise C sources + the `sh.rnnn` model
- `test/rnn_test.c` — a small test program that proves the bundled
  RNNoise code really removes noise (see "Testing" in the notes below)
- `.github/workflows/build-macos.yml` — the cloud build

## Credits

- RNNoise by Jean-Marc Valin / Mozilla / Xiph.Org (BSD licence)
- "sh" model by Gregor Richards (rnnoise-models, BSD licence)
- Built with JUCE 8 (GPLv3 / JUCE licence — see JUCE's licence terms)
- Famous Beats Studio
