# ternaryPQC: Post-Quantum Cryptographic Wallet System 

![ternaryPQC Icon](icon.png)

**ternaryPQC** is a custom toolkit for building quantum-resistant cryptocurrency wallets using **ternary (base-3) seed expansion** combined with three leading Post-Quantum Cryptography (PQC) algorithms.

It includes:
- Master + role-based keychain generation
- Robust validation tools
- Hybrid multi-algorithm signing
- Ring-based password manager
- An educational interactive game (#HASHBREAKER)

Designed for **process-separation wallets** and **Self-Verifying Coins (SVC)**, this project lets you generate, test, and use real quantum-safe keys today: with full source code, one-click setup, and a fun game to explore the system.

---

## ✨ Key Features

- **Ternary Seed Expansion**: Starts with high-entropy input by expanding to a 6000-trit master seed using SPX-QEC (pattern cutting + SHAKE-256)
- **Three PQC Algorithms**: Falcon-512, ML-DSA-65 (Dilithium), SLH-DSA-SHA2-128s (SPHINCS+)
- **Full Keychain Tools**: Generator, validator, hybrid signer, and robust tester
- **Ring Password System**: SHA3-512 hashed passwords with Ring0 proof files (.ssp) for ringCT compatibility
- **#HASHBREAKER Game**: Interactive Python game that uses your generated keys as challenge data (progressive levels, leaderboard, debug commands)
- **Testing-Ready Build**: Works with liboqs, one-command setup, JSON export, full sign/verify testing

---

## How It Works

1. You provide entropy → the system expands it into a massive **6000-trit ternary master seed**.
2. From that single seed, it deterministically derives **master + 9 role keys** for each of the three PQC algorithms.
3. You can instantly validate every key by performing real cryptographic sign/verify operations.
4. Hybrid signing lets you create signatures that combine all three algorithms for maximum defense-in-depth.
5. The included game lets you “play” with your keys: turning cryptography into an educational and entertaining experience.

This creates a **process-separated wallet** where different roles (spending, staking, viewing, etc.) have isolated keys, all derived from one secure ternary root.

---

## Quick Start

```bash
git clone https://github.com/DigiMancer3D/ternaryPQC.git
cd ternaryPQC
chmod +x setup.sh
./setup.sh          # Builds liboqs + all tools
```

**Most useful commands:**
- `./pqc_keygen` → Generate a full keychain
- `./validate_kchain` → Test all keys with real signatures
- `python3 game.py` → Play #HASHBREAKER
- `./pqc_hybrid_signer` → Create multi-algorithm signatures

See `BUILD_INSTRUCTIONS.md` for advanced setup.

---

## Terminology Table

| Term                  | What it really is                              | Plain English                                      |
|-----------------------|------------------------------------------------|----------------------------------------------------|
| **ternaryPQC**        | The whole project                              | Ternary-based Post-Quantum Crypto toolkit          |
| **6000-trit seed**    | Massive expanded master entropy                | The super-secure root everything comes from        |
| **SPX-QEC**           | Custom seed expansion pipeline                 | The “magic” that turns small input into huge safe seed |
| **Keychain**          | JSON file with all master + role keys          | One file containing your entire secure wallet      |
| **Hybrid Signer**     | Signs with Falcon + Dilithium + SPHINCS+       | Defense-in-depth signatures                        |
| **Ring Password**     | SHA3-512 + Ring0 proof (.ssp)                  | Secure password system compatible with ringCT      |
| **#HASHBREAKER**      | Interactive Python game                        | Fun way to test and learn your keys                |

---

## Project Status

**🚀 Active & Functional • Cryptographically-Ready Components**

- All core C tools compile and work on Ubuntu 22.04/24.04
- Game is fully playable and educational
- Designed for future wallet integration
- Still Finalizing for the SVC (Self-Verifying Coins) Project

**Security Note**: This is an experimental research implementation. Private keys are exported in plain JSON for controlled environments. Always use proper key management (GPG, hardware, air-gapped), & do not use real funds.

Contributions, testing on other distros, GUI development, or help integrating into actual wallets are extremely welcome!

---

