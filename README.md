Discord Voice Patcher (Unofficial)
WARNING: USE AT YOUR OWN RISK
This tool patches Discord's voice engine (discord_voice.node) to unlock higher audio quality features that are normally restricted. It may violate Discord's Terms of Service.
What's New in v2.0.0

Graphical User Interface (GUI): The tool now features a simple, user friendly GUI built with Dear ImGui for easy patching and control.

What it does (tested on Discord node version 1.0.9217)

Forces stereo output
Enables maximum possible Opus bitrate (~510 kbps)
Forces 48 kHz sample rate
Bypasses the built-in high-pass filter and replaces it with a custom one
Injects custom DC rejection
Completely disables a large number of built-in processing stages:
Compression chains
Limiters
Clippers
Voice filters
Sidechain/gating
Headroom adjustments
And more...

Removes error checks that block high bitrates

Result: Much cleaner, louder, and higher-fidelity voice audio in Discord calls.
Features

Real-time patching status feedback in the GUI
Adjustable gain control via slider
One-click apply patches

Requirements

Windows 10 or 11 (64-bit)
Visual Studio 2022 (Community edition is fine) with Desktop development with C++ workload
Discord running (Stable branch recommended)
Administrator privileges to run the patcher

Build Instructions

Install Visual Studio 2022
Download from: https://visualstudio.microsoft.com/downloads/
Select the Desktop development with C++ workload during installation.
Clone the repository (recommended)text git clone https://github.com/fttg252-cyber/audio-tool.git Or download as ZIP and extract.
Open the project
The repository now includes all necessary files: node.cpp, patch.cpp, gui.cpp, and the ImGui + GLAD dependencies (in folders imgui/ and glad/).
Create a new Empty C++ Project in Visual Studio.
Add all .cpp files from the repo to Source Files.
Add the ImGui source files (from imgui/ folder) to the project.
Include the glad/ folder for OpenGL loader.

Project Configuration
Set to Release and x64.
Additional Include Directories: Add paths to imgui/ and glad/include/.
Linker → Input → Additional Dependencies: Add opengl32.lib, glfw3.lib (if using GLFW for windowing – adjust based on gui.cpp implementation), and any other required libs.
Ensure the project uses GLFW or similar for window creation (as implemented in gui.cpp).

Build and Run
Build the solution (Ctrl+Shift+B).
Run the executable as Administrator with Discord open.


Usage

Launch the patcher (as Admin).
The GUI will appear - click to apply patches.
Use the Gain Slider to adjust output volume.
Patches apply in real-time to the running Discord process.

Notes

Tested primarily on recent Discord stable versions (Host 1.0.9217+ as of January 2026).
If Discord updates break the patches, offsets may need updating.
Source code is provided for transparency and further experimentation.
