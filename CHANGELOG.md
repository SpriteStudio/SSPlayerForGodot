# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [7.0.0-beta.1] - Unreleased

### Added
- **Initial Public Release**: SpriteStudio 7 SDK (`ssconverter-cli` + `Godot Plugin`).
- **Core Runtime**: Engine-agnostic, `#![no_std]` brain with zero-math integration and SIMD (f32x4) state computation.
- **Features**: Supports Mesh Deformation, Particles (SSEE), Instances, and SS7.1 parts (Skew, 9-Slice, Shape).
- **Integrations**: C-API and Web (WASM) ready.
- **Custom Allocator Injection**: `custom_alloc` feature lets game engines / consoles supply their own allocator and abort handler via `ss_custom_alloc` / `ss_custom_free` / `ss_custom_abort` C symbols — no wrapper crate required.
- **Documentation**: Overhauled bilingual (EN/JA) README and contribution guidelines.

### Known Limitations
- Text/Font Bitmap rendering pending validation.
- macOS/iOS binaries are not yet code-signed.
