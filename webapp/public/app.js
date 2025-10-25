/**
 * Live Location Viewer Application
 */

class LocationViewer {
  constructor() {
    this.map = null;
    this.marker = null;
    this.polyline = null;
    this.sessionId = null;
    this.encryptionKey = null;
    this.updateInterval = null;
    this.locationHistory = [];
    this.currentLocation = null;
    this.lastUpdateTimestamp = null;
    this.lastServerTimestamp = null;
    this.isActive = false;
    this.isFollowing = true;
    this.userMovedMap = false;
    this.lastNavigationMode = null;
    this.isInitialLoad = true;
  }

  /**
   * Initialize the application
   */
  async init() {
    try {
      // Parse credentials from URL
      const pathParts = window.location.pathname.split('/');
      const encodedCredentials = pathParts[pathParts.length - 1];

      const credentials = LocationCrypto.parseCredentials(encodedCredentials);
      if (!credentials) {
        this.showError('Invalid share link');
        return;
      }

      this.sessionId = credentials.sessionId;
      this.encryptionKey = credentials.encryptionKey;

      // Initialize map
      this.initMap();

      // Load location history first
      await this.loadLocationHistory();

      // Load latest location
      await this.loadLocation();

      // Check session status
      await this.checkSessionStatus();

      // Initial load is complete - now track user interactions
      setTimeout(() => {
        this.isInitialLoad = false;
      }, 1000);

      // Start polling for updates
      this.startPolling();

      // Show info panel
      document.getElementById('infoPanel').style.display = 'block';
      document.getElementById('loadingSpinner').style.display = 'none';
    } catch (err) {
      console.error('Initialization error:', err);
      this.showError(err.message || 'Failed to initialize viewer');
    }
  }

  /**
   * Initialize the Leaflet map
   */
  initMap() {
    this.map = L.map('map').setView([0, 0], 2);

    // Use CartoDB Positron - free minimalist black and white tileset
    L.tileLayer('https://{s}.basemaps.cartocdn.com/light_all/{z}/{x}/{y}{r}.png', {
      attribution: '&copy; <a href="https://www.openstreetmap.org/copyright">OpenStreetMap</a> contributors &copy; <a href="https://carto.com/attributions">CARTO</a>',
      subdomains: 'abcd',
      maxZoom: 20
    }).addTo(this.map);

    // Initialize polyline for location history
    this.polyline = L.polyline([], {
      color: '#2196f3',
      weight: 3,
      opacity: 0.7
    }).addTo(this.map);

    // Detect user map interaction (manual drag/zoom)
    this.map.on('dragstart', () => {
      if (!this.isInitialLoad) {
        this.userMovedMap = true;
        this.isFollowing = false;
        this.showFollowButton();
      }
    });

    this.map.on('zoomstart', () => {
      if (!this.isInitialLoad) {
        this.userMovedMap = true;
        this.isFollowing = false;
        this.showFollowButton();
      }
    });

    // Set up follow button
    const followButton = document.getElementById('followButton');
    followButton.addEventListener('click', () => {
      this.isFollowing = true;
      this.userMovedMap = false;
      this.hideFollowButton();
      if (this.currentLocation) {
        this.map.setView([this.currentLocation.lat, this.currentLocation.lon], 15);
      }
    });
  }

  /**
   * Show follow button
   */
  showFollowButton() {
    document.getElementById('followButton').classList.add('visible');
  }

  /**
   * Hide follow button
   */
  hideFollowButton() {
    document.getElementById('followButton').classList.remove('visible');
  }

