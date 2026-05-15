# ternaryPQC

**Post-Quantum Cryptographic Keychain Generator**  
*Falcon-512 + SPHINCS⁺-SHA2-128s with custom Ternary SPX-QEC Seed Expansion*

![C](https://img.shields.io/badge/Language-C-00599C.svg) 
![liboqs](https://img.shields.io/badge/liboqs-0.12.x-blue.svg) 
![License](https://img.shields.io/badge/License-MIT-green.svg) 
[![Kubuntu](https://img.shields.io/badge/Platform-Kubuntu%2024.04-orange.svg)](https://kubuntu.org)

---

## 📖 Overview

`ternaryPQC` is a complete, self-contained C implementation that generates secure **post-quantum cryptographic (PQC) master keychains**. It combines two NIST-selected signature algorithms from the [Open Quantum Safe (OQS)](https://openquantumsafe.org) library:

- **Falcon-512** (lattice-based signatures)
- **SPHINCS⁺-SHA2-128s** (hash-based signatures)

A custom **Ternary PQC seed expansion** engine (6000 trits / base-3 digits) applies SPX-QEC pattern cutting, differential shifting, and multi-pass transformations to produce high-entropy seeds that feed the key generation process.

The result is a single JSON `.kchain` file containing:
- The master ternary seed (raw + processed)
- Master Falcon & SPHINCS⁺ keypairs
- 8 role-based **hybrid** keypairs (Falcon + SPHINCS⁺ per role)

Perfect for wallets, service nodes, or any system that needs future-proof PQC keys in a portable, auditable format.

> **Important Limitation**  
> This tool **still does not recover/get all private keys for all formats**, including the hybrid signing method used by some wallets/services. It generates complete, valid master + role keypairs in the native `.kchain` format but does **not** yet support extraction or conversion of every possible private-key encoding/variant found in the wild.

---

## ✨ Features

- **Custom Ternary Engine**
  - 6000-trit master seed
  - SPX-QEC pattern cutting (removes forbidden bit patterns)
  - Ternary differential shift (`d-shift`)
  - Multi-pass full-pass transformations with XOR-jump and modular arithmetic
- **Full PQC Key Generation**
  - Master Falcon-512 & SPHINCS⁺-SHA2-128s keys
  - 8 role-based hybrid keypairs (Falcon + SPHINCS⁺ per role)
- **Portable JSON Output**
  - `svc-wallet/pqc_master_YYYYMMDD_HHMMSS.kchain`
  - All keys stored as hex for easy import
- **Validation & Testing Tool**
  - Loads `.kchain` files
  - Performs full sign/verify cycles for every keypair
- **One-Click Setup**
  - `setup.sh` handles liboqs 0.12.x build, dependencies, and compilation on Kubuntu 24.04
- **Production-Ready**
  - Optimized with `-O3`
  - Uses `liboqs` and `jansson` for reliability
  - Clean, commented C code

---

## 🛠 Prerequisites

- **Operating System**: Kubuntu 24.04 (X11/Wayland) recommended (tested)
- **Build tools**: `gcc`, `make`, `git`, `cmake`, `pkg-config`
- **Libraries**: `libssl-dev`, `libjansson-dev`
- **Disk space**: ~300 MB during liboqs build

---

## 🚀 Installation & Setup

### 1. Clone the repository

```bash
git clone https://github.com/DigiMancer3D/ternaryPQC.git
cd ternaryPQC
```

### 2. Run the automated setup script

```bash
chmod +x setup.sh
./setup.sh
```

The script will:
- Check/install build tools and dependencies
- Build and install **liboqs 0.12.x** from source (if needed)
- Compile `pqc_keygen` and `validate_kchain`
- Create the `svc-wallet/` directory

### 3. (Optional) Manual build instructions

See [`BUILD_INSTRUCTIONS.md`](BUILD_INSTRUCTIONS.md) for detailed manual compilation steps, environment variables, and troubleshooting.

---

## 📋 Usage

### Generate a new keychain

```bash
./pqc_keygen
```

**Output**:
```
svc-wallet/pqc_master_20250515_092100.kchain
```

### Validate & test the keychain

```bash
./validate_kchain svc-wallet/pqc_master_20250515_092100.kchain
```

The validator will:
1. Load the JSON
2. Display seed & key sizes
3. Run full Falcon-512 sign/verify test
4. Run full SPHINCS⁺-SHA2-128s sign/verify test

**Expected output**: All tests should report **"successful"**.

---

## 📁 Repository Structure

```
ternaryPQC/
├── pqc_keygen.c              # Main keychain generator (ternary engine + OQS)
├── validate_kchain.c         # Keychain validator & sign/verify tester
├── setup.sh                  # Automated dependency & build script
├── BUILD_INSTRUCTIONS.md     # Detailed manual build guide
├── README.md                 # This file
└── svc-wallet/               # Output directory (created automatically)
```

---

## 🔬 How the Ternary Engine Works (High-Level)

1. **Seed Generation** — Creates a raw 6000-trit string (0, 1, 2).
2. **SPX-QEC Pattern Cutting** — Repeatedly removes known "bad" patterns (inspired by SPHINCS⁺ quantum-error-correction style cleaning).
3. **Ternary Differential Shift (`d-shift`)** — Applies a rule-based shift between neighboring trits.
4. **Full-Pass Transformations** — Splits into thirds (A/B/C), computes jumps, XORs, and modular additions across passes.
5. **Hybrid Key Derivation** — Processed seed bytes feed `OQS_SIG_keypair()` for Falcon and SPHINCS⁺.

The resulting keys are cryptographically strong and fully compatible with liboqs.

---

## ⚠️ Limitations & Roadmap

- **Current limitation**: Does **not** yet extract or convert **all** private-key formats found in the wild, including certain hybrid signing methods used by some wallets/services.
- Only generates **new** keys (does not brute-force or recover existing ones).
- Tested only on Kubuntu 24.04 with liboqs 0.12.x.
- No Windows/macOS binaries yet.

**Future enhancements** (planned):
- Support for additional PQC algorithms
- Hybrid format export utilities
- Recovery/extraction tools for more wallet formats
- Docker / cross-platform builds

---

