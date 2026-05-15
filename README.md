# ternaryPQC - PQC Keychain Generator + Validator

**Complete Post-Quantum Cryptographic keychain generator and validator** for the ternaryPQC process-separation wallet system.

Generates Falcon-512 + Dilithium3 master and role keys from a high-entropy 6000-trit ternary seed, then lets you **cryptographically validate** every key before use.

---

## Features

✅ **Post-Quantum Secure Algorithms**
- **Falcon-512**: Lattice-based signatures (128-bit security, ~690 B signatures)
- **Dilithium3**: Lattice-based signatures (192-bit security, ~2701 B signatures)

✅ **Ternary PQC Seed Expansion**
- 6000-trit cryptographically secure seed
- SPX-QEC pattern cutting for enhanced entropy
- Converts to 750 bytes of master material

✅ **Complete Key Material**
- Master keypairs for both algorithms
- 8 role-specific keypairs (16 total keys per algorithm)
- Full public **and** private keys exported in JSON

✅ **Robust Key Validator** (included)
- Performs **real cryptographic sign + verify** operations using liboqs
- Validates master keys + all role keys (0, 1, 5, 6, 7)
- Tolerant to whitespace, newlines, and minor formatting variations in `.kchain` files
- Perfect for “Test Your Build” after key generation

✅ **Production Ready**
- Designed for self-verifying coin (SVC) wallet integration
- Direct liboqs C API (no wrapper limitations)

---

## Quick Start (Recommended)

### 1. Prerequisites (Kubuntu 24.04 / Ubuntu)

```bash
sudo apt install build-essential git cmake pkg-config libssl-dev libjansson-dev
```

### 2. Clone & Setup

```bash
git clone https://github.com/DigiMancer3D/ternaryPQC.git
cd ternaryPQC

# One-click setup (builds liboqs 0.12.x if needed)
chmod +x setup.sh
./setup.sh
```

### 3. Generate Keys

```bash
# Generate a new keychain
./pqc_keygen
```

**Output:** `svc-wallet/pqc_master_YYYYMMDD_HHMMSS.kchain`

### 4. Validate Your Keys ([Strongly Recommended] “Test Your Build”)

After generating keys, **always** validate them to confirm your liboqs setup is correct and the keys are mathematically valid:

```bash
# Build the validator (robust version)
gcc -O2 -o pqc_key_validator pqc_key_validator.c \
    -loqs -I/usr/local/include -L/usr/local/lib

# Run validation on the generated keychain
./pqc_key_validator ./svc-wallet/pqc_master_*.kchain
```

**Example successful output:**

```bash
=== PQC Key Validation Tool (ROBUST version - liboqs 0.12.x) ===

Testing Master Falcon-512 (Falcon-512)...
   PASS ✓
Testing Master Dilithium3 (Dilithium3)...
   PASS ✓

--- Role 0 ---
Testing Falcon-512 (Falcon-512)...
   PASS ✓
Testing Dilithium3 (Dilithium3)...
   PASS ✓

--- Role 1 ---
Testing Falcon-512 (Falcon-512)...
   PASS ✓
Testing Dilithium3 (Dilithium3)...
   PASS ✓

--- Role 5 ---
Testing Falcon-512 (Falcon-512)...
   PASS ✓
Testing Dilithium3 (Dilithium3)...
   PASS ✓

--- Role 6 ---
Testing Falcon-512 (Falcon-512)...
   PASS ✓
Testing Dilithium3 (Dilithium3)...
   PASS ✓

--- Role 7 ---
Testing Falcon-512 (Falcon-512)...
   PASS ✓
Testing Dilithium3 (Dilithium3)...
   PASS ✓

=== FINAL RESULT ===
ALL KEYS ARE CRYPTOGRAPHICALLY VALID ✓
You can safely use these keys in your process-separation program.
```

If validation passes, the keys are ready for production use.

---

## Manual Build (if setup.sh is not used)

```bash
export LD_LIBRARY_PATH=/usr/local/lib:$LD_LIBRARY_PATH

# Build key generator
gcc -I/usr/local/include -L/usr/local/lib \
    -o pqc_keygen pqc_keygen.c \
    -loqs -ljansson -lm -O3

# Build key validator (robust version)
gcc -O2 -o pqc_key_validator pqc_key_validator.c \
    -loqs -I/usr/local/include -L/usr/local/lib

mkdir -p svc-wallet
```

---

## Project Structure

```
ternaryPQC/
├── pqc_keygen.c              # Keychain generator
├── pqc_key_validator.c       # Robust validator (included)
├── setup.sh
├── svc-wallet/               # .kchain files go here
└── README.md
```

---

## Output Format

The generated `.kchain` file is a clean JSON document containing:

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
    "roles": [ ... ]
  },
  "generated_at": "2026-05-15T14:30:22Z",
  "algorithm": "Falcon-512 + Dilithium3",
  "library": "liboqs 0.12.x"
}
```

---

## Algorithm Comparison

| Metric          | Falcon-512       | Dilithium3       |
|-----------------|------------------|------------------|
| Security        | 128-bit          | 192-bit          |
| Signature Size  | 690 B            | 2701 B           |
| Public Key Size | 897 B            | 1472 B           |
| Secret Key Size | 1281 B           | 2560 B           |
| Performance     | Very Fast        | Fast             |

**Why both?** Defense-in-depth for critical coin transactions.

---

## Wallet Integration

See the original C integration example in the repo or use the validator-confirmed keys directly in your process-separation code.

---

## Troubleshooting

- **liboqs not found** → Run `export LD_LIBRARY_PATH=/usr/local/lib:$LD_LIBRARY_PATH` and add it to `~/.bashrc`
- **Validator fails to parse** → The included `pqc_key_validator.c` is the **robust** version and should handle messy formatting
- **Test liboqs installation** → Use the test program in the original Troubleshooting section (still present in repo)

---

## Security Notes

🔐 Private keys are fully exposed in JSON (intended for your controlled environment)  
✅ All entropy is cryptographically secure  
✅ Validator performs live sign/verify: only mathematically valid keys pass  
⚠️ Always store `.kchain` files with strict permissions (`chmod 600`) and consider file-level encryption

---

## Performance

- Key generation: ~2-3 seconds
- Validation: ~1-2 seconds
- Binary size: ~500 KB each

---

## References

- **liboqs**: https://github.com/open-quantum-safe/liboqs
- **Falcon**: https://falcon-sign.info/
- **Dilithium**: https://pq-crystals.org/dilithium/
- **Post-Quantum Cryptography**: https://csrc.nist.gov/projects/post-quantum-cryptography/

---

**Made for the ternaryPQC project**: Quantum-resistant infrastructure for [self-verifying coin (SVC) wallets](https://github.com/DigiMancer3D/quantum_fruit/tree/main/SVC).  

---
