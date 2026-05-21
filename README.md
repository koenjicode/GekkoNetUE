# GekkoNet Unreal Plugin

Unreal Engine plugin for integrating [GekkoNet](https://github.com/HeatXD/GekkoNet) into Unreal Engine.
The current implementation has been tested for Unreal Engine 5.7, this plugin may work on older and new versions of Unreal but will need to be tested.

The goal of GekkoNet Unreal (GekkoNetUE) is to provide a Unreal wrapper around GekkoNet allowing you to build Rollback gameplay loops in Unreal Engine given deterministic game simulations.

For an example project on implementing GekkoNet please check the [GekkoGameUE](https://github.com/koenjicode/GekkoGameUE) example project.

## Prerequisites
- Follow the Prerequisites present on the GekkoNet GitHub page.
- The GekkoNet requires SDL3 to be installed; SDL3 is automatically collected if you have vcpkg installed. If you're running into build issues, this is most likely the culprit.
- You'll also need to build and place the `GEKKONET_STATIC.lib` files into your GekkoNet/Binaries/Win64 folder.

## Setup

Before building, you'll need to adjust the `CMakeLists.txt` in your GekkoLib folder and remove:
```
if(BUILD_SHARED_LIBS)
        set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreadedDLL")
    else()
        set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded")
    endif()
```
And replace it with: 
```
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreadedDLL")
```
After you've done this:

1. Copy the plugin into your project's `Plugins/` folder.
2. Follow the build instructions for GekkoNet on the GitHub page.
2. Regenerate your Unreal project files.
3. Build the project from your IDE or through Unreal.
