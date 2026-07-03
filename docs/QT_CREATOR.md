# Qt Creator

As mentioned in [Building](INSTALL.md), there are several IDEs which can be used for CoMaps development. At the time of this writing, Qt Creator is the only FOSS IDE known to work with the CoMaps C++ codebase.

Qt Creator supports C++, JavaScript, Python and QML, but it lacks support for Java and Swift. A workaround is to use Qt Creator for core development, including testing the desktop version, and a different IDE (e.g. Android Studio, Xcode) for languages not supported by Qt Creator.

Running a project from Qt Creator requires three things:

- A build configuration, telling Qt Creator how to build the artifact we want to run
- A deployment method, telling Qt Creator how and where to deploy the build artifact to the environment where it is going to run (default is to deploy locally, i.e. run artifacts from wherever the build job places them)
- A run configuration, telling Qt Creator what to run and how

Before we get there, however, we need to create a new project in Qt Creator and import the code.

## Import the code

The first step is to create a new project in Qt Creator and import the code.

Start Qt Creator. In the Welcome screen, click **New Project**.

In the dialog, select **Import Project** in the left column and **Import Existing Project** in the middle column, then click **Choose...**.

In the **Location** step of the wizard dialog, enter a name for the project (e.g. `comaps`) and choose the root folder of the CoMaps source code. Click **Next**.

In the **Files** step of the wizard dialog, accept the default settings by clicking **Next**.

In the **Project Management** step, choose **None** for version control (to keep the Qt Creator project files out of git). Click **Finish**.

## Set up build configurations

Next, we need to tell Qt Creator how to build the artifacts.

- In the mode selector (top part of the pane on the left), click **Projects**.
- On the left side of the content area, in the **Build & Run** section, click **Build**.

You can now modify the existing build configuration, or add new ones. The default build configuration is not very helpful, therefore it is safe to mess with or just delete.

The build configuration for a release version of CoMaps (Linux desktop) is as follows:

- Generic Manager:
  - Build directory: the directory where the CoMaps source code resides, e.g. `/home/user/src/comaps`.
- Build Steps:
  - Custom Process Step:
    - Command: `./tools/unix/build_omim.sh`
    - Arguments: `-r desktop`
    - Working directory: `%{buildDir}` (default value)
- Clean Steps:
  - Custom Process Step:
    - Command: `./tools/unix/build_omim.sh`
    - Arguments: `-r clean`
    - Working directory: `%{buildDir}` (default value)
- Build Environment:
  - Use System Environment (default). If you need some specific environment variables, you can set them here.

This can easily be adapted to any other artifact which can be built with `build_omim.sh`. Simply change the arguments in the custom process step in Build Steps. For example, `-d desktop` would produce a debug version of the desktop app.

Qt Creator defines some variables which can be used in paths and command lines, both in build and run configurations. When editing one of the values (the cursor must be in the respective field), click the icon in the right corner of the control to get a list of variables that can be used. Some particularly useful ones:

- `%{Env:HOME}`: Home dir (on Linux – this makes use of system environment variables)
- `%{sourceDir}`: Root directory of the source code
- `%{buildDir}`: Build directory, where build artifacts will be placed

## Set up run configurations

Now we need to tell Qt Creator how to run the artifact we have built. This is very similar to setting up a build configuration.

The following example is for running the release version of CoMaps (Linux desktop):

- Run:
  - Executable: the fully qualified path to the executable, e.g. `/home/user/src/omim-build-release/CoMaps`.
  - Command line arguments: supply as needed
    - Working directory: `%{buildDir}` (default value) should be OK for most cases. This would be the directory from which you run the executable; `%{buildDir}` is the build dir from the build configuration.
    - Run in terminal: usually not needed (even for console applicatons), unless your application expects user input on the console. (Console output will be available in the IDE.)
- Run Environment:
  - Use Build Environment should work for most cases.
- Debugger Settings:
  - Enable C++: enabled (default) – you will probably want this
  - Enable QML: leave disabled for now
- Valgrind Settings: leave default for now (unless you are familiar with Valgrind and are planning on using it)
- QML Profiler Settings: leave default for now (unless you are familiar with QML Profiler and are planning on using it)

## Run, debug or build

Click the kit selector (screen icon in the lower half of the pane on the left, below the mode selector) to select your build, deployment and run configurations.

Using the three buttons below the kit selector, you can now run, debug or build the artifact you selected.

The output pane at the bottom of the screen lets you view results of the process:

- Issues: errors and warnings from the build process
- Compile Output: the entire console output from the build process
- Application Output: console output from the application

## Beyond the basics

### Building artifacts with CMake

If you want to build an artifact with CMake, here’s how. The following example is for building `traff_assessment_tool` (a tool on the `traffic` branch). It uses a build dir named `build`, which must be created beforehand, in your CoMaps source dir. Then set up a build configuration as follows:

- Generic Manager:
  - Build directory: the directory where the CoMaps source code resides, e.g. `/home/user/src/comaps`.
- Build Steps:
  - Custom Process Step:
    - Command: `cmake`
    - Arguments: `. -B build`
    - Working directory: `%{buildDir}` (default value)
  - Custom Process Step:
    - Command: `cmake`
    - Arguments: `--build build --target traff_assessment_tool`
    - Working directory: `%{buildDir}` (default value)
- Clean Steps:
  - Custom Process Step:
    - Command: `cmake`
    - Arguments: `--target clean`
    - Working directory: `%{buildDir}` (default value)
- Build Environment:
  - Use System Environment (default), see above

### Using Valgrind

Qt Creator comes with support for [Valgrind](https://www.valgrind.org/), a powerful and useful tool when you need to track down issues related to memory access.

Especially when you encounter sporadic issues that are difficult to reproduce reliably, it is highly recommended you run your code through the Valgrind suite to rule out faulty memory access or race conditions as the culprit.

Valgrind needs to be installed separately, preferably from packages (under Linux) or from the website.

Qt Creator comes with default settings for Valgrind, which should work for most use cases.

To use Valgrind, click **Debug** in the pane on the left. In the debug view, in the pane below the code editor window, choose the debugging tool you want to use:

- Memcheck: detects issues related to memory access (accessing uninitialized memory, using memory that has already been freed memory, or accessing memory beyond what has been allocated).
- Callgrind: a call graph analyzer, examines calling relationships between functions.

To start a debugging session, click the debug button (green arrow with a bug) **in the debug pane, next to the drop-down box – not in the left pane** (although both look the same, the button in the left pane will start the normal debugger, not the tool you have chosen).

Be aware that Valgrind runs code in its own virtual machine, which considerably slows down execution. Even without any checks, code takes about 4–5 times as long to execute as it would normally.

The debug view also offers other tools, which are not part of Valgrind.

Valgrind also includes Helgrind, which is useful in detecting race conditions between threads, typically when there is an issue with thread synchronization. However, Qt Creator has no native support for running Helgrind from the IDE as of 2025. There is a [feature request](https://bugreports.qt.io/browse/QTCREATORBUG-25838) for it, which also describes a workaround:

- Run Helgrind from the command line with `--xml=yes --xml-file=helgrind.xml` (this logs output to an XML file)
- In the Qt Creator debugger pane, select **Memcheck** and click **Load External XML Log File** (open folder icon), then load `helgrind.xml`.

This may still fail, as Helgrind output is somewhat different from Memcheck output.

Of course, you can always run Helgrind from the command line in the standard way, without using the IDE.
