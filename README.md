# ternaryPQC - Post-Quantum Cryptographic Wallet System

**Complete Post-Quantum Cryptographic keychain generator, validator, password manager, and game** for the ternaryPQC process-separation wallet system.

Generates **Falcon-512 + ML-DSA-65 + SLH-DSA-SHA2-128s (SPHINCS+)** master and role keys from a high-entropy 6000-trit ternary seed, then lets you **cryptographically validate** every key before use. Includes Ring Password generator and the #HASHBREAKER interactive game.

---

## Features

✅ **Post-Quantum Secure Algorithms**
- **Falcon-512**: Lattice-based signatures (128-bit security, ~690 B signatures)
- **ML-DSA-65** (Dilithium3): Lattice-based signatures (192-bit security, ~2701 B signatures)
- **SLH-DSA-SHA2-128s** (SPHINCS+): Hash-based signatures (128-bit security, ~7856 B signatures)

✅ **Ternary PQC Seed Expansion (SPX-QEC Pipeline)**
- 6000-trit cryptographically secure seed
- SPX-QEC pattern cutting for enhanced entropy distillation
- SHAKE-256 master pool expansion
- Converts to 750+ bytes of master material

✅ **Complete Key Material**
- Master keypairs for all 3 algorithms
- 9 role-specific keypairs (27 total keys per algorithm)
- Full public **and** private keys exported in JSON
- Each role can have independent signing capability

✅ **Multiple Validators & Testers**
- **validate_kchain**: Full cryptographic sign + verify operations
- **test_algs**: Test algorithm availability in your liboqs build
- **list_pqc_sigs**: Display all enabled signature schemes
- Tests master keys + all role keys (0-8)
- Tolerant to whitespace, newlines, and formatting variations
- Perfect for "Test Your Build" after key generation

✅ **Ring Password Generator**
- Hashes passwords using SHA3-512
- Generates Ring0 proof for ringCT wrapper
- Creates `.ssp` files with encoded Ring0 data
- Full ringCT-compatible output format

✅ **#HASHBREAKER Interactive Game**
- Uses SPX-QEC-distilled key material as game challenge
- Progressive difficulty levels (1-4, up to 1337)
- Leaderboard system with session tracking
- Score multipliers and bonus system
- Auto-save with manual load capability
- Debug commands for testing and development

✅ **Production Ready**
- Designed for self-verifying coin (SVC) wallet integration
- Direct liboqs C API (no wrapper limitations)
- Tested on Kubuntu 24.04 / Ubuntu 22.04+

---

## Quick Start (Recommended)

### 1. Prerequisites (Kubuntu 24.04 / Ubuntu)

```bash
sudo apt install build-essential git cmake pkg-config libssl-dev libjansson-dev python3-tk
```

### 2. Clone & Setup

```bash
git clone https://github.com/DigiMancer3D/ternaryPQC.git
cd ternaryPQC

# One-click setup (builds liboqs 0.12.x if needed)
chmod +x setup.sh
./setup.sh
```

---

## Complete Usage Workflow

### Step 1: Test Your Environment (Environment Verification)

Before generating keys, verify that all algorithms are available in your liboqs build:

```bash
# List all supported algorithms in your liboqs build
gcc -o list_pqc_sigs list_pqc_sigs.c -loqs -O2
./list_pqc_sigs
```

**Expected output:**
```
=== Supported PQC Signature Schemes in your liboqs ===

  ✓ SPHINCS+-SHA2-128s  (enabled)
  ✓ Falcon-512  (enabled)
  ✓ ML-DSA-65  (enabled)
  ... (more algorithms)

=== End of list ===
```

### Step 2: Test Algorithms (Verify Your Setup)

Verify your specific algorithms are working:

```bash
gcc -o test_algs test_algs.c -loqs -O2
./test_algs
```

**Expected output:**
```
Testing SPHINCS+ variants:
  SPHINCS+-SHA2-128s: ✓ AVAILABLE
  SPHINCS+-SHAKE-128s: ✓ AVAILABLE
  SPHINCS+-SHA2-128s-robust: ✓ AVAILABLE
  SPHINCS+-SHAKE-128s-robust: ✓ AVAILABLE
```

### Step 3: Generate Keys (Master Keychain Generation)

Generate a new keychain with the SPX-QEC ternary distillation pipeline:

