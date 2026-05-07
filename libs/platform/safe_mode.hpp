#pragma once

namespace safe_mode
{
// Call once at startup, after platform paths are initialized but before Framework is constructed.
// Checks for a sentinel file left by a previous crash during map loading.
// Returns true if crash recovery was detected (safe mode is now active).
bool Init();

// Returns true after Init() detected a previous crash.
bool IsActive();

// Write sentinel synchronously before any map file is opened.
// fwrite + fflush ensures the file survives a native crash (SIGSEGV).
void MarkLoadStarted();

// Delete sentinel after rendering is fully live.
// If this is never reached (crash), sentinel persists and triggers safe mode on next start.
void MarkLoadComplete();
}  // namespace safe_mode
