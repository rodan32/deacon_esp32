# 📱 ESP32 Deacon App - Vibe Coding Edition!

Welcome to the ESP32 Deacon App project! Tonight, we're going to learn how to **"Vibe Code."** 

## 🤔 What is "Vibe Coding"?
"Vibe coding" is a fun way to build software where you act like the *director* or *manager* of the project, and an AI (like ChatGPT, Claude, or Cursor) acts as your super-fast typist. You don't need to memorize complex programming languages or every single piece of syntax. You just need to explain *what* you want the code to do, and the AI will help write it for you!

But before we tell the AI what to do, it helps to understand the moving parts!

## 🧠 The Cool Tech We're Using

### What is an ESP32?
The little circuit board you're holding is an **ESP32**. It's basically a tiny, cheap computer (a "microcontroller") that has built-in Wi-Fi and Bluetooth. It's not as powerful as your phone, but it's perfect for "Internet of Things" (IoT) gadgets like smart home devices, weather stations, or our Deacon schedule display! The code we write tells the ESP32 exactly what to show on its screen.

### What is a Repository (Repo)?
A **Repository** is like a super-powered Google Drive folder specifically made for code. Right now, all the files that make the ESP32 app work live in a repository on a website called **GitHub**. When we want to change the app, we download a copy of this repository, change the files, and then upload (push) our changes back so everyone can see them!

### What is an IDE?
An **IDE** (Integrated Development Environment) is the software you use to write code. Think of it like Microsoft Word, but instead of writing essays, it's designed to help you write software by highlighting mistakes, color-coding keywords, and running the code!

---

## 💻 Choosing Your Tools

Depending on what computer you brought tonight, here's how you can vibe code:

### For Windows, Mac, or Linux Users
We highly recommend downloading an **AI-powered IDE**. These are modern code editors that have AI built right in, so you can just highlight code and ask the AI to change it!
* **[Cursor](https://cursor.sh/)** (Highly Recommended!)
* **[Windsurf](https://codeium.com/windsurf)**
* **VS Code** (with the GitHub Copilot extension installed)

### For Chromebook (ChromeOS) Users
Since Chromebooks can't easily install massive desktop IDEs, you'll be using **GitHub Codespaces**. 
**Codespaces** is basically a full IDE running on a powerful super-computer in the cloud, but its display streams directly into a tab in your Chrome browser! 
1. Go to our repository on GitHub.
2. Click the green `<> Code` button, switch to the "Codespaces" tab, and click "Create codespace on main". 
3. Open ChatGPT or Claude in another Chrome tab to ask the AI for code, and then paste it right into your Codespaces IDE.

---

## 🚀 Setting Up the Project

### 1. Git (Saving & Sharing Code)
Git is the tool we use to manage our **Repository**. It's like a massive "undo" button and a time machine for coders.
* **Clone:** Downloads the code from the internet to your computer.
  ```bash
  git clone https://github.com/rodan32/deacon_esp32.git
  ```
* **Commit:** Saves a snapshot of your changes to your local history. (Always write a tiny message describing what you changed!)
  ```bash
  git commit -am "Changed screen colors to blue"
  ```
* **Push:** Uploads your snapshots back up to GitHub.
  ```bash
  git push
  ```

### 2. PlatformIO (PIO) (Building the Code)
The ESP32 is a tiny computer, but it doesn't speak our C++ language directly. It only understands raw 1s and 0s. We use an IDE extension called **PlatformIO** to translate (compile) our C++ code into 1s and 0s and blast it over the USB cable onto the board.
* Look for the **Alien Head** 👽 icon on the left side of your IDE.
* **Build (Checkmark `✓`):** Checks your code for typos and translates it.
* **Upload (Arrow `→`):** Pushes the translated code onto the ESP32 device!

---

💡 **Pro-Tip: Flashing from Chromebooks (or Android Phones!)**
Since GitHub Codespaces runs in the cloud, it can't magically reach through the internet into your Chromebook's USB port to automatically upload. However, Chrome has a superpower called **Web Serial**!
1. Build your code in Codespaces by clicking the **Checkmark ✓**.
2. On the left file explorer, find `.pio/build/esp32dev/firmware.bin`. Right-click it and select **Download**.
3. Plug your ESP32 into your Chromebook (or your Android Phone with a USB-C OTG cable!).
4. Open Chrome and go to the **[Adafruit WebSerial ESPTool](https://adafruit.github.io/Adafruit_WebSerial_ESPTool/)**.
5. Click **Connect**, select your USB port, and manually upload your `firmware.bin` file to address `0x10000`. Boom! Custom code blasted from the browser!

---

## 🎮 Tonight's Challenge
Pick something you want to customize! Your job is to *describe* what you want to the AI, and let it write the code for `src/main.cpp`. Here are some fun ideas:
1. "Change the background color of the 'Assignments' screen from black to dark blue."
2. "Make the 'LESSON: THIS WEEK' text blink!"
3. "Draw a tiny circle on the screen next to the Wi-Fi icon if the device connects successfully."
4. Add a secret "Easter Egg" screen that shows up if you press one of the ESP32's physical buttons!

**Remember:** If the AI writes something that causes a red squiggly line or breaks the build, don't panic! Just copy the compile error message, paste it right back to the AI, and say "I got this error, fix it." It will almost always correctly fix its own mistakes! 

Happy Vibe Coding! 😎
