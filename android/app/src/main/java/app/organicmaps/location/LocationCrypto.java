package app.organicmaps.location;

import android.util.Base64;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

import org.json.JSONException;
import org.json.JSONObject;

import java.nio.charset.StandardCharsets;
import java.security.SecureRandom;

import javax.crypto.Cipher;
import javax.crypto.spec.GCMParameterSpec;
import javax.crypto.spec.SecretKeySpec;

/**
 * AES-256-GCM encryption/decryption for location data.
 */
public class LocationCrypto
{
  private static final String ALGORITHM = "AES/GCM/NoPadding";
  private static final int GCM_IV_LENGTH = 12; // 96 bits
  private static final int GCM_TAG_LENGTH = 128; // 128 bits

  /**
   * Encrypt plaintext JSON using AES-256-GCM.
   * @param base64Key Base64-encoded 256-bit key
   * @param plaintextJson JSON string to encrypt
   * @return JSON string with encrypted payload: {"iv":"...","ciphertext":"...","authTag":"..."}
   */
  @Nullable
  public static String encrypt(@NonNull String base64Key, @NonNull String plaintextJson)
  {
    try
    {
      // Decode the base64 key
      byte[] key = Base64.decode(base64Key, Base64.NO_WRAP);
      if (key.length != 32) // 256 bits
      {
        android.util.Log.e("LocationCrypto", "Invalid key size: " + key.length);
        return null;
      }

      // Generate random IV
      byte[] iv = new byte[GCM_IV_LENGTH];
      new SecureRandom().nextBytes(iv);

      // Create cipher
      Cipher cipher = Cipher.getInstance(ALGORITHM);
      SecretKeySpec keySpec = new SecretKeySpec(key, "AES");
      GCMParameterSpec gcmSpec = new GCMParameterSpec(GCM_TAG_LENGTH, iv);
      cipher.init(Cipher.ENCRYPT_MODE, keySpec, gcmSpec);

      // Encrypt
      byte[] plaintext = plaintextJson.getBytes(StandardCharsets.UTF_8);
      byte[] ciphertextWithTag = cipher.doFinal(plaintext);

      // Split ciphertext and auth tag
      // In GCM mode, doFinal() returns ciphertext + tag
      int ciphertextLength = ciphertextWithTag.length - (GCM_TAG_LENGTH / 8);
      byte[] ciphertext = new byte[ciphertextLength];
      byte[] authTag = new byte[GCM_TAG_LENGTH / 8];

      System.arraycopy(ciphertextWithTag, 0, ciphertext, 0, ciphertextLength);
      System.arraycopy(ciphertextWithTag, ciphertextLength, authTag, 0, authTag.length);

      // Build JSON response
      JSONObject result = new JSONObject();
      result.put("iv", Base64.encodeToString(iv, Base64.NO_WRAP));
      result.put("ciphertext", Base64.encodeToString(ciphertext, Base64.NO_WRAP));
      result.put("authTag", Base64.encodeToString(authTag, Base64.NO_WRAP));

      return result.toString();
    }
    catch (Exception e)
    {
      android.util.Log.e("LocationCrypto", "Encryption failed", e);
      return null;
    }
  }

  /**
   * Decrypt encrypted payload using AES-256-GCM.
   * @param base64Key Base64-encoded 256-bit key
   * @param encryptedPayloadJson JSON string with format: {"iv":"...","ciphertext":"...","authTag":"..."}
   * @return Decrypted plaintext JSON string
   */
  @Nullable
  public static String decrypt(@NonNull String base64Key, @NonNull String encryptedPayloadJson)
  {
    try
    {
      // Parse encrypted payload
      JSONObject payload = new JSONObject(encryptedPayloadJson);
      byte[] iv = Base64.decode(payload.getString("iv"), Base64.NO_WRAP);
      byte[] ciphertext = Base64.decode(payload.getString("ciphertext"), Base64.NO_WRAP);
      byte[] authTag = Base64.decode(payload.getString("authTag"), Base64.NO_WRAP);

      // Decode the base64 key
      byte[] key = Base64.decode(base64Key, Base64.NO_WRAP);
      if (key.length != 32) // 256 bits
      {
        android.util.Log.e("LocationCrypto", "Invalid key size: " + key.length);
        return null;
      }

      // Combine ciphertext and auth tag for GCM decryption
      byte[] ciphertextWithTag = new byte[ciphertext.length + authTag.length];
      System.arraycopy(ciphertext, 0, ciphertextWithTag, 0, ciphertext.length);
      System.arraycopy(authTag, 0, ciphertextWithTag, ciphertext.length, authTag.length);

      // Create cipher
      Cipher cipher = Cipher.getInstance(ALGORITHM);
      SecretKeySpec keySpec = new SecretKeySpec(key, "AES");
      GCMParameterSpec gcmSpec = new GCMParameterSpec(GCM_TAG_LENGTH, iv);
      cipher.init(Cipher.DECRYPT_MODE, keySpec, gcmSpec);

      // Decrypt
      byte[] plaintext = cipher.doFinal(ciphertextWithTag);

      return new String(plaintext, StandardCharsets.UTF_8);
    }
    catch (Exception e)
    {
      android.util.Log.e("LocationCrypto", "Decryption failed", e);
      return null;
    }
  }
}
