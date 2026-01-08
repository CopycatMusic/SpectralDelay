# Windows Installer Usage Guide (Inno Setup)

This guide explains how to create a professional `.exe` installer for the Windows version of **CopyCat Spectral Delay**.

## Prerequisites
1.  **Windows Computer**: You must perform these steps on a PC running Windows.
2.  **Inno Setup**: Download and install the free **Inno Setup (Standard Edition)** from [jrsoftware.org](https://jrsoftware.org/isdl.php).

---

## Step 1: Prepare the Files on Windows
On your Windows computer, create a new folder (e.g., `C:\PluginBuild\`) and copy the following files into it:

1.  **The Plugin**: The folder `CopyCat Spectral Delay.vst3` (and its contents).
2.  **The Script**: The file `installer_win.iss` (which I created in your `scripts/` folder).

**Folder Structure should look like this:**
```text
C:\PluginBuild\
├── CopyCat Spectral Delay.vst3\
│   └── Contents\
│       └── x86_64-win\
│           └── CopyCat Spectral Delay.vst3
└── installer_win.iss
```

---

## Step 2: Compile the Installer
1.  Open **Inno Setup Compiler**.
2.  Go to `File -> Open` and select `installer_win.iss`.
3.  Click the **Compile** button (blue play icon) in the toolbar or press `Ctrl + F9`.
4.  Wait for the process to finish.

---

## Step 3: Locate the Output
Once the compilation is complete, a new folder named `Output` will be created inside `C:\PluginBuild\`.
*   Inside `Output`, you will find: `CopyCat_Spectral_Delay_Win_v1.0.0.exe`.

---

## Step 4: Test the Installer
1.  Double-click the `.exe` file.
2.  Confirm that it asks for administrator permissions.
3.  Follow the wizard instructions.
4.  **Verify the cleaner layout:**
    *   **The Plugin**: Installed to `C:\Program Files\Common Files\VST3\CopyCat Spectral Delay.vst3` (Standard location).
    *   **The Uninstaller**: Installed to `C:\Program Files\CopyCat\CopyCat Spectral Delay\unins000.exe` (Dedicated folder).
5.  Open your DAW on Windows and check if the plugin is recognized.

---

## Troubleshooting
*   **Permissions**: If you get a "Permission Denied" error during installation, ensure you are running the installer as an Administrator.
*   **Path Not Found**: Ensure the `CopyCat Spectral Delay.vst3` folder is in the same directory as the `.iss` script before compiling.
