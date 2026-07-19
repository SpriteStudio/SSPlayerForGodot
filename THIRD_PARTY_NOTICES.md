# Third-Party Notices

This file contains the license notices for third-party libraries used in **SpriteStudioPlayer for Godot**.

We would like to thank the authors and contributors of these open-source projects.

---

## FlatBuffers

This project depends on FlatBuffers for binary data serialization.

Copyright (c) 2014 Google Inc.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.

The complete Apache License 2.0 text is included in `licenses/Apache-2.0.txt`,
and is bundled with binary releases under `addons/spritestudio/licenses/`.

---

## godot-cpp

The C++ bindings are built on godot-cpp, which is statically linked into the
compiled plugin binaries.

MIT License

Copyright (c) 2017-present Godot Engine contributors.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

---

## SpriteStudio-SDK

This project integrates `SpriteStudio-SDK` (`libssruntime` and `libssconverter`) via FFI bindings.

Copyright (c) CRI Middleware Co., Ltd. All rights reserved.

`SpriteStudio-SDK` is licensed under the BSD-3-Clause License.

### Rust Runtime Dependencies

`libssruntime` and `libssconverter` are written in Rust and depend on various third-party open-source crates.

**License Location:**
Because the runtime binaries are generated artifacts and not checked into this repository, the corresponding license documents for the Rust dependencies are generated or downloaded during the build/setup process.
You can find these third-party licenses inside the `ss_player/runtime/` directory (specifically, `THIRD-PARTY-LICENSES.ssruntime.md`, `THIRD-PARTY-LICENSES.ssconverter.md`, and `LICENSE.md`) after running the setup scripts.

**Distribution Note:**
Official release packages already bundle these third-party license files under `addons/spritestudio/licenses/` (`THIRD-PARTY-LICENSES.ssruntime.md`, `THIRD-PARTY-LICENSES.ssconverter.md`, and the runtime `LICENSE.md`). If you redistribute compiled Godot binaries or GDExtension packages that include `libssruntime`, keep those files alongside your distribution to comply with the open-source licenses of the underlying Rust crates.
