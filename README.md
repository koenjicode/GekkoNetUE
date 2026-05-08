# GekkoNet Unreal Plugin

Unreal Engine plugin for integrating [GekkoNet](https://github.com/HeatXD/GekkoNet) into Unreal Engine.
The current implementation has been tested for Unreal Engine 5.7, this plugin may work on older and new versions of Unreal but will need to be tested.

The goal of GekkoNet Unreal (GekkoNetUE) is to provide a Unreal wrapper around GekkoNet allowing you to build Rollback gameplay loops in Unreal Engine given deterministic game simulations.

For an example project on implementing GekkoNet please check the [GekkoGameUE](https://github.com/koenjicode/GekkoGameUE) example project.

## Prerequisites

- Follow the Prerequisites present in the GekkoNet GitHub page.
- The GekkoNet requires SDL3 to be installed, SDL3 is automatically collected if you have vcpkg installed. If you're running into build issues, this is most likely the culprit.
- You'll also need to build place the `GEKKONET_STATIC.lib` files into your GekkoNet/Binaries/Win64 folder.

## Setup

1. Copy the plugin into your project's `Plugins/` folder.
2. Run the bat inside the plugins folder to build GekkoNet with the correct settings.
2. Regenerate your Unreal project files.
3. Build the project from your IDE or through Unreal.
