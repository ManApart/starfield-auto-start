# AutoStartSFSE (Linux -> Windows cross-build)

This is a minimal **SFSE plugin** that builds on Linux (Pop!_OS/Ubuntu) using
**clang-cl + lld-link + xwin** and produces a **Windows x64 DLL** that SFSE can load.

When Starfield starts and SFSE loads the plugin, it appends log lines to:

- `Data\SFSE\Plugins\AutoStartSFSE.log` (next to the DLL)

## Prereqs (Pop!_OS/Ubuntu)

Install the LLVM Windows-targeting tools + build tools:

```bash
sudo apt update
sudo apt install -y cmake ninja-build clang lld llvm
```

Install/populate an xwin sysroot with `crt/` and `sdk/` folders.
Set:

```bash
export XWIN_SYSROOT=/path/to/xwin
```

git clone `https://github.com/libxse/commonlibsf` to `third_party/commonlibsf_build_src` so that xmake.lua is in that folder. Init the git subdirectory


## Build

```bash
chmod +x ./build-linux-to-windows.sh
./build-linux-to-windows.sh
./copy-dll.sh & sf
```

The output DLL will be:

- `build-clangcl/AutoStartSFSE.dll`

Optional: verify the exports SFSE needs:

```bash
llvm-objdump -p build-clangcl/AutoStartSFSE.dll | rg "SFSEPlugin_(Load|Preload|Version)"
```

## Install into Starfield (Steam/Proton)

Copy the DLL to your Starfield install folder:

- `<Starfield>\Data\SFSE\Plugins\AutoStartSFSE.dll`

Then launch Starfield via SFSE (as you already do).

## Verify it loaded

Check for:

- `<Starfield>\Data\SFSE\Plugins\AutoStartSFSE.log`

You should see lines like:

- `AutoStartSFSE: SFSEPlugin_Preload`
- `AutoStartSFSE: SFSEPlugin_Load`
