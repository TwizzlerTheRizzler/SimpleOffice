# Simple Word Processor

A lightweight, fast, and portable desktop word processor built with **C++17**, **Qt6**, and **CMake**. Designed to run cleanly without extra bloat while offering rich text editing, formatting tools, and native OpenDocument (`.odt`) file support.

---

## Features

- **Rich Text Formatting:** Quick controls for Bold, Italic, Underline, and Text Alignment (Left, Center, Right).
- **Font & Size Controls:** Real-time font family selector and custom size adjustments (`-` / `+` or direct input).
- **Smart Undo/Redo System:** Custom word-by-word history stack (capped at 15 memory-efficient steps) with state-aware toolbar indicators.
- **Dynamic Status Bar:** Real-time word and character counter.
- **Unsaved Changes Guard:** Prompts to save work before clearing, opening a new file, or closing the window.
- **Clean Windows UI:** Dark-themed toolbar, custom window title management (`Filename - Simple Word Processor`), and zero unwanted console window popups.

---

## Supported File Formats

- **OpenDocument Text (`.odt`)** *(Default format)*
- **HTML Document (`.html`, `.htm`)**
- **Markdown Document (`.md`)**
- **Plain Text (`.txt`)**

---

## Download & Installation

1. Go to the **[Releases](../../releases)** tab.
2. Download the latest `Release.zip`.
3. Extract the ZIP folder anywhere on your PC.
4. Double-click `WordProcessor.exe` to run.

---

## Building from Source

### Requirements
- **C++17** compatible compiler (MSVC 2022, GCC, or Clang)
- **Qt 6.x** (Widgets module)
- **CMake 3.16+**

### Build Commands

```bash
# Clone the repository
git clone [https://github.com/TwizzlerTheRizzler/SimpleOffice.git](https://github.com/TwizzlerTheRizzler/SimpleOffice.git)
cd SimpleOffice

# Generate build files
cmake -B build -DCMAKE_BUILD_TYPE=Release

# Compile the project
cmake --build build --config Release
