# Auto Start

This is my attempt to recreate TommInfinite's [Auto Recent Save Load](https://www.nexusmods.com/starfield/mods/2962). This is my first SFSE plugin and an experiment with vibe coding.

Current target runtime: Starfield `1.16.236` with SFSE `0.2.19`.

I am ashamed at how little I understand this code, but after 10 hours of wrangling, it seems to work on my machine. It's gross, and I half expect it to fail on anyone else's machine, but at least hopefully the DLL will continue to work.

----

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

git clone `https://github.com/libxse/commonlibsf` to `third_party/commonlibsf_build_src` so that xmake.lua is in that folder. Init the git subdirectory, and keep it current with the Starfield runtime you are targeting.


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
