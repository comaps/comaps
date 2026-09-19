package app.organicmaps.util

import android.content.SharedPreferences
import androidx.annotation.StringRes
import androidx.compose.foundation.clickable
import androidx.compose.foundation.interaction.MutableInteractionSource
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.runtime.Composable
import androidx.compose.runtime.remember
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.selection.selectable
import androidx.compose.foundation.selection.toggleable
import androidx.compose.material3.Text
import androidx.compose.material3.Switch
import androidx.compose.material3.MaterialTheme
import androidx.compose.ui.Modifier
import androidx.compose.ui.semantics.Role
import androidx.compose.ui.unit.dp
import androidx.compose.ui.Alignment
import androidx.compose.ui.res.stringResource
import androidx.compose.ui.semantics.clearAndSetSemantics
import androidx.compose.ui.semantics.role
import androidx.compose.ui.semantics.selected
import androidx.compose.ui.semantics.semantics
import androidx.compose.ui.semantics.text
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.text.AnnotatedString
import androidx.compose.ui.text.SpanStyle
import androidx.compose.ui.text.TextLinkStyles
import androidx.compose.ui.text.font.FontStyle
import androidx.compose.ui.text.fromHtml
import androidx.compose.ui.text.style.TextDecoration
import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.channels.Channel
import kotlinx.coroutines.channels.awaitClose
import kotlinx.coroutines.flow.Flow
import kotlinx.coroutines.flow.buffer
import kotlinx.coroutines.flow.callbackFlow
import kotlinx.coroutines.flow.stateIn


suspend fun <T>SharedPreferences.getFlow(scope: CoroutineScope, key: String, default: T, getter: SharedPreferences.(String, T) -> T): Flow<T> = callbackFlow {
    val listener = SharedPreferences.OnSharedPreferenceChangeListener { _, changedKey ->
        if (changedKey == key) {
            trySend(getter(key, default))
        }
    }
    registerOnSharedPreferenceChangeListener(listener)
    if (contains(key)) {
        send(getter(key, default))
    }
    awaitClose { unregisterOnSharedPreferenceChangeListener(listener) }
}.buffer(Channel.UNLIMITED).stateIn(scope)

// Source - https://stackoverflow.com/a/73076422
// Posted by Thracian, modified by community. See post 'Timeline' for change history
// Retrieved 2026-09-20, License - CC BY-SA 4.0
// Modified 2026-09-20.
/**
 * A switch with a space for a label, set up so that you can click the label and it activates the switch.
 *
 * @param modifier the modifier. If you apply fillMaxWidth it will automatically insert a space between the label and the switch,
 *                 whereas wrapContentWidth will not do this.
 * @param label the label to show with the switch
 * @param state whether the switch is active
 * @param enabled whether the switch can be toggled
 * @param onStateChange callback when the switch is toggled (should usually change state via hoisting)
 */
@Composable
fun SwitchWithLabel(modifier: Modifier = Modifier, label: @Composable () -> Unit, state: Boolean, enabled: Boolean, onStateChange: (Boolean) -> Unit) {
    val interactionSource = remember { MutableInteractionSource() }
    Row(
        modifier = modifier
            .toggleable(
                value = state,
                interactionSource = interactionSource,
                // This is for removing ripple when Row is clicked
                indication = null,
                enabled = enabled,
                role = Role.Switch,
                onValueChange = {
                    onStateChange(!state)
                }
            )
            .padding(8.dp)
            .clearAndSetSemantics {},
        horizontalArrangement = Arrangement.SpaceBetween,
        verticalAlignment = Alignment.CenterVertically,
    ) {
        label()
        Spacer(modifier = Modifier.padding(start = 8.dp))
        Switch(
            checked = state,
            enabled = enabled,
            onCheckedChange = {
                onStateChange(it)
            },
        )
    }
}

/**
 * Like stringResource(), but treats the text as HTML.
 * Automatically underlines links.
 */
@Composable
fun annotatedStringResource(@StringRes id: Int, linkColor: Color = MaterialTheme.colorScheme.primary) =
    AnnotatedString.fromHtml(
        stringResource(id),
        linkStyles = TextLinkStyles(
            style = SpanStyle(
                textDecoration = TextDecoration.Underline,
                color = linkColor
            )
        )
    )
