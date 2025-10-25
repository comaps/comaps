require('dotenv').config();
const express = require('express');
const cors = require('cors');
const path = require('path');
const LocationDatabase = require('./database');

const app = express();
const PORT = process.env.PORT || 3000;

// Initialize database
const db = new LocationDatabase(process.env.DATABASE_PATH);

// Middleware
app.use(cors());
app.use(express.json());
app.use(express.static(path.join(__dirname, '..', 'public')));

// Request logging middleware
app.use((req, res, next) => {
  console.log(`[${new Date().toISOString()}] ${req.method} ${req.path}`);
  next();
});

/**
 * API: Create or reactivate a session
 * POST /api/v1/session (Android app format)
 * POST /api/sessions (legacy format)
 * Body: { sessionId: string }
 */
const createSessionHandler = (req, res) => {
  const { sessionId } = req.body;

  if (!sessionId) {
    return res.status(400).json({ error: 'sessionId is required' });
  }

  try {
    const expiryHours = parseInt(process.env.SESSION_EXPIRY_HOURS) || 24;
    const result = db.createSession(sessionId, expiryHours);
    res.json(result);
  } catch (err) {
    console.error('Error creating session:', err);
    res.status(500).json({ error: 'Internal server error' });
  }
};

app.post('/api/v1/session', createSessionHandler);
app.post('/api/sessions', createSessionHandler);

/**
 * API: Store a location update
 * POST /api/v1/location/:sessionId (Android app format - encrypted payload is the entire body)
 * Body: encrypted payload JSON directly
 */
app.post('/api/v1/location/:sessionId', (req, res) => {
  const { sessionId } = req.params;

  // Android sends the encrypted payload JSON directly as the request body
  const encryptedPayload = JSON.stringify(req.body);

  if (!encryptedPayload || encryptedPayload === '{}') {
    return res.status(400).json({ error: 'Encrypted payload is required' });
  }

  try {
    const result = db.storeLocationUpdate(sessionId, encryptedPayload);

    if (!result.success) {
      return res.status(404).json({ error: result.error });
    }

    res.json(result);
  } catch (err) {
    console.error('Error storing location:', err);
    res.status(500).json({ error: 'Internal server error' });
  }
});

/**
 * API: Store a location update (legacy format)
 * POST /api/sessions/:sessionId/location
 * Body: { encryptedPayload: string }
 */
app.post('/api/sessions/:sessionId/location', (req, res) => {
  const { sessionId } = req.params;
  const { encryptedPayload } = req.body;

  if (!encryptedPayload) {
    return res.status(400).json({ error: 'encryptedPayload is required' });
  }

  try {
    const result = db.storeLocationUpdate(sessionId, encryptedPayload);

    if (!result.success) {
      return res.status(404).json({ error: result.error });
    }

    res.json(result);
  } catch (err) {
    console.error('Error storing location:', err);
    res.status(500).json({ error: 'Internal server error' });
  }
});

/**
 * API: Get session info
 * GET /api/v1/session/:sessionId (Android app format)
 * GET /api/sessions/:sessionId (legacy format)
 */
const getSessionHandler = (req, res) => {
  const { sessionId } = req.params;

  try {
    const session = db.getSession(sessionId);

    if (!session) {
      return res.status(404).json({ error: 'Session not found' });
    }

    // Don't expose internal fields, just status
    res.json({
      sessionId: session.session_id,
      isActive: session.is_active === 1,
      createdAt: session.created_at,
      lastUpdate: session.last_update,
      expiresAt: session.expires_at
    });
  } catch (err) {
    console.error('Error getting session:', err);
    res.status(500).json({ error: 'Internal server error' });
  }
};

app.get('/api/v1/session/:sessionId', getSessionHandler);
app.get('/api/sessions/:sessionId', getSessionHandler);

/**
 * API: Get latest location for a session
 * GET /api/v1/location/:sessionId/latest (Android app format)
 * GET /api/sessions/:sessionId/location/latest (legacy format)
 */
const getLatestLocationHandler = (req, res) => {
  const { sessionId } = req.params;

  try {
    const location = db.getLatestLocation(sessionId);

    if (!location) {
      return res.status(404).json({ error: 'No location data found' });
    }

    res.json({
      encryptedPayload: location.encrypted_payload,
      timestamp: location.timestamp
    });
  } catch (err) {
    console.error('Error getting location:', err);
    res.status(500).json({ error: 'Internal server error' });
  }
};

app.get('/api/v1/location/:sessionId/latest', getLatestLocationHandler);
app.get('/api/sessions/:sessionId/location/latest', getLatestLocationHandler);

/**
 * API: Get location history for a session
 * GET /api/sessions/:sessionId/location/history?limit=100
 */
app.get('/api/sessions/:sessionId/location/history', (req, res) => {
  const { sessionId } = req.params;
  const limit = parseInt(req.query.limit) || 100;

  try {
    const history = db.getLocationHistory(sessionId, limit);

    res.json({
      count: history.length,
      updates: history.map(loc => ({
        encryptedPayload: loc.encrypted_payload,
        timestamp: loc.timestamp
      }))
    });
  } catch (err) {
    console.error('Error getting history:', err);
    res.status(500).json({ error: 'Internal server error' });
  }
});

/**
 * API: Stop a session
 * DELETE /api/v1/session/:sessionId (Android app format)
 * DELETE /api/sessions/:sessionId (legacy format)
 */
const deleteSessionHandler = (req, res) => {
  const { sessionId } = req.params;

  try {
    const result = db.stopSession(sessionId);

    if (!result.success) {
      return res.status(404).json({ error: 'Session not found' });
    }

    res.json({ success: true, message: 'Session stopped' });
  } catch (err) {
    console.error('Error stopping session:', err);
    res.status(500).json({ error: 'Internal server error' });
  }
};

app.delete('/api/v1/session/:sessionId', deleteSessionHandler);
app.delete('/api/sessions/:sessionId', deleteSessionHandler);

/**
 * API: Get server statistics
 * GET /api/stats
 */
app.get('/api/stats', (req, res) => {
  try {
    const stats = db.getStats();
    res.json(stats);
  } catch (err) {
    console.error('Error getting stats:', err);
    res.status(500).json({ error: 'Internal server error' });
  }
});

/**
 * Serve the viewer page for a specific session
 * GET /live/:encodedCredentials
 */
app.get('/live/:encodedCredentials', (req, res) => {
  res.sendFile(path.join(__dirname, '..', 'public', 'viewer.html'));
});

/**
 * Health check endpoint
 */
app.get('/health', (req, res) => {
  res.json({ status: 'ok', timestamp: Date.now() });
});

// 404 handler
app.use((req, res) => {
  res.status(404).json({ error: 'Not found' });
});

// Error handler
app.use((err, req, res, next) => {
  console.error('Unhandled error:', err);
  res.status(500).json({ error: 'Internal server error' });
});

// Start server
app.listen(PORT, () => {
  console.log(`Location sharing server running on port ${PORT}`);
  console.log(`Environment: ${process.env.NODE_ENV}`);
  console.log(`Database: ${process.env.DATABASE_PATH || 'location_sharing.db'}`);
  console.log(`\nServer stats:`, db.getStats());
});

// Graceful shutdown
process.on('SIGINT', () => {
  console.log('\nShutting down gracefully...');
  db.close();
  process.exit(0);
});

process.on('SIGTERM', () => {
  console.log('\nShutting down gracefully...');
  db.close();
  process.exit(0);
});
