import org.gradle.nativeplatform.platform.internal.DefaultNativePlatform
import java.util.Locale

plugins {
    id("com.android.library")
    alias(libs.plugins.kotlin.android)
}

/** Sanitizes property values to either "ON" or "OFF". Nonsensical values default to "OFF". */
private fun Project.getPropertyValueForCMake(propertyName: String): String {
    val propertyValue = (project.findProperty(propertyName) as? String)?.uppercase() ?: "OFF"
    return if (propertyValue == "ON" || propertyValue == "OFF") propertyValue else "OFF"
}

android {
    namespace = "app.organicmaps.sdk"
    compileSdk = providers.gradleProperty("propCompileSdkVersion").get().toInt()

    ndkVersion = "28.2.13676358"

    buildFeatures {
        buildConfig = true
    }

    defaultConfig {
        minSdk = providers.gradleProperty("propMinSdkVersion").get().toInt()

        externalNativeBuild {
            cmake {
                cppFlags += listOf("-fexceptions", "-frtti")
                // There is no sense to enable sections without gcc's --gc-sections flag.
                cFlags += listOf("-fno-function-sections", "-fno-data-sections", "-Wno-extern-c-compat")
                arguments += listOf(
                    "-DANDROID_TOOLCHAIN=clang",
                    "-DANDROID_STL=c++_static",
                    "-DSKIP_TESTS=ON",
                    "-DSKIP_TOOLS=ON",
                    "-DNJOBS=${project.findProperty("njobs") as? String ?: ""}",
                    "-DUSE_PCH=${project.getPropertyValueForCMake("enablePCH")}",
                    "-DENABLE_TRACE=${project.getPropertyValueForCMake("enableTrace")}",
                    "-DENABLE_VULKAN_DIAGNOSTICS=${project.getPropertyValueForCMake("enableVulkanDiagnostics")}",
                )
                targets += "organicmaps"
            }
        }

        // Use, for example, -Parm32 Gradle parameter to build only for armeabi-v7a.
        ndk {
            abiFilters.clear()
            if (project.hasProperty("arm32") || project.hasProperty("armeabi-v7a")) {
                abiFilters.add("armeabi-v7a")
            }
            if (project.hasProperty("arm64") || project.hasProperty("arm64-v8a")) {
                abiFilters.add("arm64-v8a")
            }
            if (project.hasProperty("x86")) {
                abiFilters.add("x86")
            }
            if (project.hasProperty("x86_64") || project.hasProperty("x64")) {
                abiFilters.add("x86_64")
            }
            if (abiFilters.isEmpty()) {
                abiFilters.add("armeabi-v7a")
                abiFilters.add("arm64-v8a")
                // For the emulator, Chromebooks and some Intel Atom devices.
                abiFilters.add("x86_64")
            }
            println("Building for $abiFilters architectures.")
        }
    }

    buildTypes {
        debug {
            isJniDebuggable = true
            externalNativeBuild.cmake.arguments += "-DANDROID_STL=c++_shared"
            if (!project.hasProperty("disableHWAsan"))
              externalNativeBuild.cmake.arguments += "-DENABLE_ASAN=ON"
        }
        register("beta") {
            matchingFallbacks += "release"
        }
    }

    externalNativeBuild {
        cmake {
            version = "3.25.3+"
            path = File("../../CMakeLists.txt")
        }
    }

    compileOptions {
        isCoreLibraryDesugaringEnabled = true
        sourceCompatibility = JavaVersion.VERSION_17
        targetCompatibility = JavaVersion.VERSION_17
    }

    lint {
        disable += "MissingTranslation"
        // https://github.com/organicmaps/organicmaps/issues/3551
        disable += listOf("MissingQuantity", "UnusedQuantity")
        // https://github.com/organicmaps/organicmaps/issues/1077
        disable += "CustomSplashScreen"
        // https://github.com/organicmaps/organicmaps/issues/3610
        disable += "InsecureBaseConfiguration"
        abortOnError = true
    }
    androidResources {
        ignoreAssetsPatterns += "!design"
    }
    kotlinOptions {
        // TODO: Remove after upgrading to Gradle 9.0 or higher.
        jvmTarget = "17"
    }
}

dependencies {
    implementation(libs.androidx.core)
    implementation(libs.androidx.media)
    implementation(libs.android.material)
    implementation(libs.androidx.core.ktx)
    implementation(libs.androidx.fragment)
    implementation(libs.androidx.annotation)
    implementation(libs.androidx.customview)
    implementation(libs.androidx.preference)
    implementation(libs.androidx.recyclerview)
    implementation(libs.androidx.documentfile)
    implementation(libs.androidx.lifecycle.process)
    testImplementation(libs.junit)
    testImplementation(libs.robolectric)
    coreLibraryDesugaring(libs.android.tools.desugar)
}

// TODO: Running lint task triggers native build. Find a better solution.
project.afterEvaluate {
    val hasLintRun = project.gradle.startParameter.taskNames.any { it.lowercase(Locale.getDefault()).contains("lint") }
    if (hasLintRun) {
        tasks.filter { task ->
            (task.name.startsWith("Native") || task.name.contains("CMake")) && task.project == project
        }.forEach { nativeTask ->
            logger.warn("Disabling task ${nativeTask.path} because lint is running.")
            nativeTask.onlyIf { false }
        }
    }

    val taskNames = gradle.startParameter.taskNames
    val keywords = listOf("assemble", "bundle", "compile", "install", "lint", "publish", "run")
    val runSetup = taskNames.any { task ->
        keywords.any { keyword ->
            task.startsWith(keyword)
        }
    }
    if (runSetup) {
        val isWindows = DefaultNativePlatform.getCurrentOperatingSystem().isWindows
        exec {
            workingDir = File("../..")

            if (!taskNames.toString().contains("Google")) {
                environment("SKIP_MAP_DOWNLOAD", "1")
            }

            if (isWindows) {
                environment("PYTHONUTF8", "1")
            }

            commandLine = mutableListOf(
                if (isWindows) "C:/Program Files/Git/bin/bash.exe" else "bash",
                "./configure.sh",
            )
        }
    }
}

tasks.withType<JavaCompile>().configureEach {
    options.compilerArgs.addAll(listOf("-Xlint:unchecked", "-Xlint:deprecation"))
}
