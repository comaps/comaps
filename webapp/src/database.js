const Database = require('better-sqlite3');
const path = require('path');

class LocationDatabase {
  constructor(dbPath) {
    this.db = new Database(dbPath || path.join(__dirname, '..', 'location_sharing.db'));
    this.initSchema();
    this.setupCleanup();
  }

  initSchema() {
    // Create sessions table
    this.db.exec(`
      CREATE TABLE IF NOT EXISTS sessions (
        session_id TEXT PRIMARY KEY,
        created_at INTEGER NOT NULL,
        last_update INTEGER NOT NULL,
        expires_at INTEGER NOT NULL,
        is_active INTEGER DEFAULT 1
      );

      CREATE INDEX IF NOT EXISTS idx_sessions_active
        ON sessions(is_active, expires_at);
    `);

    // Create location_updates table
    this.db.exec(`
      CREATE TABLE IF NOT EXISTS location_updates (
        id INTEGER PRIMARY KEY AUTOINCREMENT,
        session_id TEXT NOT NULL,
        encrypted_payload TEXT NOT NULL,
        timestamp INTEGER NOT NULL,
        FOREIGN KEY (session_id) REFERENCES sessions(session_id) ON DELETE CASCADE
      );

      CREATE INDEX IF NOT EXISTS idx_location_session
        ON location_updates(session_id, timestamp DESC);
    `);
  }

  /**
   * Create a new session
   */
  createSession(sessionId, expiryHours = 24) {
    const now = Date.now();
    const expiresAt = now + (expiryHours * 60 * 60 * 1000);

    const stmt = this.db.prepare(`
      INSERT INTO sessions (session_id, created_at, last_update, expires_at, is_active)
      VALUES (?, ?, ?, ?, 1)
    `);

    try {
      stmt.run(sessionId, now, now, expiresAt);
      return { success: true, sessionId };
    } catch (err) {
      if (err.code === 'SQLITE_CONSTRAINT') {
        // Session already exists, update it
        return this.reactivateSession(sessionId, expiryHours);
      }
      throw err;
    }
  }

  /**
   * Reactivate an existing session
   */
  reactivateSession(sessionId, expiryHours = 24) {
    const now = Date.now();
    const expiresAt = now + (expiryHours * 60 * 60 * 1000);

    const stmt = this.db.prepare(`
      UPDATE sessions
      SET is_active = 1, last_update = ?, expires_at = ?
      WHERE session_id = ?
    `);

    const result = stmt.run(now, expiresAt, sessionId);
    return {
      success: result.changes > 0,
      sessionId,
      reactivated: true
    };
  }

  /**
   * Store a location update
   */
  storeLocationUpdate(sessionId, encryptedPayload) {
    const now = Date.now();

    // First, check if session exists and is active
    const session = this.getSession(sessionId);
    if (!session || !session.is_active || session.expires_at < now) {
      return { success: false, error: 'Session not found or expired' };
    }

    // Update session last_update timestamp
    const updateSession = this.db.prepare(`
      UPDATE sessions SET last_update = ? WHERE session_id = ?
    `);
    updateSession.run(now, sessionId);

    // Insert location update
    const stmt = this.db.prepare(`
      INSERT INTO location_updates (session_id, encrypted_payload, timestamp)
      VALUES (?, ?, ?)
    `);

    try {
      const result = stmt.run(sessionId, encryptedPayload, now);
      return {
        success: true,
        updateId: result.lastInsertRowid
      };
    } catch (err) {
      return { success: false, error: err.message };
    }
  }

  /**
   * Get session info
   */
  getSession(sessionId) {
    const stmt = this.db.prepare(`
      SELECT * FROM sessions WHERE session_id = ?
    `);
    return stmt.get(sessionId);
  }

  /**
   * Get latest location update for a session
   */
  getLatestLocation(sessionId) {
    const stmt = this.db.prepare(`
      SELECT encrypted_payload, timestamp
      FROM location_updates
      WHERE session_id = ?
      ORDER BY timestamp DESC
      LIMIT 1
    `);
    return stmt.get(sessionId);
  }

  /**
   * Get location history for a session
   */
  getLocationHistory(sessionId, limit = 100) {
    const stmt = this.db.prepare(`
      SELECT encrypted_payload, timestamp
      FROM location_updates
      WHERE session_id = ?
      ORDER BY timestamp DESC
      LIMIT ?
    `);
    return stmt.all(sessionId, limit);
  }

  /**
   * Stop a session
   */
  stopSession(sessionId) {
    const stmt = this.db.prepare(`
      UPDATE sessions SET is_active = 0 WHERE session_id = ?
    `);
    const result = stmt.run(sessionId);
    return { success: result.changes > 0 };
  }

  /**
   * Clean up expired sessions and old location updates
   */
  cleanup() {
    const now = Date.now();
    const oneWeekAgo = now - (7 * 24 * 60 * 60 * 1000);

    // Mark expired sessions as inactive
    const expireStmt = this.db.prepare(`
      UPDATE sessions
      SET is_active = 0
      WHERE expires_at < ? AND is_active = 1
    `);
    const expired = expireStmt.run(now);

    // Delete old inactive sessions (older than 1 week)
    const deleteSessionsStmt = this.db.prepare(`
      DELETE FROM sessions
      WHERE is_active = 0 AND expires_at < ?
    `);
    const deletedSessions = deleteSessionsStmt.run(oneWeekAgo);

    // Delete orphaned location updates (sessions deleted by CASCADE)
    // This is automatic due to CASCADE, but we can also delete old updates
    const deleteUpdatesStmt = this.db.prepare(`
      DELETE FROM location_updates
      WHERE timestamp < ?
    `);
    const deletedUpdates = deleteUpdatesStmt.run(oneWeekAgo);

    return {
      expiredSessions: expired.changes,
      deletedSessions: deletedSessions.changes,
      deletedUpdates: deletedUpdates.changes
    };
  }

  /**
   * Setup automatic cleanup
   */
  setupCleanup(intervalMinutes = 60) {
    setInterval(() => {
      const result = this.cleanup();
      console.log('[DB Cleanup]', new Date().toISOString(), result);
    }, intervalMinutes * 60 * 1000);
  }

  /**
   * Get database statistics
   */
  getStats() {
    const sessionCount = this.db.prepare(`
      SELECT COUNT(*) as count FROM sessions WHERE is_active = 1
    `).get();

    const totalUpdates = this.db.prepare(`
      SELECT COUNT(*) as count FROM location_updates
    `).get();

    return {
      activeSessions: sessionCount.count,
      totalLocationUpdates: totalUpdates.count
    };
  }

  close() {
    this.db.close();
  }
}

module.exports = LocationDatabase;
