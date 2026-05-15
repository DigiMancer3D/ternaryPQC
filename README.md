# PQC Keychain Generator

**Complete C implementation** for generating post-quantum cryptographic keychains with full private key access.

## Features

✅ **Post-Quantum Secure Algorithms**
- **Falcon-512** — Lattice-based signatures (128-bit security, 690 B signatures)
- **Dilithium3** — Lattice-based signatures (192-bit security, 2701 B signatures)

✅ **Ternary PQC Seed Expansion**
- 6000-trit cryptographically secure seed
- SPX-QEC pattern cutting for enhanced entropy
- Converts to 750 bytes of master material

✅ **Complete Key Material**
- Master keypair for both algorithms
- 8 role-specific keypairs (16 total keys per algorithm)
- All public AND private keys in JSON export

✅ **Production Ready**
- Designed for self-verifying coin (SVC) wallet integration
- JSON output with hex-encoded keys
- Sign/verify capabilities via liboqs C API

---

## Quick Start

### Prerequisites (Kubuntu 24.04)

```bash
sudo apt install build-essential git cmake pkg-config libssl-dev libjansson-dev
```

### Build & Generate

```bash
git clone https://github.com/DigiMancer3D/ternaryPQC.git
cd ternaryPQC

# One-click setup (builds liboqs 0.12.x if needed)
chmod +x setup.sh
./setup.sh

# Generate keychain
./pqc_keygen
```

**Output:** `svc-wallet/pqc_master_YYYYMMDD_HHMMSS.kchain`

---

## Manual Build

```bash
# Set library path
export LD_LIBRARY_PATH=/usr/local/lib:$LD_LIBRARY_PATH

# Compile
gcc -I/usr/local/include -L/usr/local/lib \
    -o pqc_keygen pqc_keygen.c \
    -loqs -ljansson -lm -O3

# Create output directory
mkdir -p svc-wallet

# Run
./pqc_keygen
```

---

## Output Format

```json
{
  "seed": {
    "ternary_6000_trits": "012010...",
    "master_bytes_hex": "a1b2c3..."
  },
  "keys": {
    "falcon_512_master_pk": "hex...",
    "falcon_512_master_sk": "hex...",
    "dilithium3_master_pk": "hex...",
    "dilithium3_master_sk": "hex...",
    "roles": [
      {
        "role": 0,
        "falcon_512_pk": "hex...",
        "falcon_512_sk": "hex...",
        "dilithium3_pk": "hex...",
        "dilithium3_sk": "hex..."
      },
      ...
    ]
  },
  "generated_at": "2026-05-15T14:30:22Z",
  "algorithm": "Falcon-512 + Dilithium3",
  "library": "liboqs 0.12.x"
}
```

---

## Algorithm Comparison

| Metric | Falcon-512 | Dilithium3 |
|--------|-----------|-----------|
| Security | 128-bit | 192-bit |
| Signature Size | 690 B | 2701 B |
| Public Key Size | 897 B | 1472 B |
| Secret Key Size | 1281 B | 2560 B |
| Performance | Very Fast | Fast |
| Foundation | Lattice | Lattice |

**Why both?** Redundancy and defense-in-depth for critical coin transactions.

---

## Wallet Integration

### Load & Sign

```c
#include <oqs/oqs.h>
#include <jansson.h>

// Parse .kchain file
json_t *root = json_load_file("pqc_master_*.kchain", 0, NULL);

// Extract Falcon private key
const char *falcon_sk_hex = json_string_value(
    json_object_get(
        json_object_get(root, "keys"),
        "falcon_512_master_sk"
    )
);

// Convert hex to bytes
unsigned char *sk = hex_to_bytes(falcon_sk_hex);

// Sign transaction
OQS_SIG *sig = OQS_SIG_new("Falcon-512");
unsigned char signature[690];
size_t sig_len;

OQS_SIG_sign(sig, signature, &sig_len, 
             transaction_bytes, txn_len, sk);

// Verify
OQS_SIG_verify(sig, transaction_bytes, txn_len,
               signature, sig_len, public_key);

OQS_SIG_free(sig);
```

---

## Troubleshooting

### liboqs not found

```bash
export LD_LIBRARY_PATH=/usr/local/lib:$LD_LIBRARY_PATH
echo 'export LD_LIBRARY_PATH=/usr/local/lib:$LD_LIBRARY_PATH' >> ~/.bashrc
```

### Compilation errors

```bash
# Verbose compile to diagnose
gcc -I/usr/local/include -L/usr/local/lib \
    -o pqc_keygen pqc_keygen.c \
    -loqs -ljansson -lm -O3 -v
```

### Check liboqs installation

```bash
pkg-config --modversion liboqs
# Should output: 0.12.0

# Test algorithms
cat > test_algs.c << 'EOF'
#include <oqs/oqs.h>
#include <stdio.h>

int main() {
    const char *algs[] = {"Falcon-512", "Dilithium3"};
    for (int i = 0; i < 2; i++) {
        OQS_SIG *sig = OQS_SIG_new(algs[i]);
        printf("%s: %s\n", algs[i], sig ? "✓" : "✗");
        if (sig) OQS_SIG_free(sig);
    }
    return 0;
}
EOF

gcc -I/usr/local/include -L/usr/local/lib test_algs.c -loqs -o test_algs
export LD_LIBRARY_PATH=/usr/local/lib:$LD_LIBRARY_PATH
./test_algs
```

---

## Security Notes

🔐 **Private keys are fully exposed** in JSON (unlike Python wrapper)

✅ **All entropy cryptographically secure**
- System randomness via `OQS_randombytes_switch_algorithm("system")`
- 6000-trit ternary expansion with pattern cutting

✅ **No backdoors or weak parameters**
- Direct liboqs C API (no wrapper limitations)
- Standard PQC algorithms (NIST-standardized)

⚠️ **Secure your keychains**
- Store in encrypted wallet directory
- Consider file-level encryption: `encfs` or `cryptsetup`
- Restrict file permissions: `chmod 600 *.kchain`

---

## Performance

- **Generation time:** ~2-3 seconds
- **Binary size:** ~500 KB
- **Keychain file size:** ~50-100 KB
- **Memory usage:** ~50 MB peak

---

## References

- **liboqs:** https://github.com/open-quantum-safe/liboqs
- **Falcon:** https://falcon-sign.info/
- **Dilithium:** https://pq-crystals.org/dilithium/
- **Post-Quantum Cryptography:** https://csrc.nist.gov/projects/post-quantum-cryptography/

---

## License

MIT — See LICENSE file

---

## Author

DigiMancer3D — SVC Wallet Cryptography  
Generated: May 15, 2026
