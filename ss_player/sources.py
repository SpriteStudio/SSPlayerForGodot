import os
import subprocess

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

# Godot's scons arch names differ from the Android ABI directory names the SDK
# packages libraries under (this is also what download-sdk / build-runtime emit).
# Map the build arch to the on-disk ABI directory so the linker finds the lib.
_ANDROID_ARCH_TO_ABI = {
    "arm64": "arm64-v8a",
    "arm32": "armeabi-v7a",
    "x86_64": "x86_64",
    "x86_32": "x86",
    "x86": "x86",
}

def get_runtime_lib_path(base_dir, platform, arch):
    # Unify logic: platforms like macos, ios, and web typically don't use arch subdirs in this project's structure
    if platform in ['macos', 'ios', 'web']:
        return os.path.join(base_dir, "runtime", "libs", platform)
    elif platform == 'android':
        abi = _ANDROID_ARCH_TO_ABI.get(arch, arch)
        return os.path.join(base_dir, "runtime", "libs", platform, abi)
    else:
        return os.path.join(base_dir, "runtime", "libs", platform, arch)


# --- Version header generation -------------------------------------------------
# Single source of truth for the plugin version is ss_player/VERSION.txt; the
# commit hash is stamped in at build time. generate_version_header() fills the
# placeholders in ss_player/ss_version.h.in and writes ss_player/gen/
# ss_version.gen.h. It is called from both SConstruct (GDExtension) and SCsub
# (custom module) so either build bakes in the same version.

def _ss_player_dir():
    # Absolute path to the ss_player/ directory, independent of the build's CWD.
    # sources.py is imported as a real module, so __file__ is defined here even
    # though it is not inside the SCsub script itself.
    return os.path.dirname(os.path.abspath(__file__))

def read_version():
    """Canonical semantic version from ss_player/VERSION.txt."""
    path = os.path.join(_ss_player_dir(), "VERSION.txt")
    try:
        with open(path, "r") as f:
            return f.read().strip() or "0.0.0"
    except (OSError, IOError):
        return "0.0.0"

def _git_hash_via_command(repo_dir):
    # Short commit hash via the git binary. Cross-platform (no shell=True).
    # Returns None when git is missing or this is not a checkout.
    try:
        proc = subprocess.Popen(
            ["git", "rev-parse", "--short", "HEAD"],
            cwd=repo_dir,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            universal_newlines=True,
        )
        out, _ = proc.communicate()
    except (OSError, ValueError):
        return None
    if proc.returncode != 0:
        return None
    out = out.strip()
    return out or None

def _find_git_dir(start):
    # Walk up from `start` to locate the .git directory. Returns None at the
    # filesystem root (e.g. a zip download with no checkout).
    d = start
    while True:
        dot_git = os.path.join(d, ".git")
        if os.path.isdir(dot_git):
            return dot_git
        if os.path.isfile(dot_git):
            # ".git" is a pointer file ("gitdir: <path>") in worktrees/submodules.
            try:
                with open(dot_git, "r") as f:
                    line = f.read().strip()
            except (OSError, IOError):
                return None
            if line.startswith("gitdir:"):
                p = line[len("gitdir:"):].strip()
                if not os.path.isabs(p):
                    p = os.path.normpath(os.path.join(d, p))
                return p
            return None
        parent = os.path.dirname(d)
        if parent == d:
            return None  # reached filesystem root
        d = parent

def _git_hash_via_head_file(start):
    # Parse .git/HEAD by hand: covers "git binary missing but checkout present".
    git_dir = _find_git_dir(start)
    if not git_dir:
        return None
    try:
        with open(os.path.join(git_dir, "HEAD"), "r") as f:
            head = f.read().strip()
    except (OSError, IOError):
        return None
    if not head.startswith("ref:"):
        # Detached HEAD: the file already holds the raw SHA.
        return head[:7] or None
    ref = head[4:].strip()
    # Loose ref file.
    try:
        with open(os.path.join(git_dir, ref), "r") as f:
            return (f.read().strip()[:7]) or None
    except (OSError, IOError):
        pass
    # packed-refs fallback.
    try:
        with open(os.path.join(git_dir, "packed-refs"), "r") as f:
            for line in f:
                line = line.strip()
                if not line or line[0] in "#^":
                    continue
                parts = line.split(" ", 1)
                if len(parts) == 2 and parts[1] == ref:
                    return parts[0][:7] or None
    except (OSError, IOError):
        pass
    return None

def get_git_hash():
    """Short commit hash, with fallbacks for git-less (zip download) installs.

    Resolution order:
      1. SSPLAYER_GIT_HASH env var  -- explicit override (CI, source-zip rebuilds)
      2. `git rev-parse --short HEAD`
      3. .git/HEAD parsed by hand   -- git binary missing, checkout present
      4. "unknown"                  -- zip download without .git: never fails the build
    """
    env_hash = os.environ.get("SSPLAYER_GIT_HASH", "").strip()
    if env_hash:
        return env_hash
    repo_dir = _ss_player_dir()  # git resolves the repo root from any subdir
    return (
        _git_hash_via_command(repo_dir)
        or _git_hash_via_head_file(repo_dir)
        or "unknown"
    )

def generate_version_header(base_dir=""):
    """Substitute placeholders in ss_version.h.in -> gen/ss_version.gen.h.

    Writes only when the content changes so unchanged builds don't recompile.
    Returns the path to the generated header.
    """
    version = read_version()
    git_hash = get_git_hash()
    version_full = version if git_hash == "unknown" else "%s+%s" % (version, git_hash)

    ss_dir = _ss_player_dir()
    template_path = os.path.join(ss_dir, "ss_version.h.in")
    gen_dir = os.path.join(ss_dir, "gen")
    out_path = os.path.join(gen_dir, "ss_version.gen.h")

    try:
        with open(template_path, "r") as f:
            content = f.read()
    except (OSError, IOError):
        # Fall back to a minimal header so a missing template never breaks builds.
        content = (
            "#pragma once\n"
            '#define SSPLAYER_VERSION      "@SSPLAYER_VERSION@"\n'
            '#define SSPLAYER_GIT_HASH     "@SSPLAYER_GIT_HASH@"\n'
            '#define SSPLAYER_VERSION_FULL "@SSPLAYER_VERSION_FULL@"\n'
        )

    content = (content
               .replace("@SSPLAYER_VERSION@", version)
               .replace("@SSPLAYER_GIT_HASH@", git_hash)
               .replace("@SSPLAYER_VERSION_FULL@", version_full))

    try:
        os.makedirs(gen_dir)
    except OSError:
        pass  # already exists

    # Write-if-changed: leave mtime untouched when nothing changed.
    try:
        with open(out_path, "r") as f:
            if f.read() == content:
                return out_path
    except (OSError, IOError):
        pass

    with open(out_path, "w") as f:
        f.write(content)
    return out_path
