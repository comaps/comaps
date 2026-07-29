import com.github.triplet.gradle.androidpublisher.ReleaseStatus
import ru.cian.huawei.publish.BuildFormat
import ru.cian.huawei.publish.DeployType
import ru.cian.huawei.publish.ReleaseNote
import ru.cian.huawei.publish.ReleaseNotesExtension
import java.util.Locale

buildscript {
    repositories {
        google()
        mavenCentral()
    }

    dependencies {
        classpath(libs.android.tools)
        classpath(libs.triplet.play.publisher)
        classpath(libs.huawei.publish)
    }
}

plugins {
    id("com.android.application")
    id("com.github.triplet.play") version libs.versions.tripletPlayPublisher
    id("ru.cian.huawei-publish-gradle-plugin") version libs.versions.huaweiPublish
    alias(libs.plugins.kotlin.android)
}

java {
    toolchain {
        languageVersion.set(JavaLanguageVersion.of(21))
    }
}

project.ext["appId"] = "app.comaps"
// These are properly set in the 'secure.properties.*' files but must be declared here for build sync to succeed.
project.ext["secretTestStoreFile"] = "comaps-test.keystore"
project.ext["secretTestStorePassword"] = ""
project.ext["secretTestKeyAlias"] = "CoMaps Test"
project.ext["secretTestKeyPassword"] = ""
project.ext["secretReleaseStoreFile"] = "comaps-release.keystore"
project.ext["secretReleaseStorePassword"] = ""
project.ext["secretReleaseKeyAlias"] = "CoMaps Release"
project.ext["secretReleaseKeyPassword"] = ""

val secureReleasePropertiesFileExists = File("secure.properties.release").exists()
if (secureReleasePropertiesFileExists) {
    apply("../secure.properties.release")
}

val secureTestPropertiesFileExists = File("secure.properties.test").exists()
if (secureTestPropertiesFileExists) {
    apply("../secure.properties.test")
}

