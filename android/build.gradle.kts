import org.gradle.nativeplatform.platform.internal.DefaultNativePlatform

plugins {
    alias(libs.plugins.android.application) apply false
    alias(libs.plugins.android.library) apply false
    alias(libs.plugins.kotlin.android) apply false
    alias(libs.plugins.compose.compiler) apply false
}

fun run(cmd: List<String>): String = providers.exec { commandLine = cmd }.standardOutput.asText.get().trim()

val isWindows = DefaultNativePlatform.getCurrentOperatingSystem().isWindows
val bash = if (isWindows) "C:\\Program Files\\Git\\bin\\bash.exe" else "bash"
rootProject.ext["versionCode"] = Integer.parseInt(run(listOf(bash, "../tools/unix/version.sh", "android_code")))
rootProject.ext["versionName"] = run(listOf(bash, "../tools/unix/version.sh", "android_name"))
