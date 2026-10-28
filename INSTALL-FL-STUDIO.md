# Install Chaz Velvet in FL Studio (Windows x64)

## 1. Get the plugin

Download **`ChazVelvet-VST3-Windows-x64.zip`** from GitHub Actions artifacts / Release on
[chazmaniac302/chaz-velvet](https://github.com/chazmaniac302/chaz-velvet), or from your OneDrive
**Chaz Velvet** folder.

If Windows marks the zip as downloaded from the internet: right-click → **Properties** → tick **Unblock** → OK.
Or run `Install-ChazVelvet.ps1` as Administrator (preferred).

## 2. Install the VST3

1. Unzip. You should see a folder named **`Chaz Velvet.vst3`** (bundle / folder, not a single file).
2. Copy the entire **`Chaz Velvet.vst3`** folder into:

   ```
   C:\Program Files\Common Files\VST3\
   ```

   Confirm `Contents\x86_64-win\Chaz Velvet.vst3` (binary) exists inside the bundle.
3. In FL Studio: **Options → Manage plugins**. Confirm that `C:\Program Files\Common Files\VST3\` is in the search paths.
4. Click **Find installed plugins** / **Start scan**.
5. Look for **Chaz Velvet** by **Chazmaniac** under **Generators / Instruments** (not Effects).

## 3. Load it

Channel Rack → **(+)** → generators → **Chaz Velvet**. It is an **instrument** — MIDI notes play keys. Do **not** put it on a Mixer FX slot as an effect.

## 4. First session

1. Open preset **VELVET**.
2. Draw a Cmin7 (C–Eb–G–Bb) → F → Bb progression, mid velocity.
3. Ride macros: **Velvet** (warmth), **Glow** (air), **Space** (plate), **Dust** (vinyl), **Width**.
4. Toggle layers: EP / PAD / SPARK / SUB as needed.

## Vendor

Chazmaniac · GPL-3.0-or-later · Plugin code `ChVl`