```bash
./pqc_keygen
```

**Output:** `svc-wallet/pqc_master_YYYYMMDD_HHMMSS.kchain`

**Console output:**
```
========== PQC Keychain Generator (SPX-QEC driven) ==========

[1/7] Generating high-entropy 512-trit seed...
 Generated 512 ternary digits

[2/7] Expanding to 10k+ trits with SPX-QEC...
 Expansion complete: 10247 trits

[3/7] Finalizing to 6000-trit seed...
 Seed ready: 6000 trits

[4/7] Feeding distilled trits into master entropy pool...
 Master pool ready (SHAKE-256 expanded)

[5/7] Generating Master keypairs (Falcon-512 + ML-DSA-65 + SLH-DSA-SHA2-128s)...
 Master keys generated

[6/7] Generating 9 Role keypairs...
 Role 0 generated
 Role 1 generated
 ...
 Role 8 generated

Building JSON keychain...
✅ Keychain saved to: svc-wallet/pqc_master_20260520_143022.kchain

✅ Complete! All 3 algorithms (Falcon + Dilithium/ML-DSA + SPHINCS+/SLH-DSA) are now working.
```

### Step 4: Validate Your Keys (Cryptographic Verification)

After generating keys, **always** validate them to confirm your liboqs setup is correct:

```bash
# Build the validator
gcc -O2 -o validate_kchain validate_kchain.c \
    -loqs -ljansson -I/usr/local/include -L/usr/local/lib

# Run validation on the generated keychain
./validate_kchain ./svc-wallet/pqc_master_*.kchain
```

**Example successful output:**

```
===== Keychain Validator & Sign/Verify Tester =====

[1/4] Loading keychain file: ./svc-wallet/pqc_master_20260520_143022.kchain
✓ JSON loaded

[2/4] Parsing seed data...
✓ Seed: 6000 ternary digits

[3/4] Testing master Falcon-512 keys...
  Falcon public key:  897 bytes
  Falcon private key: 1281 bytes
  Signature length: 690 bytes
  ✓ Falcon sign/verify successful

[4/4] Testing master SPHINCS+-SHA2-128s keys...
  SPHINCS+ public key:  32 bytes
  SPHINCS+ private key: 128 bytes
  Signature length: 7856 bytes
  ✓ SPHINCS+ sign/verify successful

✅ All validations passed!
   Keychain is valid and ready for use.
```

If validation passes, the keys are ready for production use.

### Step 5: Setup Ring Password (Ring-based Authentication)

Generate a Ring-authenticated password proof:

```bash
# Build Ring Password generator
gcc -o ring_password ring_password.c -loqs -lssl -lcrypto -O2

# Generate password proof
./ring_password "your super secret password here"
```

**Output:**
```
✅ Created: abcxyz.ssp
Ring0 (password hash): a41217303c74a5a6fd401da567f9234f9d5900d191139eb927ace61f5b47b863...

Full ringCT wrapper ready (Ring0 → Ring3 + ringCT proof).
```

The generated `.ssp` file contains:
- Filename header (Ring0 identifier)
- Full SHA3-512 hash of the password
- "Super Secret Password" footer (for manual verification)

### Step 6: Play #HASHBREAKER (Interactive Game)

Launch the interactive cryptographic game:

```bash
# Make game executable
chmod +x game.py

# Run the game
./game.py
```

**Game Features:**
- **Modes**: Use text input, load files, or use generated keychain hashes
- **Levels**: Progress from level 1 to 4 based on score
- **Difficulty**: Auto-adjusts based on performance (New Game → Easy → Normal → Hard → 1337)
- **Scoring**: Points = matches × level, with bonuses for level completion
- **Leaderboard**: Top 10 scores tracked with session IDs
- **Debug Commands**: `isuck`, `simwin`, `simhit`, `comanlist`, etc.

**Debug Commands** (embed in text input):
```
isuck              - Show win state
simwin             - Simulate level win
simhit             - Simulate good match
simmiss            - Simulate miss
comanlist          - Show all commands
diffplus           - Increase difficulty
diffminus          - Decrease difficulty
newsessvalueplease - Generate new session
```

---

## Build Instructions (Manual Build)

If you prefer to build everything manually:

