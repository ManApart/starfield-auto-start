# AutoStartSFSE (Linux → Windows cross-build)

This is a minimal **SFSE plugin** that builds on Linux (Pop!_OS) using **MinGW-w64** and produces a **Windows x64 DLL** that SFSE can load.

When Starfield starts and SFSE loads the plugin, it appends log lines to:

- `Data\SFSE\Plugins\AutoStartSFSE.log` (next to the DLL)

## Prereqs (Pop!_OS)

Install the cross compiler + CMake:

```bash
sudo apt update
sudo apt install -y cmake make mingw-w64 gcc-mingw-w64-x86-64 g++-mingw-w64-x86-64
```

**Verify Windows headers are installed:**

```bash
# Check if Windows.h exists
ls -la /usr/x86_64-w64-mingw32/include/Windows.h || \
ls -la /usr/x86_64-w64-mingw32/include/windows.h

# If not found, try reinstalling:
sudo apt install --reinstall mingw-w64
```

## Build

```bash
chmod +x ./build-linux-to-windows.sh
./build-linux-to-windows.sh
```

The output DLL will be:

- `build-mingw/AutoStartSFSE.dll`

Optional: verify the exports SFSE needs:

```bash
x86_64-w64-mingw32-objdump -p build-mingw/AutoStartSFSE.dll | rg "SFSEPlugin_(Load|Preload|Version)"
```

## Install into Starfield (Steam/Proton)

Copy the DLL to your Starfield install folder:

- `<Starfield>\Data\SFSE\Plugins\AutoStartSFSE.dll`

Then launch Starfield via SFSE (as you already do).

## Verify it loaded

Check for:

- `<Starfield>\Data\SFSE\Plugins\AutoStartSFSE.log`

You should see lines like:

- `AutoStartSFSE: hello world (SFSEPlugin_Preload)`
- `AutoStartSFSE: hello world (SFSEPlugin_Load)`

