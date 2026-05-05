import os

def get_fb_sources(base_dir=""):
    fb_src_dir = os.path.join(base_dir, "flatbuffers/src")
    fb_sources = [
        "idl_parser.cpp",
        "idl_gen_text.cpp",
        "reflection.cpp",
        "util.cpp",
    ]
    return [os.path.join(fb_src_dir, f) for f in fb_sources]

def get_include_paths(base_dir=""):
    return [
        os.path.join(base_dir, "flatbuffers/src"),
        os.path.join(base_dir, "flatbuffers/include"),
        os.path.join(base_dir, "format"),
        os.path.join(base_dir, "runtime/include"),
    ]

def get_runtime_lib_path(base_dir, platform, arch):
    # Unify logic: platforms like macos, ios, and web typically don't use arch subdirs in this project's structure
    if platform in ['macos', 'ios', 'web']:
        return os.path.join(base_dir, "runtime", "libs", platform)
    else:
        return os.path.join(base_dir, "runtime", "libs", platform, arch)
