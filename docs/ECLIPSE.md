# Eclipse

As mentioned in [Building](INSTALL.md), there are several IDEs which can be used for CoMaps development. With the refactoring of the code that happened in August 2025, Eclipse finally can be used to work with the CoMaps C++ and Java codebase. (Code in other languages is untested.)

Eclipse natively supports Java, thus it can be used to edit the entire Android codebase. However, although Eclipse was once the tool for Android development, the Android plug-ins (ADT and its fork, Andmore) are no longer being actively developed. Therefore, although you can edit Java code for Android in Eclipse, you will get warnings about missing Android-specific dependencies. Building and debugging the Android app is not possible (or will require a lot of tinkering to get it to work).

## Prerequisites

You need Eclipse with CDT (C/C++ developer tools).

## Import the code

**Do not** import the entire source directory – the code indexer will choke on `3party`. Instead, import the following directories as individual projects:

* `libs` – this should be the first dir to import
* `qt` if you want to work on the desktop app
* `tools` if you want to work on tools which are not part of the main app

If you want to work on the Android app from within Eclipse, you can also import `android`. We don’t have instructions for that yet (feel free to contribute yours) but have been able to import that portion of the code into Eclipse. There are three main caveats:

* You need to import C++ and Java code separately
* In Java, Eclipse may not be able to find Android-specific dependencies which are not part of the standard Java API
* The code has been refactored since this was last tried successfully; Android code has been split into an app and an SDK portion; you may need to import each of these separately

## Import code dirs

