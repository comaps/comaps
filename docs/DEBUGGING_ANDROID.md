# Debugging Android

## Crashes in C++ code

You will need the stack trace from the logcat. It looks like this:

```

*** *** *** *** *** *** *** *** *** *** *** *** *** *** *** ***
pid: 0, tid: 17618 >>> app.comaps.google <<<

backtrace:
  #00  pc 0x000000000006fb48  /apex/com.android.runtime/lib64/bionic/libc.so (abort+156)
  #01  pc 0x0000000000827870  /data/app/~~PKz1DnOhgE2jUJue3wWB-A==/app.comaps.google-8At0mOvHeweqUC0sAJVrnA==/split_config.arm64_v8a.apk!liborganicmaps.so
  #02  pc 0x000000000082f674  /data/app/~~PKz1DnOhgE2jUJue3wWB-A==/app.comaps.google-8At0mOvHeweqUC0sAJVrnA==/split_config.arm64_v8a.apk!liborganicmaps.so
  #03  pc 0x00000000008791f4  /data/app/~~PKz1DnOhgE2jUJue3wWB-A==/app.comaps.google-8At0mOvHeweqUC0sAJVrnA==/split_config.arm64_v8a.apk!liborganicmaps.so
  #04  pc 0x000000000083bbe0  /data/app/~~PKz1DnOhgE2jUJue3wWB-A==/app.comaps.google-8At0mOvHeweqUC0sAJVrnA==/split_config.arm64_v8a.apk!liborganicmaps.so
  #05  pc 0x00000000008729d4  /data/app/~~PKz1DnOhgE2jUJue3wWB-A==/app.comaps.google-8At0mOvHeweqUC0sAJVrnA==/split_config.arm64_v8a.apk!liborganicmaps.so
  #06  pc 0x0000000000796d44  /data/app/~~PKz1DnOhgE2jUJue3wWB-A==/app.comaps.google-8At0mOvHeweqUC0sAJVrnA==/split_config.arm64_v8a.apk!liborganicmaps.so
  #07  pc 0x00000000007d5a34  /data/app/~~PKz1DnOhgE2jUJue3wWB-A==/app.comaps.google-8At0mOvHeweqUC0sAJVrnA==/split_config.arm64_v8a.apk!liborganicmaps.so
  #08  pc 0x000000000079c424  /data/app/~~PKz1DnOhgE2jUJue3wWB-A==/app.comaps.google-8At0mOvHeweqUC0sAJVrnA==/split_config.arm64_v8a.apk!liborganicmaps.so
  #09  pc 0x0000000000addbd0  /data/app/~~PKz1DnOhgE2jUJue3wWB-A==/app.comaps.google-8At0mOvHeweqUC0sAJVrnA==/split_config.arm64_v8a.apk!liborganicmaps.so
  #10  pc 0x0000000000ae19d0  /data/app/~~PKz1DnOhgE2jUJue3wWB-A==/app.comaps.google-8At0mOvHeweqUC0sAJVrnA==/split_config.arm64_v8a.apk!liborganicmaps.so
  #11  pc 0x000000000007f7ac  /apex/com.android.runtime/lib64/bionic/libc.so (__pthread_start(void*)+232)
  #12  pc 0x0000000000072adc  /apex/com.android.runtime/lib64/bionic/libc.so (__start_thread+64)
```

Starting on the first line that ends with `liborganicmaps.so`, take the long hexadecimal number. This is the program counter (pc).

You now need to download the debug symbols corresponding to the exact version of the crashed APK.

<details>
<summary>Via CI (not possible yet)</summary>
You can find these as an artifact on the release CI job (available for 90 days).
</details>

<details>
<summary>For a Google Play or Codeberg release</summary>
You can find it on the Release page, you'll need to unzip the zip file first.
</details>

If you have built a debug build locally you can find the debug symbols in `android/app/build/intermediates/merged_native_libs/webDebug/mergeWebDebugNativeLibs/out/lib/`, named `liborganicmaps.so`.

Once you have the debug symbols in a file named `liborganicmaps.so`, run `addr2line -p -e liborganicmaps.so <pc>`. For example:
```
addr2line -p -e android/app/build/intermediates/merged_native_libs/webDebug/mergeWebDebugNativeLibs/out/lib/arm64-v8a/liborganicmaps.so 0x0000000000827870
```

It will output the filename and line number of the crash. If the output is not useful, try using the next pc, until you find a good one.

## Interactive debugging of C++ code

1. Find the `lldb-server` executable, it should be in your Ndk toolchain.
   `find /path/to/Android/Ndk -name lldb-server`
2. `adb push /path/to/lldb-server /data/local/tmp/lldb-server`
3. `adb shell 'cat /data/local/tmp/lldb-server | run-as app.comaps.debug tee /data/data/app.comaps.debug/files/lldb-server >/dev/null'`
4. `adb shell run-as app.comaps.debug chmod 777 /data/data/app.comaps.debug/files/lldb-server`
5. `adb forward tcp:10086 tcp:10086`
6. `adb shell run-as app.comaps.debug /data/data/app.comaps.debug/files/lldb-server platform --listen '*:10086' --server`
7. Launch `lldb` locally and type into the LLDB shell:
   ```
   platform select remote-android
   platform connect connect://<serial-or-ip>:10086 # may need to retry once or twice
   process handle -p true -s false -n true SIGSTOP
   process handle -p true -s false -n true SIGBUS
   process handle -p true -s false -n true SIGSEGV
   platform process attach -n app.comaps.debug
   target symbols add -s liborganicmaps.so app/build/intermediates/merged_native_libs/webDebug/mergeWebDebugNativeLibs/out/lib/arm64-v8a/liborganicmaps.so
   b MyClass::MyMethod # or you can use filename:lineno format
   ```
