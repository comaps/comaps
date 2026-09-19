package app.organicmaps.crashhandler

import android.content.Intent
import android.content.SharedPreferences
import android.os.Bundle
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.activity.result.ActivityResultLauncher
import androidx.compose.animation.AnimatedVisibility
import androidx.compose.foundation.Image
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.aspectRatio
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.widthIn
import androidx.compose.material3.Button
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.OutlinedButton
import androidx.compose.material3.Scaffold
import androidx.compose.material3.Text
import androidx.compose.material3.Icon
import androidx.compose.material3.LocalContentColor
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.produceState
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.res.painterResource
import androidx.compose.ui.res.stringResource
import androidx.compose.ui.unit.dp
import androidx.core.content.edit
import androidx.lifecycle.compose.collectAsStateWithLifecycle
import app.organicmaps.MwmActivity
import app.organicmaps.R
import app.organicmaps.sdk.util.log.LogsManager
import app.organicmaps.ui.theme.CoMapsTheme
import app.organicmaps.util.SharingUtils
import app.organicmaps.util.Utils
import app.organicmaps.util.getFlow
import app.organicmaps.util.SwitchWithLabel
import app.organicmaps.util.annotatedStringResource
import java.io.File

class CrashHandlerActivity : ComponentActivity() {
    // updated in onResume, since the core code doesn't expose a callback for changes
    private var launcher: ActivityResultLauncher<SharingUtils.SharingIntent>? = null

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        launcher = SharingUtils.RegisterLauncher(this)
        val prefs = getSharedPreferences(getString(app.organicmaps.sdk.R.string.pref_file_name), MODE_PRIVATE)
        LogsManager.INSTANCE.initFileLoggingReadOnly(this, prefs)
        setContent {
            val loggingEnabled by produceState<Boolean?>(initialValue = null) {
                prefs
                    .getFlow(this@produceState, getString(app.organicmaps.sdk.R.string.pref_enable_logging), false, SharedPreferences::getBoolean)
                    .collect { value = it }
            }

            CoMapsTheme {
                CrashHandler(
                    loggingEnabled = loggingEnabled,
                    enableLogging = {
                        prefs.edit {
                            putBoolean(getString(app.organicmaps.sdk.R.string.pref_enable_logging), true)
                        }
                    }
                )
            }
        }
    }

    @Composable
    private fun CrashHandler(loggingEnabled: Boolean?, enableLogging: () -> Unit) {
        var loggingWasEverShown by remember { mutableStateOf(false) }
        // TODO switch to new SideEffect(key, effect) api - just needs replacing once we upgrade compose
        LaunchedEffect(loggingEnabled) { loggingWasEverShown = loggingWasEverShown || loggingEnabled == false }

        Scaffold { innerPadding ->
            Column(
                modifier = Modifier
                    .padding(innerPadding)
                    .padding(32.dp),
                verticalArrangement = Arrangement.spacedBy(16.dp),
                horizontalAlignment = Alignment.CenterHorizontally,
            ) {
                CrashLogo(Modifier.widthIn(max = 192.dp).fillMaxWidth(0.7f).aspectRatio(1.0f))
                Text(
                    text = stringResource(R.string.crashed),
                    style = MaterialTheme.typography.headlineMedium,
                )
                Text(
                    annotatedStringResource(R.string.crash_explainer),
                )
                if (loggingWasEverShown) {
                    val loggingEnabled = loggingEnabled ?: false
                    Text(stringResource(R.string.crash_enable_logging))
                    SwitchWithLabel(
                        modifier = Modifier.fillMaxWidth(),
                        label = { Text(stringResource(id = R.string.enable_logging)) },
                        state = loggingEnabled,
                        enabled = !loggingEnabled,
                        onStateChange = { enableLogging() }
                    )
                }
                Button(
                    modifier = Modifier.fillMaxWidth(),
                    onClick = ::sendReport
                ) {
                    Text(stringResource(R.string.send_report))
                }
                OutlinedButton(
                    modifier = Modifier.fillMaxWidth(),
                    onClick = ::restart
                ) {
                    Text(stringResource(R.string.restart))
                }
            }
        }
    }

    private fun findLogs(): List<File> {
        return File(CrashHandler.getReportDir(this)).listFiles {
            it.name.matches(Regex("""crash_\d+_\d+_(java|native)\.txt"""))
        }?.toList() ?: emptyList()
    }

    @Composable
    private fun CrashLogo(modifier: Modifier) {
        Icon(painterResource(R.drawable.warning_icon), null, modifier = modifier, tint = LocalContentColor.current.copy(alpha = 0.5f))
    }

    private fun restart() {
        findLogs().forEach { it.delete() }
        val intent = Intent(this, MwmActivity::class.java)
        intent.setFlags(Intent.FLAG_ACTIVITY_CLEAR_TOP or Intent.FLAG_ACTIVITY_NEW_TASK)
        startActivity(intent)
        finish()
    }

    private fun sendReport() {
        Utils.sendCrashReport(launcher!!, this, "", "", findLogs().map { it.absolutePath })
    }
}