android {
    namespace = "app.organicmaps"

    dependenciesInfo {
        // Disables dependency metadata when building APKs (for IzzyOnDroid/F-Droid)
        includeInApk = false
        // Disables dependency metadata when building Android App Bundles (for Google Play)
        includeInBundle = false
    }

    buildFeatures {
        dataBinding = true
        buildConfig = true
    }

    // Users are complaining that the app should be re-downloaded from the Play Store after changing the language.
    bundle {
        language {
            enableSplit = false
        }
    }

    compileSdk = providers.gradleProperty("propCompileSdkVersion").get().toInt()

    defaultConfig {
        versionCode = rootProject.ext.get("versionCode") as Int
        versionName = rootProject.ext.get("versionName") as String
        applicationId = project.ext.get("appId") as String
        minSdk = providers.gradleProperty("propMinSdkVersion").get().toInt()
        targetSdk = providers.gradleProperty("propTargetSdkVersion").get().toInt()
        base.archivesName = "${project.name.replace(oldValue = "\\s", newValue = "")}-${defaultConfig.versionCode}"
        ndk.debugSymbolLevel = "full"
        buildConfigField("String", "REVIEW_URL", "\"\"")
        buildConfigField("String", "SUPPORT_MAIL", "\"android@comaps.app\"") // Customized in flavors.
        println("Version: $versionName")
        println("VersionCode: $versionCode")
    }

    flavorDimensions += "default"

    productFlavors {
        create("google") {
            dimension = "default"
            applicationIdSuffix = ".google"
            versionName = "${android.defaultConfig.versionName}-Google"
            buildConfigField("String", "SUPPORT_MAIL", "\"gplay@comaps.app\"")
            buildConfigField("String", "REVIEW_URL", "\"market://details?id=app.comaps.google\"")
        }

        // Distributed directly by the project, e.g. in repo releases, chats, etc.
        create("web") {
            dimension = "default"
            versionName = android.defaultConfig.versionName
            buildConfigField("String", "SUPPORT_MAIL", "\"apk@comaps.app\"")
        }

        create("fdroid") {
            dimension = "default"
            applicationIdSuffix = ".fdroid"
            versionName = "${android.defaultConfig.versionName}-FDroid"
            buildConfigField("String", "SUPPORT_MAIL", "\"fdroid@comaps.app\"")
        }

        create("huawei") {
            val huaweiVersionCodeBase = 1_00_00_00_00
            dimension = "default"
            applicationIdSuffix = ".huawei"
            versionName = "${android.defaultConfig.versionName}-Huawei"
            versionCode = huaweiVersionCodeBase + android.defaultConfig.versionCode as Int
            buildConfigField("String", "SUPPORT_MAIL", "\"huawei@comaps.app\"")
            buildConfigField("String", "REVIEW_URL", "\"appmarket://details?id=app.comaps\"")
        }
    }

    playConfigs {
        create("googleRelease") {
            enabled = true
        }
    }

    splits.abi {
        isEnable = project.hasProperty("splitApk").also { println("Create separate apks: $it") }
        reset()
        include("x86", "armeabi-v7a", "arm64-v8a", "x86_64")
        isUniversalApk = true
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

    gradle.projectsEvaluated {
        android.applicationVariants.configureEach {
            tasks.register<Exec>(name = "run${name.replaceFirstChar(transform = Char::uppercase)}") {
                commandLine(
                    android.adbExecutable,
                    "shell",
                    "am",
                    "start",
                    "-n",
                    "$applicationId/app.organicmaps.DownloadResourcesActivity",
                    "-a",
                    "android.intent.action.MAIN",
                    "-c",
                    "android.intent.category.LAUNCHER"
                )
            }
        }
    }

    signingConfigs {
        getByName("debug") {
            storeFile = File("${rootProject.layout.projectDirectory}/app/comaps-debug.keystore")
            storePassword = "12345678"
            keyAlias = "CoMaps Debug"
            keyPassword = "12345678"
        }
        create("test") {
            storeFile = File(project.ext["secretTestStoreFile"] as String)
            storePassword = project.ext["secretTestStorePassword"] as String
            keyAlias = project.ext["secretTestKeyAlias"] as String
            keyPassword = project.ext["secretTestKeyPassword"] as String
        }
        create("release") {
            storeFile = File(project.ext["secretReleaseStoreFile"] as String)
            storePassword = project.ext["secretReleaseStorePassword"] as String
            keyAlias = project.ext["secretReleaseKeyAlias"] as String
            keyPassword = project.ext["secretReleaseKeyPassword"] as String
        }
    }

    buildTypes {
        val taskName = getGradle().startParameter.taskRequests.toString().lowercase(Locale.getDefault())

        debug {
            applicationIdSuffix = ".debug" // Allows installing debug and release builds together.
            versionNameSuffix = "-debug"
            signingConfig = signingConfigs["debug"]
            resValue("string", "app_name", "CoMaps Debug")
        }
        release {
            if (taskName.contains("release")) {
                if (secureReleasePropertiesFileExists) {
                    println("Using RELEASE signing keys from secure.properties.release")
                    signingConfig = signingConfigs["release"]
                } else {
                    println("NO RELEASE signing keys found")
                    println("Using DEBUG signing keys")
                    signingConfig = signingConfigs["debug"]
                }
            }

            isMinifyEnabled = true
            isShrinkResources = true
            proguardFiles(getDefaultProguardFile("proguard-android-optimize.txt"), "proguard-rules.pro")
            resValue("string", "app_name", project.name)
        }

        create("beta") {
            applicationIdSuffix = ".test"
            versionNameSuffix = "-test"
            if (taskName.contains("beta")) {
                if (secureTestPropertiesFileExists) {
                    println("Using TEST signing keys from secure.properties.test")
                    signingConfig = signingConfigs["test"]
                } else {
                    println("NO TEST signing keys found")
                    println("Using DEBUG signing keys")
                    signingConfig = signingConfigs["debug"]
                }
            }
            isMinifyEnabled = true
            isShrinkResources = true
            proguardFiles(getDefaultProguardFile("proguard-android-optimize.txt"), "proguard-rules.pro")
            matchingFallbacks += "release" // Use dependencies of "release" build type.
            resValue("string", "app_name", "CoMaps Test")
        }
    }

    // We don't compress these extensions in assets/ because our random FileReader can't read zip-compressed files from apk.
    // TODO: Load all minor files via separate call to ReadAsString which can correctly handle compressed files in zip containers.
    androidResources {
        ignoreAssetsPattern = "!.svn:!.git:!.DS_Store:!*.scc:.*:<dir>_*:!CVS:!thumbs.db:!picasa.ini:!*~"
        noCompress += listOf("txt", "bin", "html", "png", "json", "mwm", "ttf", "sdf", "ui", "config", "csv", "spv", "obj")
        // Some languages not supported by Android require to be specified here to be applied
        localeFilters += listOf(
            "en",
            "af",
            "ar",
            "az",
            "be",
            "bg",
            "bn",
            "ca",
            "cs",
            "da",
            "de",
            "el",
            "en-rAU",
            "en-rGB",
            "es",
            "es-rMX",
            "et",
            "eu",
            "fa",
            "fi",
            "fr",
            "fr-rCA",
            "gl",
            "gsw",
            "he",
            "hi",
            "hu",
            "id",
            "in",
            "is",
            "it",
            "iw",
            "ja",
            "kw",
            "ko",
            "lt",
            "lv",
            "mr",
            "mt",
            "nb",
            "nb-rNO",
            "nl",
            "pl",
            "pt",
            "pt-rBR",
            "ro",
            "ru",
            "sl",
            "sk",
            "sr",
            "b+sr+Latn",
            "sv",
            "sw",
            "ta",
            "th",
            "tr",
            "uk",
            "vi",
            "zh",
            "zh-rHK",
            "zh-rMO",
            "zh-rTW",
        )
    }

    compileOptions {
        isCoreLibraryDesugaringEnabled = true
        sourceCompatibility = JavaVersion.VERSION_21
        targetCompatibility = JavaVersion.VERSION_21

    }
}

dependencies {
    implementation(project(":sdk"))
    coreLibraryDesugaring(libs.android.tools.desugar)

    // Google Play Location Services
    // TODO(@pastk): enabled via microG in all flavors,
    // so move google/java/app/organicmaps/location/* into main/ and remove symlinks.
    //
    // Please add symlinks to google/java/app/organicmaps/location for each new gms-enabled flavor below:
    // ```
    // mkdir -p src/$flavor/java/app/organicmaps/
    // ln -sf ../../../../google/java/app/organicmaps/location src/$flavor/java/app/organicmaps/
    // ls -la src/$flavor/java/app/organicmaps/location/GoogleFusedLocationProvider.java
    // ```
    //
    // microG project's FOSS re-implementation of the proprietary libs.google.services.location
    implementation(libs.microg.services.location)
    implementation(libs.androidx.core)
    implementation(platform(libs.jetbrains.kotlin.bom))
    implementation(libs.androidx.annotation)
    implementation(libs.androidx.appcompat)
    implementation(libs.androidx.car.app)
    implementation(libs.androidx.car.app.projected)
    implementation(libs.androidx.constraintlayout)
    implementation(libs.androidx.fragment)
    implementation(libs.androidx.preference)
    implementation(libs.androidx.recyclerview)
    implementation(libs.androidx.work.runtime)
    implementation(libs.androidx.lifecycle.process)
    implementation(libs.androidx.documentfile)
    // 1.13 Material library version doesn't render properly alpha properties on map buttons
    implementation(libs.android.material)
    // Fix for app/organicmaps/util/FileUploadWorker.java:14: error: cannot access ListenableFuture
    // https://github.com/organicmaps/organicmaps/issues/6106
    implementation(libs.google.guava)
    implementation(libs.appdevnext.androidchart)

    // Test Dependencies
    androidTestImplementation(libs.androidx.test.junit)
    testImplementation(libs.junit)
    testImplementation(libs.mockito.core)
}

android.applicationVariants.configureEach {
    val authorityValue = "$applicationId.provider"
    buildConfigField("String", "FILE_PROVIDER_AUTHORITY", "\"$authorityValue\"")
    getMergedFlavor().manifestPlaceholders["FILE_PROVIDER_PLACEHOLDER"] = authorityValue
    resValue("string", "app_id", applicationId)
}

play {
    enabled = false
    track = "production"
    defaultToAppBundles = true
    releaseStatus = ReleaseStatus.IN_PROGRESS
    userFraction = 0.2 // Rollout to 20% of users.
    serviceAccountCredentials = File("google-play.json")
}

huaweiPublish {
    instances {
        create("huaweiRelease") {
            credentialsPath = "$projectDir/huawei-appgallery.json"
            buildFormat = BuildFormat.AAB
            deployType = DeployType.DRAFT
            val releaseDescriptions = mutableListOf<ReleaseNote>()
            val localeOverride = mapOf(
                "am" to "am-ET",
                "gu" to "gu_IN",
                "iw-IL" to "he_IL",
                "kn-IN" to "kn_IN",
                "ml-IN" to "ml_IN",
                "mn-MN" to "mn_MN",
                "mr-IN" to "mr_IN",
                "ta-IN" to "ta_IN",
                "te-IN" to "te_IN",
            )
            fileTree(baseDir = "$projectDir/src/fdroid/play/listings")
                .matching { include("**/release-notes.txt") }
                .forEach { file ->
                    localeOverride[file.parentFile.name]?.let { name ->
                        releaseDescriptions += ReleaseNote(lang = name, filePath = file.path)
                    }
                }
            releaseNotes = ReleaseNotesExtension(descriptions = releaseDescriptions, removeHtmlTags = true)
        }
    }
}

kotlin {
    compilerOptions {
        freeCompilerArgs.addAll("-Xlint:unchecked", "-Xlint:deprecation")
    }
}

tasks.withType<JavaCompile>().configureEach {
    options.compilerArgs.addAll(listOf("-Xlint:unchecked", "-Xlint:deprecation"))
}
