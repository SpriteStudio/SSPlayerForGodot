# SpriteStudioPlayer for Godot

**The branch is in experimental and unstable phase.**

**No guarantee, No support and can't reply any requests and any bug reports regarding the branch.**

# For Player Develoeprs

## prepare development environment of Godot
Reference the official documents

- windows
    - https://docs.godotengine.org/en/stable/engine_details/development/compiling/compiling_for_windows.html
- macOS
    - https://docs.godotengine.org/en/stable/engine_details/development/compiling/compiling_for_macos.html

## clone repositories

```bash
git clone --recursive git@github.com:SpriteStudio/SSPlayerForGodot.git

cd SSPlayerForGodot
git clone https://github.com/godotengine/godot.git -b 4.6
git clone https://github.com/godotengine/godot-cpp.git -b 4.6
```

## get libssruntime binaries
T.B.D

## build custom godot or gdextension
### gdextension

```bash
./scripts/build-extension.sh
```
or
```pwsh
.\scripts\build-extension.ps1
```

### custom godot

```bash
./scripts/build.sh
```
or
```pwsh
.\scripts\build.ps1
```

# For SDK Developers
## prepare development environment of SpriteStudio7-SDK
Reference [For Develoeprs of SpriteStudio7-SDK](https://github.com/SpriteStudio/SpriteStudio7-SDK?tab=readme-ov-file#for-sdk-developers) and prepare development environment of SpriteStudio7-SDK.


## build and deploy libssruntime
Build ssruntime binary for your platform and archtecture and deploy the binary to [the runtime directory](./gd_spritestudio/runtime)

```bash
./scripts/build-runtime.sh
```
or
```pwsh
.\scripts\build-runtime.ps1
```
