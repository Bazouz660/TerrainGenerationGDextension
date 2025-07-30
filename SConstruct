#!/usr/bin/env python
import os
import sys

env = SConscript("godot-cpp/SConstruct")

# Override C++ standard to C++20 for jthread support
if env.get("is_msvc", False):
    # Remove existing C++17 flag and add C++20
    if "/std:c++17" in env["CXXFLAGS"]:
        env["CXXFLAGS"].remove("/std:c++17")
    env.Append(CXXFLAGS=["/std:c++20"])
else:
    # Remove existing C++17 flag and add C++20
    if "-std=c++17" in env["CXXFLAGS"]:
        env["CXXFLAGS"].remove("-std=c++17")
    env.Append(CXXFLAGS=["-std=c++20"])

# For reference:
# - CCFLAGS are compilation flags shared between C and C++
# - CFLAGS are for C-specific compilation flags
# - CXXFLAGS are for C++-specific compilation flags
# - CPPFLAGS are for pre-processor flags
# - CPPDEFINES are for pre-processor defines
# - LINKFLAGS are for linking flags

# tweak this if you want to use different folders, or more folders, to store your source code in.
env.Append(CPPPATH=["src/"])
sources = Glob("src/*.cpp")

# Library name variable
env["library_name"] = "libgdterrain_generator"

if env["platform"] == "macos":
    library = env.SharedLibrary(
        "demo/bin/libgdterrain_generator.{}.{}.framework/libgdterrain_generator.{}.{}".format(
            env["platform"], env["target"], env["platform"], env["target"]
        ),
        source=sources,
    )
elif env["platform"] == "ios":
    if env["ios_simulator"]:
        library = env.StaticLibrary(
            "demo/bin/libgdterrain_generator.{}.{}.simulator.a".format(env["platform"], env["target"]),
            source=sources,
        )
    else:
        library = env.StaticLibrary(
            "demo/bin/libgdterrain_generator.{}.{}.a".format(env["platform"], env["target"]),
            source=sources,
        )
else:
    library = env.SharedLibrary(
        "demo/bin/libgdterrain_generator{}{}".format(env["suffix"], env["SHLIBSUFFIX"]),
        source=sources,
    )

Default(library)