```bash
# Set library path
export LD_LIBRARY_PATH=/usr/local/lib:$LD_LIBRARY_PATH

# Build all utilities
gcc -I/usr/local/include -L/usr/local/lib \
    -o pqc_keygen pqc_keygen.c \
    -loqs -ljansson -lm -O3

gcc -I/usr/local/include -L/usr/local/lib \
    -o validate_kchain validate_kchain.c \
    -loqs -ljansson -lm -O3

gcc -I/usr/local/include -L/usr/local/lib \
    -o test_algs test_algs.c \
    -loqs -O2

gcc -I/usr/local/include -L/usr/local/lib \
    -o list_pqc_sigs list_pqc_sigs.c \
    -loqs -O2

gcc -I/usr/local/include -L/usr/local/lib \
    -o ring_password ring_password.c \
    -lssl -lcrypto -O2

# Create wallet directory
mkdir -p svc-wallet
```

**Add to ~/.bashrc for permanent library path:**
```bash
export LD_LIBRARY_PATH=/usr/local/lib:$LD_LIBRARY_PATH
```

---

## Project Structure

```
ternaryPQC/
├── pqc_keygen.c              # Main keychain generator (SPX-QEC driven)
├── validate_kchain.c         # Cryptographic validator (sign/verify test)
├── test_algs.c              # Algorithm availability tester
├── list_pqc_sigs.c          # List all supported algorithms
├── ring_password.c          # Ring-authenticated password generator
├── game.py                  # #HASHBREAKER interactive game
├── setup.sh                 # One-click setup script
├── svc-wallet/              # .kchain files (generated)
├── BUILD_INSTRUCTIONS.md    # Additional build details
├── README.md                # This file
├── icon.png                 # Game window icon
└── image.png                # Game logo
```

---

## Programs Reference

| Program | Purpose | Input | Output |
|---------|---------|-------|--------|
| **pqc_keygen** | Generate keychain | None (generates entropy) | `pqc_master_*.kchain` |
| **validate_kchain** | Verify keychain validity | `*.kchain` file | Sign/verify test results |
| **test_algs** | Check algorithm support | None | Algorithm availability |
| **list_pqc_sigs** | List all algorithms | None | Full algorithm list |
| **ring_password** | Generate Ring0 proof | Password string | `*.ssp` file + Ring0 hash |
| **game.py** | Play #HASHBREAKER | Text/File/Hash | Leaderboard scores |

---

## Output Format (.kchain JSON)

The generated `.kchain` file is a complete JSON document:

```json
{
  "seed": {
    "ternary_6000_trits": "012010...",
    "master_pool_hex": "a1b2c3..."
  },
  "keys": {
    "falcon_512_master_pk": "hex...",
    "falcon_512_master_sk": "hex...",
    "dilithium3_master_pk": "hex...",
    "dilithium3_master_sk": "hex...",
    "sphincs128s_master_pk": "hex...",
    "sphincs128s_master_sk": "hex...",
    "roles": [
      {
        "role": 0,
        "falcon_512_pk": "hex...",
        "falcon_512_sk": "hex...",
        "dilithium3_pk": "hex...",
        "dilithium3_sk": "hex...",
        "sphincs128s_pk": "hex...",
        "sphincs128s_sk": "hex..."
      },
      ...
    ]
  },
  "generated_at": "2026-05-20T14:30:22Z",
  "algorithm": "Falcon-512 + ML-DSA-65 + SLH-DSA-SHA2-128s (SPHINCS+)",
  "library": "liboqs 0.12.x",
  "note": "All keys are driven by the SPX-QEC ternary distillation pipeline."
}
```

---

## Algorithm Comparison

| Metric | Falcon-512 | ML-DSA-65 | SLH-DSA-128s |
|--------|-----------|----------|------------|
| Security | 128-bit | 192-bit | 128-bit |
| Signature Size | 690 B | 2701 B | 7856 B |
| Public Key Size | 897 B | 1472 B | 32 B |
| Secret Key Size | 1281 B | 2560 B | 128 B |
| Performance | Very Fast | Fast | Moderate |
| Type | Lattice | Lattice | Hash-based |

**Why three algorithms?** Defense-in-depth for critical transactions. Falcon for speed, ML-DSA for security margin, SPHINCS+ for quantum-resistant hash guarantees.

---

## Supported Algorithms (Full List from liboqs 0.12.x)