  /**
   * Load location history from server
   */
  async loadLocationHistory(sinceTimestamp = null) {
    try {
      let url = `/api/sessions/${this.sessionId}/location/history?limit=100`;
      if (sinceTimestamp) {
        url += `&since=${sinceTimestamp}`;
      }

      const response = await fetch(url);

      if (!response.ok) {
        return; // History might not be available yet
      }

      const data = await response.json();
      const newLocations = [];

      for (const update of data.updates) {
        try {
          const decrypted = await LocationCrypto.decryptPayload(
            update.encryptedPayload,
            this.encryptionKey
          );

          // Check if this location is not already in history
          const isDuplicate = this.locationHistory.some(loc =>
            loc.lat === decrypted.lat &&
            loc.lon === decrypted.lon &&
            loc.timestamp === decrypted.timestamp
          );

          if (!isDuplicate) {
            newLocations.push({
              lat: decrypted.lat,
              lon: decrypted.lon,
              timestamp: decrypted.timestamp
            });
          }
        } catch (err) {
          console.error('Failed to decrypt historical location:', err);
        }
      }

      // Add new locations to history sorted by timestamp
      this.locationHistory.push(...newLocations);
      this.locationHistory.sort((a, b) => a.timestamp - b.timestamp);

      // Update polyline with historical data
      if (this.locationHistory.length > 0) {
        this.updatePolyline();

        // Center map on latest location only on first load and if following
        if (!sinceTimestamp && this.isFollowing) {
          const latestLoc = this.locationHistory[this.locationHistory.length - 1];
          this.map.setView([latestLoc.lat, latestLoc.lon], 15);
        }
      }
    } catch (err) {
      console.error('Load history error:', err);
    }
  }

  /**
   * Check session status
   */
  async checkSessionStatus() {
    try {
      const response = await fetch(`/api/v1/session/${this.sessionId}`);

      if (!response.ok) {
        this.isActive = false;
        return;
      }

      const data = await response.json();
      this.isActive = data.isActive;
    } catch (err) {
      console.error('Check session status error:', err);
      this.isActive = false;
    }
  }

  /**
   * Load location from server
   */
  async loadLocation() {
    try {
      const response = await fetch(`/api/v1/location/${this.sessionId}/latest`);

      if (!response.ok) {
        if (response.status === 404) {
          throw new Error('No location data available yet. Waiting for updates...');
        }
        throw new Error('Failed to load location');
      }

      const data = await response.json();
      const newTimestamp = data.timestamp;

      // Check for gap in updates (more than 30 seconds)
      if (this.lastServerTimestamp && (newTimestamp - this.lastServerTimestamp) > 30000) {
        console.log('Detected gap in updates, loading missing history...');
        await this.loadLocationHistory(this.lastServerTimestamp);
      }

      this.lastUpdateTimestamp = data.timestamp;
      this.lastServerTimestamp = newTimestamp;
      await this.processLocationUpdate(data);

      // Update session status on each location update
      await this.checkSessionStatus();
    } catch (err) {
      console.error('Load location error:', err);
      throw err;
    }
  }

  /**
   * Process an encrypted location update
   */
  async processLocationUpdate(data) {
    try {
      // Decrypt the payload
      const decrypted = await LocationCrypto.decryptPayload(
        data.encryptedPayload,
        this.encryptionKey
      );

      this.currentLocation = decrypted;

      // Update map
      this.updateMap(decrypted);

      // Update info panel
      this.updateInfoPanel(decrypted, data.timestamp);

      // Add to history only if it's not already there (avoid duplicates)
      const isDuplicate = this.locationHistory.some(loc =>
        loc.lat === decrypted.lat &&
        loc.lon === decrypted.lon &&
        loc.timestamp === decrypted.timestamp
      );

      if (!isDuplicate) {
        this.locationHistory.push({
          lat: decrypted.lat,
          lon: decrypted.lon,
          timestamp: decrypted.timestamp
        });

        // Update polyline only when we add a new point
        this.updatePolyline();
      }
    } catch (err) {
      console.error('Failed to process location update:', err);
      // Don't throw - just skip this update
    }
  }

  /**
   * Update map with new location
   */
  updateMap(location) {
    const latLng = [location.lat, location.lon];

    if (!this.marker) {
      // Create marker
      const icon = L.divIcon({
        className: 'location-marker',
        html: '<div style="background: #2196f3; width: 20px; height: 20px; border-radius: 50%; border: 3px solid white; box-shadow: 0 2px 4px rgba(0,0,0,0.3);"></div>',
        iconSize: [26, 26],
        iconAnchor: [13, 13]
      });

      this.marker = L.marker(latLng, { icon }).addTo(this.map);
      if (this.isFollowing) {
        this.map.setView(latLng, 15);
      }
    } else {
      // Update marker position
      this.marker.setLatLng(latLng);
    }

    // Add accuracy circle if available
    if (location.accuracy) {
      if (this.accuracyCircle) {
        this.accuracyCircle.setLatLng(latLng);
        this.accuracyCircle.setRadius(location.accuracy);
      } else {
        this.accuracyCircle = L.circle(latLng, {
          radius: location.accuracy,
          color: '#2196f3',
          fillColor: '#2196f3',
          fillOpacity: 0.1,
          weight: 1
        }).addTo(this.map);
      }
    }

    // Pan to new location only if following
    if (this.isFollowing) {
      this.map.panTo(latLng);
    }
  }