Repeat these steps for each subdir you want to work with (`libs`, `qt`, `android`, tools` etc.).

From the menu, choose **File > Import**.

In the dialog that opens, select **C/C++ > Existing Code as Makefile Project** and click **Next**.

In the next dialog:

* Set the **Project Name** (we recommend using the name of the subdir)
* Set the path for the **Existing Code Location** (a subdir of the CoMaps code dir, such as `libs`, `qt` or `tools`)
* Ensure both C and C++ are selected under **Languages**
* Under **Toolchain for Indexer Settings**, select **CMake driven**.
* Click **Finish**.

After you have imported the code and open the first source files, Eclipse will flag lots of errors in the code, as it cannot find any of the included header files outside the project. We will rectify that in the next steps.

## Header files in project root

Some header files are located in the root dir of the source tree. You to import them into a location where Eclipse can find them.

Right-click the `libs` project and choose **Import** from the context menu.

In the dialog that opens, select **General > File System** and click **Next**.

In the **File System** dialog:

* In **From Directory**, enter (or browse to) the root dir of the CoMaps sources.
* In the list of files below, check all header files (at the time of this writing, `defines.hpp`, `omim_config.h`, `precompiled_headers.hpp` and `private.h`).
* Under **Options**, click **Advanced**.
* Check **Create links in workspace**.
* Click **Finish**.

The header files will now appear at the root of `libs`. These are links that point to the files in their original location.

Should any additional header files ever get added to the root dir, you will have to repeat this step.

## Dependencies

Right-click the project root in Package Explorer and select **Properties** from the context menu.

In the dialog that opens, select **C/C++ General > Paths and Symbols** from the tree on the left.

Click the **Includes** tab and add the following paths:

* The standard include paths of your system (these can be obtained by running `echo | g++ -v -x c++ -E -` and looking for the line which reads `#include <...> search starts here:`)
* The following subdirs of `3party` (in the CoMaps source dir):
  * `boost`
  * `pugixml/pugixml/src`
  * For other dependencies, figure out where the header files are and how they are included – for example, if one of the source files includes `<foo/foo_base.hpp>` and you find the file in `3party/libfoo/headers/foo/foo_base.hpp`, include `3party/libfo/headers`
* Qt6 headers – on Ubuntu 24.04, they are found in `/usr/include/x86_64-linux-gnu/qt6`. (Only for needed for `qt`; also for `tools` if you’re working on GUI tools.) You will also need at least the following subdirs:
  * `QtCore`
  * `QtGui`
  * `QtOpenGL`
  * `QtOpenGLWidgets`
  * `QtWidgets`
* This list may be incomplete – if you spot anything missing and know how to add it, please add it here

On the **References** tab, check `libs` (except in the `libs` project itself, where this is not available and not needed).

## Building

> [!NOTE]
> So far, we haven’t found a way to configure our toolchain as a builder in Eclipse (which would allow us to have Eclipse build the artifact as needed before running it)  – if you know how to do this, your input is appreciated. Until then, the workaround is to configure the build command as an external tool. The build command must then be launched manually after making changes to the code. After that you can launch the build artifact using Run.

Build and run configurations are relative to a project. Since the code is spread across multiple projects, the recommendation is to configure the build for the project where the frontend resides – `qt` for the desktop app, `tools` for any tools.

### Configure the build command as an external command

From the menu, choose **Run > External Tools > External Tools Configurations...***

Right-click **Program** in the left pane and select **New configuration**. (One blank configuration may already have been created, which you can adapt to your needs.)

* Set a name for the external command (e.g. `Build`, followed by the artifact name and release or debug)
* On the **Main** tab:
  * **Location**: `${workspace_loc:/tools/unix/build_omim.sh}`
  * **Working Directory**: `${workspace_loc:/qt}/..` or `${workspace_loc:/tools}/..`
  * **Arguments**: arguments to the build script (e.g. `-r desktop` for a desktop release build)
* On the **Refresh** tab, uncheck **Refresh resources upon completion**.
* On the **Build** tab, uncheck **Build before launch**.

### Configure a builder

> [!WARNING]
> **This currently does not work, see above.**
> * The prebuilt binary gets launched but no build happens, not even after changes.
> * When explicitly starting a build, it fails with an error.
> * Changes to the build config do not seem to take effect.
> The steps here are for reference only; any input on getting this to work is welcome.

Right-click the project root in Package Explorer and select **Properties** from the context menu.

In the dialog that opens, select **C/C++ Build** from the tree on the left.

* Click **Manage Configurations** and create a new build configuration.
* Make sure the new configuration is selected in the **Configuration** drop-down list.
* On the **Builder Settings** tab:
  * Uncheck **Use default build command** and set the build command to `./tools/unix/build_omim.sh`.
  * Set the build directory to `${workspace_loc:/qt}/..` or `${workspace_loc:/tools}/..`.
* On the **Behavior** tab:
  * Select **Use custom build arguments** and supply the arguments to the build script (e.g. `-r desktop` for a desktop release build).
  * Uncheck **Build (Incremental build)** and **Clean**
  
### Create a run configuration

To run the artifact, choose **Run > Run Configurations...** from the menu. In the dialog that opens, right-click **C/C++ Application** in the pane on the left and choose **New Configuration**. Make the following settings:
* In the **Main** tab:
  * **Project**: one of the projects in the workspace, see recommendation above
  * **C/C++ Application**: path where the build artifact (the finished binary) resides
  * **Build Configuration**: choose the appropriate build configuration (see above)
* In the **Arguments** tab, you can choose the working directory from which to launch the binary, and specify command line arguments.
* In the **Environment** tab, you can set environment variables if you need this for your artifact.

### Building with CMake

The recommended toolchain is `build_omim.sh`. If nonetheless you need to build with CMake for some reason:
* Specify `cmake` (with full path, as returned by `which cmake`) as the build command in the build configuration
* Use the same build directory as you would for `build_omim.sh`, i.e. `${workspace_loc:/qt}/..` or `${workspace_loc:/tools}/..`.
* Specify the build arguments for CMake. This would also include the `--build` dir. You will also need `--target` to specify the build target.

### Building for Android

Although we have not tested it yet, triggering an Android build from within Eclipse should be possible in the same way if you specify the `gradlew` script as the build tool. This should be pretty straightforward with the external tool workaround described above.

Installing, launching and debugging the Android app from Eclipse might be more difficult or not even possible, as there is no longer an actively maintained toolchain for Android development with Eclipse.