Run `./list_pqc_sigs` to see your specific build. Common signature schemes:

**Lattice-based:**
- Falcon-512, Falcon-1024
- ML-DSA-44, ML-DSA-65, ML-DSA-87
- CRYSTALS-Dilithium2, Dilithium3, Dilithium5

**Hash-based:**
- SPHINCS+-SHA2-128s, SPHINCS+-SHA2-128f
- SPHINCS+-SHAKE-128s, SPHINCS+-SHAKE-128f
- SLH-DSA-SHA2-128s, SLH-DSA-SHA2-128f

**Other:**
- XMSS variants, FAWKES, PICNIC variants

---

## Wallet Integration

See the original C integration example or use the validator-confirmed keys directly:

```c
// Load keychain
json_t *root = json_load_file("path/to/file.kchain", 0, NULL);

// Extract master Falcon public key
const char *pk_hex = json_string_value(
    json_object_get(json_object_get(root, "keys"), "falcon_512_master_pk")
);

// Use with liboqs for signing/verification
```

---

## Troubleshooting

| Issue | Solution |
|-------|----------|
| **liboqs not found** | Run `export LD_LIBRARY_PATH=/usr/local/lib:$LD_LIBRARY_PATH` and add to `~/.bashrc` |
| **Validator fails to parse** | Use `validate_kchain.c` (robust version handles formatting variations) |
| **Test algorithm fails** | Run `./list_pqc_sigs` to check what's available in your build |
| **gcc: command not found** | Install with `sudo apt install build-essential` |
| **jansson library missing** | Install with `sudo apt install libjansson-dev` |
| **Python game won't start** | Ensure `python3-tk` is installed: `sudo apt install python3-tk` |
| **Icon/Image files missing** | Game will still run; icons are optional enhancements |

**Test liboqs installation:**
```bash
gcc -o test_oqs -xc - -loqs << 'EOF'
#include <oqs/oqs.h>
#include <stdio.h>
int main() {
    OQS_randombytes_switch_algorithm("system");
    printf("OQS initialized!\n");
    return 0;
}
EOF
./test_oqs
```

---

## Security Notes

🔐 **Private keys are fully exposed in JSON** (intended for your controlled environment)  
✅ **All entropy is cryptographically secure** (via OpenSSL EVP + liboqs)  
✅ **Validator performs live sign/verify**: only mathematically valid keys pass  
✅ **Ring password**: SHA3-512 hashing, suitable for ringCT integration  
✅ **Game security**: Uses actual keychain data as challenge material  
⚠️ **Always store `.kchain` files with strict permissions:**
```bash
chmod 600 svc-wallet/*.kchain
```

**Consider file-level encryption for production:**
```bash
gpg --symmetric --cipher-algo AES256 svc-wallet/pqc_master_*.kchain
```

---

## Performance

- **Key generation**: ~2-3 seconds (includes SPX-QEC distillation)
- **Validation**: ~1-2 seconds (all algorithms tested)
- **Binary size**: pqc_keygen ~500 KB, validate_kchain ~400 KB
- **Game startup**: <1 second
- **Ring password**: <100ms

---

## References

- **liboqs**: https://github.com/open-quantum-safe/liboqs
- **Falcon**: https://falcon-sign.info/
- **ML-DSA / Dilithium**: https://pq-crystals.org/dilithium/
- **SPHINCS+ / SLH-DSA**: https://sphincs.org/
- **Post-Quantum Cryptography**: https://csrc.nist.gov/projects/post-quantum-cryptography/
- **RingCT**: https://lab.getmonero.org/

---

## License

This project is part of the ternaryPQC initiative for quantum-resistant infrastructure.

---

**Made for the ternaryPQC project**: Quantum-resistant infrastructure for [self-verifying coin (SVC) wallets](https://github.com/DigiMancer3D/quantum_fruit/tree/main/SVC).

**Components:**
- `pqc_keygen`: SPX-QEC ternary seed distillation + multi-algorithm key generation
- `validate_kchain`: Cryptographic verification suite
- `ring_password`: Ring0 proof generator for ringCT integration
- `game.py`: #HASHBREAKER interactive learning/testing tool
- `test_algs` / `list_pqc_sigs`: Environment diagnostics

**Repository**: https://github.com/DigiMancer3D/ternaryPQC  
**Last Updated**: 2026-05-20
