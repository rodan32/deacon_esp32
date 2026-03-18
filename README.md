# 📱 ESP32 Deacon App - Vibe Coding Edition!

Welcome to the ESP32 Deacon App project! Tonight, we're going to learn how to **"Vibe Code."** 

## 🤔 What is "Vibe Coding"?
"Vibe coding" is a fun way to build software where you act like the *director* or *manager* of the project, and an AI (like ChatGPT, Claude, or Cursor) acts as your super-fast typist. You don't need to memorize complex programming languages or every single piece of syntax. You just need to explain *what* you want the code to do, and the AI will help write it for you!

## 🛠️ Prerequisites
Before we start, you'll need:
1. **Hardware:** An ESP32 microcontroller with a TFT display (like the TTGO T-Display).
2. **A Cable:** A USB cable that handles *data*, not just charging.
3. **An Account:** A free GitHub account so you can save your code.

## 💻 Choosing Your Tools

Depending on what computer you brought tonight, here's how you can vibe code:

### For Windows, Mac, or Linux Users
We highly recommend downloading an **AI-powered IDE** (Integrated Development Environment). These are code editors that have AI built right in to make writing code insanely fast.
* **[Cursor](https://cursor.sh/)** (Highly Recommended!)
* **[Windsurf](https://codeium.com/windsurf)**
* **VS Code** (with the GitHub Copilot extension installed)

### For Chromebook (ChromeOS) Users
Since Chromebooks can't install desktop apps easily, you can edit and build code directly in your browser using the cloud!
* **[GitHub Codespaces](https://github.com/features/codespaces)**: Go to our repository on GitHub, click the green `<> Code` button, switch to the "Codespaces" tab, and click "Create codespace on main". It literally opens a full VS Code editor directly in Chrome! 
* You can open up ChatGPT or Claude in another browser tab to ask for code, and then paste it right into your browser editor.

## 🚀 Setting Up the Project

### 1. Git (Saving & Sharing Code)
Git is like a massive "undo" button and a cloud backup for coders.
* **Clone:** Downloads the code from the internet to your computer.
  ```bash
  git clone https://github.com/rodan32/deacon_esp32.git
  ```
* **Commit:** Saves a snapshot of your changes to your local history. (Always write a tiny message describing what you changed!)
  ```bash
  git commit -am "Changed screen colors to blue"
  ```
* **Push:** Uploads your snapshots back up to GitHub so everyone else can see it.
  ```bash
  git push
  ```

### 2. PlatformIO (PIO) (Building the Code)
The ESP32 is a tiny computer, but it doesn't speak C++ directly. We use an extension called **PlatformIO** to translate (compile) our code into 1s and 0s and blast it over the USB cable onto the board.
* Look for the **Alien Head** 👽 icon on the left side of your editor (VS Code / Cursor).
* **Build (Checkmark `✓`):** Checks your code for typos and translates it.
* **Upload (Arrow `→`):** Pushes the translated code onto the ESP32 device!

---

💡 **Pro-Tip: Flashing from Chromebooks (or Android Phones!)**
Since GitHub Codespaces runs in the cloud, it can't magically reach through the internet into your Chromebook's USB port to automatically upload. However, Chrome has a superpower called **Web Serial**!
1. Build your code in Codespaces by clicking the **Checkmark ✓**.
2. On the left file explorer, find `.pio/build/esp32dev/firmware.bin`. Right-click it and select **Download**.
3. Plug your ESP32 into your Chromebook (or your Android Phone with a USB-C OTG cable!).
4. Open Chrome and go to the **[Adafruit WebSerial ESPTool](https://adafruit.github.io/Adafruit_WebSerial_ESPTool/)**.
5. Click **Connect**, select your USB port, and manually upload your `firmware.bin` file to address `0x10000`. Boom! Custom code blasted from a phone!

---

## 🎮 Tonight's Challenge
Pick something you want to customize! Your job is to *describe* what you want to the AI, and let it write the code for `src/main.cpp`. Here are some fun ideas:
1. "Change the background color of the 'Assignments' screen from black to dark blue."
2. "Make the 'LESSON: THIS WEEK' text blink!"
3. "Draw a tiny circle on the screen next to the Wi-Fi icon if the device connects successfully."
4. Add a secret "Easter Egg" screen that shows up if you press one of the ESP32's physical buttons!

**Remember:** If the AI writes something that causes a red squiggly line or breaks the build, don't panic! Just copy the error message, paste it right back to the AI, and say "I got this error, fix it." It will almost always correctly fix its own mistakes! 

Happy Vibe Coding! 😎
