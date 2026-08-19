import org.gradle.nativeplatform.platform.internal.DefaultNativePlatform
import java.util.Locale

plugins {
    id("com.android.library")
    alias(libs.plugins.kotlin.android)
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
            var pchFlag = "OFF"
            if (project.hasProperty("pch")) pchFlag = "ON"

            var njobs = ""
            if (project.hasProperty("njobs")) njobs = project.property("njobs") as String

            var enableVulkanDiagnostics = "OFF"
            if (project.hasProperty("enableVulkanDiagnostics")) enableVulkanDiagnostics = project.property("enableVulkanDiagnostics") as String

            var enableTrace = "OFF"
            if (project.hasProperty("enableTrace")) enableTrace = project.property("enableTrace") as String

            cmake {
                cppFlags += listOf("-fexceptions", "-frtti")
                // There is no sense to enable sections without gcc's --gc-sections flag.
                cFlags += listOf("-fno-function-sections", "-fno-data-sections", "-Wno-extern-c-compat")
                arguments += listOf(
                    "-DANDROID_TOOLCHAIN=clang", "-DANDROID_STL=c++_static",
                    "-DSKIP_TESTS=ON", "-DSKIP_TOOLS=ON", "-DUSE_PCH=$pchFlag",
                    "-DNJOBS=$njobs", "-DENABLE_VULKAN_DIAGNOSTICS=$enableVulkanDiagnostics",
                    "-DENABLE_TRACE=$enableTrace",
                )
                targets += "organicmaps"
            }
        }

        // Use, for example, -Parm32 Gradle parameter to build only for armeabi-v7a.
        ndk {
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
        }
        register("beta") {
            matchingFallbacks += "release"
        }
    }

    externalNativeBuild {
        cmake {
            version = "3.22.1+"
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
