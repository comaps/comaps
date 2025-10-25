# CoMaps Live Location Sharing Server

A Node.js server for real-time, end-to-end encrypted location sharing with web client viewer.

## Features

- **End-to-end encryption**: Location data is encrypted on the device using AES-GCM-256
- **Real-time updates**: Poll-based location updates every 5 seconds
- **Session management**: Automatic cleanup of expired sessions
- **Web viewer**: Interactive map viewer with Leaflet.js
- **Navigation support**: Display ETA, distance, and destination when navigating
- **Battery monitoring**: Shows battery level and warnings
- **Location history**: Trail visualization with polyline

## Quick Start

### Installation

```bash
cd webapp
npm install
```

### Configuration

Copy `.env.example` to `.env` and configure:

```bash
cp .env.example .env
```

Edit `.env`:
```
PORT=3000
NODE_ENV=development
DATABASE_PATH=./location_sharing.db
SESSION_EXPIRY_HOURS=24
CLEANUP_INTERVAL_MINUTES=60
```

### Running the Server

Development mode (with auto-reload):
```bash
npm run dev
```

Production mode:
```bash
npm start
```

The server will start on `http://localhost:3000`

## API Documentation

### Create/Reactivate Session

```
POST /api/sessions
Content-Type: application/json

{
  "sessionId": "uuid-string"
}
```

### Store Location Update

```
POST /api/sessions/:sessionId/location
Content-Type: application/json

{
  "encryptedPayload": "{\"iv\":\"...\",\"ciphertext\":\"...\",\"authTag\":\"...\"}"
}
```

### Get Session Info

```
GET /api/sessions/:sessionId
```

### Get Latest Location

```
GET /api/sessions/:sessionId/location/latest
```

### Get Location History

```
GET /api/sessions/:sessionId/location/history?limit=100
```

### Stop Session

```
DELETE /api/sessions/:sessionId
```

### Server Statistics

```
GET /api/stats
```

## URL Format

Share URLs are formatted as:
```
https://your-server.com/live/{base64url(sessionId:encryptionKey)}
```

Example:
```
https://live.organicmaps.app/live/MjAyM...c3NrZXk
```

## Database Schema

### sessions
- `session_id` (TEXT, PRIMARY KEY)
- `created_at` (INTEGER)
- `last_update` (INTEGER)
- `expires_at` (INTEGER)
- `is_active` (INTEGER)

### location_updates
- `id` (INTEGER, PRIMARY KEY AUTOINCREMENT)
- `session_id` (TEXT, FOREIGN KEY)
- `encrypted_payload` (TEXT)
- `timestamp` (INTEGER)

## Encryption

Location data is encrypted using AES-GCM-256 with:
- Random 96-bit IV
- 128-bit authentication tag
- Base64-encoded encryption key (256-bit)

Encrypted payload format (JSON):
```json
{
  "iv": "base64-encoded-iv",
  "ciphertext": "base64-encoded-ciphertext",
  "authTag": "base64-encoded-auth-tag"
}
```

Decrypted payload format (JSON):
```json
{
  "timestamp": 1234567890,
  "lat": 37.7749,
  "lon": -122.4194,
  "accuracy": 10.5,
  "speed": 5.2,
  "bearing": 45.0,
  "mode": "navigation",
  "eta": 1234567900,
  "distanceRemaining": 5000,
  "destinationName": "Home",
  "batteryLevel": 75
}
```

## Web Viewer

The web viewer is accessible at `/live/{encodedCredentials}` and provides:

- Interactive map with Leaflet.js and OpenStreetMap tiles
- Real-time location marker with accuracy circle
- Location trail visualization
- Info panel showing:
  - Status (live/inactive)
  - Coordinates
  - Accuracy
  - Speed (if available)
  - Navigation info (ETA, distance, destination)
  - Battery level
- Automatic polling every 5 seconds
- Responsive design (mobile-friendly)

## Security Considerations

1. **HTTPS Required**: Always use HTTPS in production to protect the share URLs
2. **Encryption Keys**: Never log or expose encryption keys server-side
3. **Session Expiry**: Sessions automatically expire after 24 hours (configurable)
4. **Rate Limiting**: Consider adding rate limiting for production deployments
5. **CORS**: Configure CORS appropriately for your deployment

## Production Deployment

### Using PM2

```bash
npm install -g pm2
pm2 start src/server.js --name location-sharing
pm2 save
pm2 startup
```

### Using Docker

```dockerfile
FROM node:18-alpine
WORKDIR /app
COPY package*.json ./
RUN npm ci --production
COPY . .
EXPOSE 3000
CMD ["node", "src/server.js"]
```

### Environment Variables

Set these in production:
```
NODE_ENV=production
PORT=3000
DATABASE_PATH=/data/location_sharing.db
SESSION_EXPIRY_HOURS=24
```

## Maintenance

### Database Cleanup

The server automatically cleans up:
- Expired sessions (older than expiry time)
- Inactive sessions (older than 1 week)
- Old location updates (older than 1 week)

Cleanup runs every 60 minutes by default (configurable).

### Manual Cleanup

```bash
# Stop the server
# Delete the database file
rm location_sharing.db
# Restart the server (will recreate schema)
```

## Troubleshooting

### Location not updating
- Check that the session is active
- Verify the encrypted payload format
- Check server logs for decryption errors
- Ensure the encryption key matches

### Web viewer shows error
- Verify the share URL is correct
- Check that the session exists
- Ensure at least one location update has been sent
- Check browser console for decryption errors

## License

MIT
