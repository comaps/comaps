/**
 * Crypto utilities for decrypting location data
 * Uses Web Crypto API (AES-GCM-256)
 */

class LocationCrypto {
  /**
   * Decode base64url to base64
   */
  static base64UrlToBase64(base64url) {
    let base64 = base64url.replace(/-/g, '+').replace(/_/g, '/');
    while (base64.length % 4) {
      base64 += '=';
    }
    return base64;
  }

  /**
   * Parse the encoded credentials from URL
   * Format: sessionId:encryptionKey (base64url encoded)
   */
  static parseCredentials(encodedCredentials) {
    try {
      const base64 = this.base64UrlToBase64(encodedCredentials);
      const decoded = atob(base64);
      const [sessionId, encryptionKey] = decoded.split(':');

      if (!sessionId || !encryptionKey) {
        throw new Error('Invalid credentials format');
      }

      return { sessionId, encryptionKey };
    } catch (err) {
      console.error('Failed to parse credentials:', err);
      return null;
    }
  }

  /**
   * Convert base64 encryption key to CryptoKey
   */
  static async importKey(base64Key) {
    try {
      const keyData = Uint8Array.from(atob(base64Key), c => c.charCodeAt(0));

      return await window.crypto.subtle.importKey(
        'raw',
        keyData,
        { name: 'AES-GCM', length: 256 },
        false,
        ['decrypt']
      );
    } catch (err) {
      console.error('Failed to import key:', err);
      throw new Error('Invalid encryption key');
    }
  }

  /**
   * Decrypt the encrypted payload
   * Payload format (JSON): { iv: base64, ciphertext: base64, authTag: base64 }
   */
  static async decryptPayload(encryptedPayloadJson, encryptionKey) {
    try {
      // Parse the encrypted payload
      const payload = JSON.parse(encryptedPayloadJson);
      const { iv, ciphertext, authTag } = payload;

      if (!iv || !ciphertext || !authTag) {
        throw new Error('Invalid payload format');
      }

      // Import the key
      const cryptoKey = await this.importKey(encryptionKey);

      // Decode base64 components
      const ivBytes = Uint8Array.from(atob(iv), c => c.charCodeAt(0));
      const ciphertextBytes = Uint8Array.from(atob(ciphertext), c => c.charCodeAt(0));
      const authTagBytes = Uint8Array.from(atob(authTag), c => c.charCodeAt(0));

      // Combine ciphertext and auth tag (GCM mode requires them together)
      const combined = new Uint8Array(ciphertextBytes.length + authTagBytes.length);
      combined.set(ciphertextBytes, 0);
      combined.set(authTagBytes, ciphertextBytes.length);

      // Decrypt
      const decryptedBuffer = await window.crypto.subtle.decrypt(
        {
          name: 'AES-GCM',
          iv: ivBytes,
          tagLength: 128 // 16 bytes = 128 bits
        },
        cryptoKey,
        combined
      );

      // Convert to string and parse JSON
      const decoder = new TextDecoder();
      const decryptedText = decoder.decode(decryptedBuffer);
      return JSON.parse(decryptedText);
    } catch (err) {
      console.error('Decryption failed:', err);
      throw new Error('Failed to decrypt location data');
    }
  }
}

// Export for use in app.js
window.LocationCrypto = LocationCrypto;