  /**
   * Update polyline with location history
   */
  updatePolyline() {
    const points = this.locationHistory.map(loc => [loc.lat, loc.lon]);
    this.polyline.setLatLngs(points);
  }

  /**
   * Update info panel
   */
  updateInfoPanel(location, serverTimestamp) {
    // Determine if session is truly live (updated within last 30 seconds)
    const now = Date.now();
    const timeSinceUpdate = now - serverTimestamp;
    const isLive = timeSinceUpdate < 30000;

    // Only show status if it's inactive AND there's old data OR session is explicitly closed
    let statusHtml = '';
    if (!isLive && timeSinceUpdate > 60000) {
      statusHtml = `
        <div class="info-item">
          <span class="info-label">Status</span>
          <span class="status inactive">Inactive</span>
        </div>
      `;
    } else if (isLive) {
      statusHtml = `
        <div class="info-item">
          <span class="info-label">Status</span>
          <span class="status active">Live</span>
        </div>
      `;
    }

    // Show speed if available
    if (location.speed !== undefined && location.speed >= 0) {
      statusHtml += `
        <div class="info-item">
          <span class="info-label">Speed</span>
          <span class="info-value">${(location.speed * 3.6).toFixed(1)} km/h</span>
        </div>
      `;
    }

    document.getElementById('statusInfo').innerHTML = statusHtml;

    // Navigation info - only show if currently in navigation mode
    if (location.mode === 'navigation' && location.eta) {
      this.lastNavigationMode = true;
      const etaDate = new Date(location.eta * 1000);
      const navHtml = `
        <div class="nav-info">
          <div class="nav-info-label">NAVIGATION ACTIVE</div>
          ${location.destinationName ? `<div class="nav-info-value">To: ${location.destinationName}</div>` : ''}
          <div class="nav-info-value">ETA: ${etaDate.toLocaleTimeString()}</div>
          ${location.distanceRemaining ? `<div class="nav-info-value">Distance: ${(location.distanceRemaining / 1000).toFixed(1)} km</div>` : ''}
        </div>
      `;
      document.getElementById('navInfo').innerHTML = navHtml;
    } else if (this.lastNavigationMode && location.mode !== 'navigation') {
      // Navigation just ended - clear the display
      this.lastNavigationMode = false;
      document.getElementById('navInfo').innerHTML = '';
    } else if (!this.lastNavigationMode) {
      // Never was in navigation or already cleared
      document.getElementById('navInfo').innerHTML = '';
    }

    // Battery warning
    if (location.batteryLevel !== undefined && location.batteryLevel < 20) {
      document.getElementById('batteryWarning').innerHTML = `
        <div class="battery-warning">
          ⚠️ Low battery: ${location.batteryLevel}%
        </div>
      `;
    } else {
      document.getElementById('batteryWarning').innerHTML = '';
    }

    // Update time
    const updateDate = new Date(serverTimestamp);
    document.getElementById('updateTime').innerHTML = `
      Last update: ${updateDate.toLocaleTimeString()}
    `;
  }

  /**
   * Start polling for location updates
   */
  startPolling() {
    // Poll every 5 seconds
    this.updateInterval = setInterval(async () => {
      try {
        await this.loadLocation();
      } catch (err) {
        console.error('Polling error:', err);
        // Continue polling even if there's an error
      }
    }, 5000);
  }

  /**
   * Stop polling
   */
  stopPolling() {
    if (this.updateInterval) {
      clearInterval(this.updateInterval);
      this.updateInterval = null;
    }
  }

  /**
   * Show error message
   */
  showError(message) {
    document.getElementById('loadingSpinner').style.display = 'none';
    document.getElementById('errorMessage').style.display = 'block';
    document.getElementById('errorText').textContent = message;
  }
}

// Initialize when DOM is ready
document.addEventListener('DOMContentLoaded', () => {
  const viewer = new LocationViewer();
  viewer.init();

  // Cleanup on page unload
  window.addEventListener('beforeunload', () => {
    viewer.stopPolling();
  });
});
