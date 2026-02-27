# VS Codium (VS Code without Telemetry)

Development of the C++ core and the Qt application is also possible by using VS Codium. The simplest way to develop and debug the Qt desktop app would be to install the Microsoft C/C++ extension. However, it is not a free software and is not running outside of VS Code anymore. As you might care about privacy, you may prefere using libre software like VS Codium, which is not supported by this extension. Luckily, there are workarounds to use only completely libre software. By installing the following extensions, you get an almost full-fledged IDE experience with integrated debugger:

* `clangd` by llvm-vs-code-extensions
* `CDT GDB Debug Adapter Extension` by Eclipse CDT Cloud
* `Memory Inspector` by Eclipse CDT Cloud


## Setup the development environment

Install VS Codium and the extensions mentioned above. Additionally, setup your development environment as described in [INSTALL_DESKTOP](INSTALL_DESKTOP.md).


## Traversing the code

Thanks to the `clangd` extension, you can find references, declarations, definitions and more for your code. You only need to tell `clangd` about your development environment by creating a `compile_commands.json` file. To create it, simply build the Qt application as described in [INSTALL_DESKTOP](INSTALL_DESKTOP.md), but add the additional parameter `-j`. This automatically creates the file and puts it into your worktree. Once you restarted VS Codium, `clangd` becomes your friend.


## Building the app

Building the app was done from terminal so far as described in [INSTALL_DESKTOP](INSTALL_DESKTOP.md).


## Setup a launch configuration

Debugging is possible thanks to the extension `CDT GDB Debug Adapter Extension`. It needs a file named `launch.json` in the directory `.vscode`. The content
varies from your specific use case, by the following configuration would work for executing the application built in debug mode:

```
{
    "version": "0.2.0",
    "configurations": [
        {
            "name": "Debug",
            "type": "gdb",
            "request": "launch",
            "program": "${workspaceFolder}/../omim-build-debug/CoMaps",
            "arguments": "data/styles/default/light/style.mapcss",
            "cwd": "${workspaceFolder}"
        }
    ]
}
```