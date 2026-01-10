# Maps
## The maps_generator

Please refer to the maps_generator tool [instructions](../tools/python/maps_generator/README.md). 

## Publishing `countries.txt`

CoMaps supports updating map metadata independently of app releases 
by downloading a fresh `countries.txt` from the active map server. 
For official CDNs, we require an Ed25519 signature (`countries.txt.sig`) 
to verify integrity/authenticity.

### Files and paths

The app expects these two files at:

- `maps/latest/countries.txt`
- `maps/latest/countries.txt.sig` (raw 64-byte Ed25519 signature)

Map `.mwm` files are still downloaded from versioned directories, e.g.:

- `maps/260106/<region>.mwm`

So `/latest` only needs to contain the metadata files (and signature). 
It does **not** need to contain MWMs.

> Note: If your CDN doesn't support symlinks, you can just upload and overwrite 
> the two files at `maps/latest/` on every publish.

---

### 1) Generate a keypair

Generate an Ed25519 keypair:

```bash
openssl genpkey -algorithm Ed25519 -out countries_ed25519_sk.pem
openssl pkey -in countries_ed25519_sk.pem -pubout -out countries_ed25519_pk.pem
```

Store the private key securely. Only the 
public key is embedded in the app for official builds.

---

### 2) Extract the raw 32-byte public key (for embedding)

The app embeds the raw 32-byte Ed25519 public key. Extract it from 
the PEM public key as hex:

```bash
openssl pkey -pubin -in countries_ed25519_pk.pem -outform DER \
  | tail -c 32 | xxd -p -c 32
```

Take the resulting 32-byte hex string and convert it into the `0x.. `
initializer used in [`kCountriesTxtPublicKey`](../libs/storage/countries_txt_signature.hpp).

### 3) Sign countries.txt (every release)

Create a raw 64-byte signature file:

```bash
openssl pkeyutl -sign -inkey countries_ed25519_sk.pem -rawin \
  -in countries.txt -out countries.txt.sig
```

### 4) Verify locally

```bash
openssl pkeyutl -verify -pubin -inkey countries_ed25519_pk.pem -rawin \
  -in countries.txt -sigfile countries.txt.sig
```

### 5) Publish to the CDN

For a new map release (e.g., `260106`):

1. Upload MWMs to:
    * maps/260106/...
2. Publish metadata + signature to:
   * maps/latest/countries.txt
   * maps/latest/countries.txt.sig

If you have symlink support, you may point `maps/latest/countries.txt` 
to `maps/260106/countries.txt` and same for `.sig`, but it's optional; 
overwriting the two `maps/latest/*` files works everywhere.

### Notes about custom servers

* Custom servers may optionally provide `maps/latest/countries.txt.sig`.
* The signature files for custom servers are currently ignored.
* A future enhancement may allow configuring a trusted public key for 
custom servers so signatures can be enforced there too.
