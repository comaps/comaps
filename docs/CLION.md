# CLion

[CLion](https://www.jetbrains.com/clion/) is a C/C++ IDE from JetBrains. It will be familiar to Android developers -
Android Studio comes from the same IDE family. It is a proprietary, commercial IDE, but a non-commercial CLion licence
is available and can be used for CoMaps development. The IDE can be set up so that accurate static analysis is available
for C++ and Python code. It also supports step-by-step debugging.

## Setup

First, follow the [installation steps](INSTALL.md) to verify that CoMaps builds okay from the command line.

### Automatic CMake Configuration

Open `comaps` directory as a project in CLion. The IDE will parse the CMake configuration and
create _Run/Debug Configurations_ corresponding to CMake targets. You should be able to build and run them straight
away.

### Manual Build Configuration

If we wanted to follow the official build process as closely as possible, we can set up custom targets and run
configurations that use `tools/unix/build_omim.sh` and the binaries output to `../omim-build-{release|debug}`. For
example, for the CoMaps desktop app:

1. set up custom target in Settings → Build, Execution, Deployment → Custom Build Targets:
    1. Name: `CoMaps (debug)`
    2. Toolchain: leave on _Use Default_
    3. Build: press ellipsis and add a tool:
        1. Name: `build_omim.sh -d desktop`
        2. Program: `$ProjectFileDir$/tools/unix/build_omim.sh`
        3. Arguments: `-d desktop`
        4. Working directory: `$ProjectFileDir$`
    4. Clean: press ellipsis and add a tool:
        1. Name: `build_omim.sh -d clean`
        2. Program: `$ProjectFileDir$/tools/unix/build_omim.sh`
        3. Arguments: `-d clean`
        4. Working directory: `$ProjectFileDir$`
2. set up Run/Debug Configuration of type _Custom Build Application_:
    1. Name: `CoMaps`
    2. Target: `CoMaps (debug)`
    3. Executable: `../omim-build-debug/CoMaps`

Building this configuration will then execute the custom CoMaps build tooling, and the IDE will still be able to run a
step-by-step debugging session using the resultant binary.

### Python

CLion comes bundled with Python Community Edition plugin. To have the IDE run static analysis on files int
`tools/python`, first set up a python virtual env - we will use [uv](https://docs.astral.sh/uv/) in these instructions,
but you can also use plain
`venv`:

```shell
cd tools/python
uv venv
source .venv/bin/activate
```

Then, with the virtual env active, install the C++ build dependencies described in [INSTALL.md](INSTALL.md):

```shell
uv pip install "protobuf<3.21"
```

Install the dependencies for the modules of interest, for example;

```shell
uv pip install -r maps_generator/requirements_dev.txt
```

Finally, configure the python interpreter in the IDE:

* File → Settings → Build Execution Deployment → Python interpreter → Add Local Interpreter... → Select existing
    * Type: Python
    * Python path: `.../comaps/tools/python/.venv/bin/python`
* File → Reload CMake Project

## Troubleshooting

### Static analysis in C++ worked but is now broken!

File → Invalidate caches...; select "Reset CMake Cache" then _Invalidate and Restart_.

### I still see spurious errors highlighted in C++ files

Try turning off `clangd` analysis: Files → Settings → Languages & Frameworks → C/C++ → Clangd: uncheck "show errors and warnings from clangd".

### Spurious errors are highlighted in Python files

1. Make sure `tools/python` is marked as source root.
2. File → Settings → Build Execution Deployment → Python interpreter and make sure the python interpreter in
   `tools/python/.venv` is selected.
3. If the error is on an import, check that the required python package is installed in the `toosl/python/.venv` virtual
   environment.
