# ESP32 Deacon App — “Vibe coding” edition

Welcome! This project runs on a small **ESP32** board with a TFT screen and shows Deacon schedule data from a JSON file you host. You edit **C++** in `src/main.cpp`, build with **PlatformIO**, and flash over USB.

## What is “vibe coding”?

You describe what you want (to a human teammate or an AI assistant), review the code it suggests, and learn by changing things. You do not have to memorize every API — but it helps to know *roughly* what Wi-Fi, JSON, and the display stack are doing.

## What you are using

| Piece | What it is |
|--------|------------|
| **ESP32** | Microcontroller with Wi-Fi; runs your firmware. |
| **This repo** | Source code; usually hosted on **GitHub** so you can fork, clone, and share changes. |
| **IDE** | Editor + tools. Common choices: **Cursor**, **VS Code** + PlatformIO, **Google Antigravity**, etc. |
| **PlatformIO** | Builds the firmware and uploads it to the board (alien icon in VS Code / Cursor). |

## Choose your computer

### Windows, Mac, or Linux

Use a desktop IDE with **PlatformIO** (extension for VS Code / Cursor, or PlatformIO CLI).

- [Google Antigravity](https://antigravity.google/) — Google’s agentic IDE (preview).
- [Cursor](https://cursor.com/) — AI-native editor (very common for this workflow).
- [Windsurf](https://codeium.com/windsurf)
- **VS Code** + [PlatformIO IDE](https://platformio.org/install/ide?install=vscode) + optional GitHub Copilot

### Chromebook (ChromeOS)

Use **GitHub Codespaces** in the browser (full dev environment in the cloud). You cannot upload over USB from the cloud VM, so after **Build** you download `firmware.bin` and flash with **Web Serial** (see below).

---

## Set up the project

### 1. Git (optional but recommended)

Git tracks history and helps you share work.

- **Fork** the repo on GitHub (your own copy to push to).
- **Clone** your fork:

  ```bash
  git clone https://github.com/YOUR_USERNAME/deacon_esp32.git
  cd deacon_esp32
  ```

- **Commit** saves a snapshot locally; **push** sends it to GitHub.

  ```bash
  git add src/main.cpp
  git commit -m "Describe your change in plain English"
  git push
  ```

**Important:** `src/secrets.h` is **not** committed (it is in `.gitignore`). Never paste real passwords into a file you plan to push. Only `secrets.example.h` belongs in git.

### 2. Wi-Fi and JSON URL

1. In `src/`, **copy** `secrets.example.h` to **`secrets.h`** (same folder, exact name `secrets.h`).
2. Edit **`WIFI_NETWORKS`**: one or more `{ "ssid", "password" }` entries. The board tries them **in order**, about **20 seconds** each, then moves on.
3. Set **`DATA_JSON_URL`** to the HTTPS URL of your `deacon_data.json` (or the example URL your leaders provide).

After a successful sync, the device may not refetch for roughly **an hour** (to save battery / complexity). If data looks stale, check your server export and any CDN cache — not only the board.

### 3. Build and upload (PlatformIO)

1. Install **PlatformIO** (VS Code extension or [CLI](https://docs.platformio.org/en/latest/core/installation.html)).
2. Open this project folder in the IDE.
3. **Build** (checkmark): compiles firmware.  
4. **Upload** (arrow): flashes the ESP32 over USB.

**Serial monitor:** baud **115200**. Open the monitor, then press **RST** on the board to see boot messages (version string, Wi-Fi attempts). If the monitor shows nothing, reset while the monitor is open.

### 4. macOS tips (Apple laptops)

Macs are great for this project, but **USB** is where most workshop pain shows up. Try these in order:

**Serial port missing in the IDE**

- Many ESP32 boards use a **USB–serial bridge** (Silicon Labs **CP210x**, WCH **CH34x**, etc.). If nothing like `/dev/cu.usbserial-*` or `/dev/cu.SLAB_USBtoUART` appears in **PlatformIO → project → devices**, install the chip vendor’s driver. This repo ships macOS installers under **`drivers/`** (open the matching folder or PDF inside the zip).
- After installing a driver, **unplug and replug** the board. On newer macOS you may need to allow the driver under **System Settings → Privacy & Security** if macOS blocked it.

**USB-C–to–USB-C straight into the Mac**

- A lot of **USB-C cables are charge-only** (no data wires) or are flaky for **serial + reset** timing. If upload fails with “corrupt” / “wrong boot mode” / serial noise, **do not assume the cable is fine** just because it charges your phone.
- **What often works better:** plug the board through a **simple USB-C hub or dongle** that has an **USB-A port**, then use a **short USB-A ↔ micro-USB or USB-A ↔ USB-C** cable you know carries **data** (many kit cables are fine). An **Apple USB-C → USB-A adapter** plus the board’s data cable is a common winning combo.
- A **powered USB hub** (wall-powered) can help if the Mac’s port browns out during flash (rare but real on some hubs/laptops). It is **not required** for everyone — try a hub when you see random disconnects mid-upload.

**Upload / serial monitor quirks**

- Use **115200** for the serial monitor (matches `platformio.ini`).
- If the monitor is blank, open the monitor first, then press **RST** on the board.
- If **Upload** never connects: try another cable, another port, or **hold BOOT** (if your board has one) while starting upload, then release when esptool begins (exact timing varies by board — ask a leader or check the board’s silkscreen).

### 5. Chromebook / Flatpak / no USB from IDE

Codespaces and some **Flatpak** sandboxes cannot see USB. **Build** in the IDE, then:

1. Download **`.pio/build/esp32dev/firmware.bin`** from the project.
2. Use Chrome: [Adafruit WebSerial ESPTool](https://adafruit.github.io/Adafruit_WebSerial_ESPTool/) → Connect → flash **`firmware.bin`** at offset **`0x10000`** (default app slot for this partition layout).

*iPhone/iPad:* browsers cannot do Web Serial USB flashing the same way.

**Linux:** your user may need to be in the `dialout` group for serial access:

```bash
sudo usermod -aG dialout $USER
```

(log out and back in afterward)

---

## Firmware version

The file **`VERSION`** holds a simple release number (e.g. `1.0.0`). Each build runs `scripts/gen_fw_version.py`, which writes **`include/fw_version.h`** (git describe / commit). At boot, the firmware prints that info on **Serial** so you can confirm what is running.

---

## Ideas to customize (`src/main.cpp`)

Pick one, describe it clearly to your assistant, then build and test.

1. Change colors or fonts on one of the schedule screens.
2. Add a small indicator when Wi-Fi sync succeeds.
3. Use a physical button to cycle screens or show an “easter egg” message.

### Stretch: call a public HTTP API

The project already uses **HTTPClient** and **ArduinoJson**. You could add another screen that GETs JSON from a public API (respect their rate limits and terms).

Examples (for learning — APIs change over time):

- Minecraft server status: `https://api.mcsrvstat.us/2/mc.hypixel.net`
- Dad jokes (ask for JSON): `https://icanhazdadjoke.com/`
- NASA picture of the day: `https://api.nasa.gov/planetary/apod?api_key=DEMO_KEY` (`DEMO_KEY` is heavily rate-limited)
- PokéAPI: `https://pokeapi.co/api/v2/pokemon/pikachu`

If the build fails, copy the **compiler error** into your assistant and ask for a fix — that is normal iteration.

Happy building.
