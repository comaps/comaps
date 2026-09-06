# Screen reader support

## Android

The Android app is mostly accessible to screen readers.

Things to watch out for during development:
- unlabeled drawables
- focus traps with no close button
- modal dialogs that don't trap focus

## iOS

The iOS app has significant accessibility issues,
unfortunately we don't have enough developers to fix this.

If you'd like to help out, it's basically a matter of trying out each
part of the app with VoiceOver and making sure everything is labeled,
meaningful and focuseable as needed.

For adding support to the main map, you'll need to translate
android/sdk/src/main/java/app/.../MapViewAccessibilityDelegate.java
into iOS-speak.

Please see CONTRIBUTING.md and get in touch
via Zulip if you want to help!

## Core (drape) accessibility

The drape render engine supports exposing semantic information about
what is shown on the screen. This is used by screen readers to allow
users to explore the map by touch and TTS.

### Editing the accessibility tree

Every OverlayHandle contains an accessibility node.
Some things get rendered currently without an overlay handle,
so to make them available to accessibility you should give them
an overlay handle.

Alternatively if a synthetic or out-of-bucket node needs to be
added to the accessibility tree, you can do this in
`FrontendRenderer::RenderFrame`.

### How it works and extending the capabilities

Most of the interesting code is in `dp/accessiblity_node_info.hpp`
and `dp/accessibility_presenter.cpp`.

Accessibility nodes are created and stored in their overlays on the
Render thread and pushed through a channel (with conflation)
to the Gui thread, so the platform can query them near-instantly.

If you want to add new fields to AccessibilityNodeInfo that's fine,
but make sure they don't contain any pointers, and please
don't subclass it (because it needs to be copied between threads
and if subclasses exist then someone will certainly add an
unprotected pointer one day).

Data originates in `rule_drawer.cpp` and
`apply_feature_functors.cpp`. These instantiate a Shape and a
Handle, the handle receives the data and is added to a bucket by the
shape. The `FrontendRenderer`, after each frame, will gather up the
node info of each handle, and atomically send it to the `Gui` thread.
The `Gui` thread (in `drape_engine.cpp`) moves the data into
thread-local storage and notifies the AccessibilityPresenter,
which triggers the platform's callbacks.
