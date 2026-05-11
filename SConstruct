#!/usr/bin/env python
import os
import sys
import subprocess

# --- Configuration & Constants ---
LIB_NAME = "SSGodot"
DEFAULT_PROJECT_DIR = os.path.join("examples", "dev_gdextension")

# --- Helper Functions ---
def normalize_path(val, env):
    return val if os.path.isabs(val) else os.path.join(env.Dir("#").abspath, val)

def validate_parent_dir(key, val, env):
    if not os.path.isdir(normalize_path(os.path.dirname(val), env)):
        raise UserError("'%s' is not a directory: %s" % (key, os.path.dirname(val)))

# --- Environment Setup ---
localEnv = Environment(tools=["default"], PLATFORM="")

# Import shared source definitions
sys.path.append(os.path.join(str(Dir("#").abspath), "ss_player"))
import sources

customs = ["custom.py"]
customs = [os.path.abspath(path) for path in customs]

opts = Variables(customs, ARGUMENTS)
opts.Add(
    BoolVariable(
        key="compiledb",
        help="Generate compilation DB (`compile_commands.json`) for external tools",
        default=localEnv.get("compiledb", False),
    )
)
opts.Add(
    PathVariable(
        key="compiledb_file",
        help="Path to a custom `compile_commands.json` file",
        default=localEnv.get("compiledb_file", "compile_commands.json"),
        validator=validate_parent_dir,
    )
)
opts.Add(
    PathVariable(
        key="target_path",
        help="Path to the Godot project where the extension will be installed",
        default=DEFAULT_PROJECT_DIR,
    )
)
opts.Update(localEnv)

Help(opts.GenerateHelpText(localEnv))

env = localEnv.Clone()
env["compiledb"] = False

# --- Compilation Database ---
env.Tool("compilation_db")
compilation_db = env.CompilationDatabase(
    normalize_path(localEnv["compiledb_file"], localEnv)
)
env.Alias("compiledb", compilation_db)

# --- Submodule Validation ---
submodule_initialized = False
dir_name = 'godot-cpp'
if os.path.isdir(dir_name) and os.listdir(dir_name):
    submodule_initialized = True

if not submodule_initialized:
    print("""godot-cpp is not available within this folder, as Git submodules haven't been initialized.
Run the following command to download godot-cpp:

    git submodule update --init --recursive""")
    sys.exit(1)

# --- Initialize godot-cpp ---
env = SConscript("godot-cpp/SConstruct", {"env": env, "customs": customs})

# --- Source Files ---
# Extension sources
sources_list = Glob("ss_player/*.cpp")

# FlatBuffers sources
sources_list.extend(sources.get_fb_sources("ss_player"))

# --- Compilation Flags & Includes ---
env.Append(CPPDEFINES = "SPRITESTUDIO_GODOT_EXTENSION")
env.Append(CPPPATH=sources.get_include_paths("ss_player"))

if env["platform"] in ["macos", "linux", "ios", "android"]:
    env.Append(CCFLAGS=["-fvisibility=hidden"])
    env.Append(CXXFLAGS=["-fvisibility=hidden"])

# --- Libraries & Library Paths ---
extension_path = env.Dir('.').abspath
platform = env['platform']
arch = env['arch']

runtime_libpath = sources.get_runtime_lib_path(os.path.join(extension_path, "ss_player"), platform, arch)

env.Append(LIBPATH=[runtime_libpath])

if env['target'] == 'editor':
    env.Append(LIBS=["ssconverter"])
    if platform == "windows":
        env.Append(LINKFLAGS=["Userenv.lib", "Bcrypt.lib", "Ntdll.lib", "Ws2_32.lib"])
        env.Append(LINKFLAGS=["/FORCE:MULTIPLE", "/IGNORE:4006", "/WX:NO"])

env.Append(LIBS=["ssruntime"])

# Platform specific frameworks
if platform == 'macos':
    env.Append(LINKFLAGS=["-framework", "CoreFoundation", "-framework", "CoreServices"])

if platform == "ios":
    ios_flags = ["-miphoneos-version-min=12.0"]
    env.Append(CCFLAGS=ios_flags)
    env.Append(LINKFLAGS=ios_flags)

# --- DocData Generation ---
if env["target"] in ["editor", "template_debug"]:
    try:
        # Output to ss_player/gen instead of src/gen
        doc_data = env.GodotCPPDocData("ss_player/gen/doc_data.gen.cpp", source=Glob("doc_classes/*.xml"))
        sources_list.append(doc_data)
    except AttributeError:
        print("Not including class reference as we're targeting a pre-4.3 baseline.")

# --- Shared Library Target ---
target_suffix = env["suffix"]
lib_suffix = env["SHLIBSUFFIX"]
file_name = "lib{}{}{}".format(LIB_NAME, target_suffix, lib_suffix)
framework_path = ""

if platform in ["macos", "ios"]:
    framework_path = "lib{}.{}.{}.framework/".format(LIB_NAME, platform, env["target"])
    file_name = "lib{}.{}.{}".format(LIB_NAME, platform, env["target"])
    env.Append(LINKFLAGS=["-Wl,-install_name,@rpath/{}{}".format(framework_path, file_name)])

library_output = "bin/{}/{}{}".format(platform, framework_path, file_name)
library = env.SharedLibrary(library_output, source=sources_list)

# --- macOS / iOS Framework Bundles ---
plist_target = None
if platform in ["macos", "ios"]:
    try:
        git_branch = subprocess.check_output(['git', 'rev-parse', '--abbrev-ref', 'HEAD']).decode('utf-8').strip()
        version = git_branch.split('-')[0] + '.0'
    except:
        version = "1.0.0"

    plist_subst = {
        "${BUNDLE_LIBRARY}": file_name,
        "${BUNDLE_NAME}": "ssplayer-godot",
        "${BUNDLE_IDENTIFIER}": "jp.co.cri-mw.spritestudio.ssplayer-godot.{}".format(env["target"]),
        "${BUNDLE_VERSION}": version,
        "${MIN_MACOS_VERSION}": "10.12",
        "${MIN_IOS_VERSION}": "12.0"
    }

    if platform == "macos":
        plist_src = "misc/Info.macos.plist"
        plist_file = "bin/macos/{}Resources/Info.plist".format(framework_path)
    else: # ios
        plist_src = "misc/Info.ios.plist"
        plist_file = "bin/ios/{}Info.plist".format(framework_path)

    plist_target = env.Substfile(target=plist_file, source=plist_src, SUBST_DICT=plist_subst)
    env.Depends(library, plist_target)

# --- Installation & Default Target ---
project_dir = env["target_path"]
addons_dir = "addons/spritestudio"
install_targets = [library]

# Copy library to project (following addons/spritestudio structure)
library_install_path = "{}/{}/{}".format(project_dir, addons_dir, library_output.replace("bin/", "bin/"))
copy_lib = env.InstallAs(library_install_path, library)
install_targets.append(copy_lib)

# Copy Plist if applicable
if plist_target:
    plist_install_path = "{}/{}/{}".format(project_dir, addons_dir, plist_file.replace("bin/", "bin/"))
    copy_plist = env.InstallAs(plist_install_path, plist_target)
    install_targets.append(copy_plist)

# Copy .gdextension file to addons/spritestudio
gdextension_src = "misc/spritestudio.gdextension"
gdextension_dest = "{}/{}/spritestudio.gdextension".format(project_dir, addons_dir)
copy_gdextension = env.InstallAs(gdextension_dest, gdextension_src)
install_targets.append(copy_gdextension)

if localEnv.get("compiledb", False):
    install_targets.append(compilation_db)

Default(*install_targets)
