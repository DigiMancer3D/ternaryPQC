/* ==================== SPHINCS++ HYBRID BTC BUILDER ====================
 * compile script:
 * gcc -o pqc_sphincs_plus pqc_sphincs_plus.c -loqs -ljansson -lcrypto -lm
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <oqs/oqs.h>
#include <jansson.h>
#include <openssl/ec.h>
#include <openssl/obj_mac.h>
#include <openssl/bn.h>
#include <openssl/evp.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <unistd.h>

static const char *BASE_PATTERNS[] = {
    "00", "11", "01", "10", "100", "011", "101", "010",
    "1001", "0110", "10100", "01011", "001101"
};
static const int BASE_PATTERNS_COUNT = 13;

typedef struct {
    char **patterns;
    int count;
} PatternList;

static PatternList build_spx_patterns(void) {
    PatternList result = {0};
    char temp[256];
    char temp2[256];
    int capacity = 100;
    result.patterns = (char **)malloc(capacity * sizeof(char *));
    result.count = 0;
    for (int i = 0; i < BASE_PATTERNS_COUNT; i++) {
        const char *base = BASE_PATTERNS[i];
        int base_len = strlen(base);
        if (result.count >= capacity) { capacity *= 2; result.patterns = (char **)realloc(result.patterns, capacity * sizeof(char *)); }
        snprintf(temp, sizeof(temp), "%s%s", base, base + 1);
        result.patterns[result.count++] = strdup(temp);

        if (result.count >= capacity) { capacity *= 2; result.patterns = (char **)realloc(result.patterns, capacity * sizeof(char *)); }
        strcpy(temp, base);
        for (int j = 0; j < base_len / 2; j++) { char c = temp[j]; temp[j] = temp[base_len - 1 - j]; temp[base_len - 1 - j] = c; }
        result.patterns[result.count++] = strdup(temp);

        if (result.count >= capacity) { capacity *= 2; result.patterns = (char **)realloc(result.patterns, capacity * sizeof(char *)); }
        strcpy(temp, base);
        for (int j = 0; j < base_len / 2; j++) { char c = temp[j]; temp[j] = temp[base_len - 1 - j]; temp[base_len - 1 - j] = c; }
        snprintf(temp2, sizeof(temp2), "%s%s", base, temp);
        result.patterns[result.count++] = strdup(temp2);
    }
    return result;
}

static void free_patterns(PatternList *plist) {
    for (int i = 0; i < plist->count; i++) free(plist->patterns[i]);
    free(plist->patterns);
}

static char *spx_qec_cleanup(const char *trits, int max_iterations) {
    char *cleaned = strdup(trits);
    PatternList patterns = build_spx_patterns();
    for (int iter = 0; iter < max_iterations; iter++) {
        char *prev = strdup(cleaned);
        for (int p = 0; p < patterns.count; p++) {
            char *pos = strstr(cleaned, patterns.patterns[p]);
            while (pos) {
                memmove(pos, pos + strlen(patterns.patterns[p]), strlen(pos + strlen(patterns.patterns[p])) + 1);
                pos = strstr(pos, patterns.patterns[p]);
            }
        }
        if (strlen(cleaned) == strlen(prev)) { free(prev); break; }
        free(prev);
    }
    free_patterns(&patterns);
    return cleaned;
}

static char* base58_encode(const unsigned char* data, size_t len) {
    static const char* alphabet = "123456789ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz";
    size_t i, j, carry;
    size_t size = len * 138 / 100 + 1;
    unsigned char* buf = calloc(size, 1);
    for (i = 0; i < len; i++) {
        carry = data[i];
        for (j = 0; j < size; j++) {
            carry += (unsigned char)buf[j] * 256;
            buf[j] = carry % 58;
            carry /= 58;
        }
    }
    for (i = 0; i < size && buf[i] == 0; i++);
    char* out = malloc(size - i + 1);
    for (j = 0; i < size; i++, j++) out[j] = alphabet[buf[i]];
    out[j] = '\0';
    free(buf);
    return out;
}

static char *bytes_to_hex(const unsigned char *data, size_t len) {
    char *hex = malloc(len * 2 + 1);
    for (size_t i = 0; i < len; i++) sprintf(hex + i * 2, "%02x", data[i]);
    hex[len * 2] = '\0';
    return hex;
}

static int generate_btc_keypair_from_seed(const unsigned char *seed32, unsigned char **priv_out, size_t *priv_len_out,
                                          unsigned char **pub_out, size_t *pub_len_out) {
    *priv_len_out = 32;
    *priv_out = malloc(32);
    memcpy(*priv_out, seed32, 32);

    const EC_GROUP *group = EC_GROUP_new_by_curve_name(NID_secp256k1);
    BIGNUM *priv_bn = BN_bin2bn(*priv_out, 32, NULL);
    EC_POINT *pub_point = EC_POINT_new(group);
    if (!group || !priv_bn || !pub_point || !EC_POINT_mul(group, pub_point, priv_bn, NULL, NULL, NULL)) {
        BN_free(priv_bn); EC_POINT_free(pub_point); EC_GROUP_free((EC_GROUP*)group); return 0;
    }

    *pub_len_out = EC_POINT_point2oct(group, pub_point, POINT_CONVERSION_COMPRESSED, NULL, 0, NULL);
    *pub_out = malloc(*pub_len_out);
    EC_POINT_point2oct(group, pub_point, POINT_CONVERSION_COMPRESSED, *pub_out, *pub_len_out, NULL);

    BN_free(priv_bn);
    EC_POINT_free(pub_point);
    EC_GROUP_free((EC_GROUP*)group);
    return 1;
}

/* ==================== BIP-341 TAGGED HASH ==================== */
static void tagged_hash(unsigned char *out, const char *tag, const unsigned char *data, size_t len) {
    EVP_MD_CTX *ctx = EVP_MD_CTX_new();
    unsigned char taghash[32];
    EVP_DigestInit_ex(ctx, EVP_sha256(), NULL);
    EVP_DigestUpdate(ctx, tag, strlen(tag));
    EVP_DigestFinal_ex(ctx, taghash, NULL);

    EVP_DigestInit_ex(ctx, EVP_sha256(), NULL);
    EVP_DigestUpdate(ctx, taghash, 32);
    EVP_DigestUpdate(ctx, taghash, 32);
    EVP_DigestUpdate(ctx, data, len);
    EVP_DigestFinal_ex(ctx, out, NULL);
    EVP_MD_CTX_free(ctx);
}

/* ==================== BECH32M FOR TAPROOT ==================== */
static const char *BECH32_CHARSET = "qpzry9x8gf2tvdw0s3jn54khce6mua7l";

static int convert_bits(unsigned char *out, size_t *out_len, const unsigned char *in, size_t in_len, int from_bits, int to_bits, int pad) {
    uint32_t acc = 0;
    int bits = 0;
    size_t out_idx = 0;
    uint32_t maxv = (1 << to_bits) - 1;

    for (size_t i = 0; i < in_len; ++i) {
        acc = (acc << from_bits) | in[i];
        bits += from_bits;
        while (bits >= to_bits) {
            bits -= to_bits;
            out[out_idx++] = (acc >> bits) & maxv;
        }
    }
    if (pad) {
        if (bits) out[out_idx++] = (acc << (to_bits - bits)) & maxv;
    } else if (bits >= from_bits || ((acc << (to_bits - bits)) & maxv)) {
        return 0;
    }
    *out_len = out_idx;
    return 1;
}

static uint32_t bech32_polymod(uint32_t pre) {
    uint32_t b = pre >> 25;
    return ((pre & 0x1FFFFFF) << 5) ^ (-((b >> 0) & 1) & 0x3b6a57b2) ^
           (-((b >> 1) & 1) & 0x26508e6d) ^ (-((b >> 2) & 1) & 0x1ea119fa) ^
           (-((b >> 3) & 1) & 0x3d4233dd) ^ (-((b >> 4) & 1) & 0x2a1462b3);
}

static void bech32_encode(char *out, const char *hrp, const unsigned char *data, size_t data_len) {
    unsigned char tmp[65];
    size_t tmp_len = 0;

    /* The first byte is the witness version (0-16), keep it as-is for 5-bit encoding */
    /* The remaining bytes (witness program) get converted from 8 to 5 bits */
    unsigned char witness_version = data[0];

    if (data_len < 2) {
        fprintf(stderr, "ERROR: Invalid bech32 data length\n");
        return;
    }

    /* Manually handle witness version as first 5-bit value */
    tmp[0] = witness_version & 0x1F;  /* Keep only lower 5 bits */

    /* Convert the remaining bytes (witness program) from 8 to 5 bits */
    size_t program_len = data_len - 1;
    uint32_t acc = 0;
    int bits = 0;
    size_t tmp_idx = 1;
    uint32_t maxv = 31;  /* (1 << 5) - 1 */

    for (size_t i = 0; i < program_len; ++i) {
        acc = (acc << 8) | data[i + 1];
        bits += 8;
        while (bits >= 5) {
            bits -= 5;
            tmp[tmp_idx++] = (acc >> bits) & maxv;
        }
    }
    if (bits) {
        tmp[tmp_idx++] = (acc << (5 - bits)) & maxv;
    }
    tmp_len = tmp_idx;

    size_t hrp_len = strlen(hrp);
    uint32_t chk = 1;
    for (size_t i = 0; i < hrp_len; ++i) chk = bech32_polymod(chk) ^ (hrp[i] >> 5);
    chk = bech32_polymod(chk);
    for (size_t i = 0; i < hrp_len; ++i) chk = bech32_polymod(chk) ^ (hrp[i] & 31);
    for (size_t i = 0; i < tmp_len; ++i) chk = bech32_polymod(chk) ^ tmp[i];

    /* Use Bech32m for witness version 1+, Bech32 for version 0 */
    uint32_t bech32_const = (witness_version == 0) ? 0x2bc830a3 : 0x98f2bc8e;
    chk ^= bech32_const;

    size_t idx = 0;
    for (size_t i = 0; i < hrp_len; ++i) out[idx++] = hrp[i];
    out[idx++] = '1';
    for (size_t i = 0; i < tmp_len; ++i) out[idx++] = BECH32_CHARSET[tmp[i]];
    for (size_t i = 0; i < 6; ++i) {
        out[idx++] = BECH32_CHARSET[(chk >> (5 * (5 - i))) & 31];
    }
    out[idx] = '\0';
}

/* ==================== BIP-341 TAPROOT + TAPLEAF ==================== */
static char* generate_taproot_address(const unsigned char *internal_key_32) {
    unsigned char script[34];
    script[0] = 0xc0;
    script[1] = 0x20;
    memcpy(script + 2, internal_key_32, 32);

    unsigned char leaf_hash[32];
    tagged_hash(leaf_hash, "TapLeaf", script, 34);

    unsigned char merkle_root[32];
    memcpy(merkle_root, leaf_hash, 32);

    unsigned char tweak_data[64];
    memcpy(tweak_data, internal_key_32, 32);
    memcpy(tweak_data + 32, merkle_root, 32);

    unsigned char tweak[32];
    tagged_hash(tweak, "TapTweak", tweak_data, 64);

    const EC_GROUP *group = EC_GROUP_new_by_curve_name(NID_secp256k1);

    BIGNUM *x_bn = BN_bin2bn(internal_key_32, 32, NULL);
    EC_POINT *int_point = EC_POINT_new(group);
    EC_POINT_set_compressed_coordinates_GFp(group, int_point, x_bn, 0, NULL);
    BN_free(x_bn);

    BIGNUM *tweak_bn = BN_bin2bn(tweak, 32, NULL);
    EC_POINT *tweak_point = EC_POINT_new(group);
    EC_POINT_mul(group, tweak_point, tweak_bn, NULL, NULL, NULL);

    EC_POINT_add(group, int_point, int_point, tweak_point, NULL);

    unsigned char *buf = malloc(65);
    size_t len = EC_POINT_point2oct(group, int_point, POINT_CONVERSION_COMPRESSED, buf, 65, NULL);
    unsigned char output_key_32[32];
    memcpy(output_key_32, buf + 1, 32);

    unsigned char data[33] = {0x01};
    memcpy(data + 1, output_key_32, 32);

    char *addr = malloc(80);
    bech32_encode(addr, "bc", data, 33);

    BN_free(tweak_bn);
    EC_POINT_free(int_point); EC_POINT_free(tweak_point);
    EC_GROUP_free((EC_GROUP*)group);
    free(buf);
    return addr;
}

static char* build_descriptor_pkh(const char* wif) {
    char *desc = malloc(100);
    snprintf(desc, 100, "pkh(%s)", wif);
    return desc;
}

static char* build_descriptor_tr(const char* pubkey_hex) {
    char *desc = malloc(100);
    snprintf(desc, 100, "tr(%s)", pubkey_hex);
    return desc;
}

/* ==================== MAIN ==================== */
int main(int argc, char** argv) {
    if (argc < 3) {
        fprintf(stderr, "Usage: %s <kchain_file> <role_number>\n", argv[0]);
        return 1;
    }

    const char* kchain_file = argv[1];
    int role = atoi(argv[2]);

    printf("🚀 Starting SPHINCS++ Hybrid BTC Builder (Full BIP-341 Taproot + Tapleaf)\n");
    printf("   Kchain: %s\n   Role: %d\n\n", kchain_file, role);

    json_t* root = json_load_file(kchain_file, 0, NULL);
    if (!root) {
        fprintf(stderr, "ERROR: Failed to load kchain file\n");
        return 1;
    }

    json_t* keys = json_object_get(root, "keys");
    json_t* roles = json_object_get(keys, "roles");

    json_t* role_obj = NULL;
    size_t index;
    int found = 0;
    json_array_foreach(roles, index, role_obj) {
        json_t* r = json_object_get(role_obj, "role");
        if (r && json_integer_value(r) == role) {
            found = 1;
            break;
        }
    }
    if (!found || !role_obj) {
        fprintf(stderr, "ERROR: Role %d not found\n", role);
        json_decref(root);
        return 1;
    }

    const char* hybrid_sk_hex = json_string_value(json_object_get(role_obj, "sphincs128s_hybrid_sk"));
    json_t* btc_obj = json_object_get(role_obj, "bitcoin");
    const char* btc_priv_hex = json_string_value(json_object_get(btc_obj, "private_key_hex"));

    if (!hybrid_sk_hex || !btc_priv_hex) {
        fprintf(stderr, "ERROR: Missing keys for role %d\n", role);
        json_decref(root);
        return 1;
    }

    char timestamp[32] = "unknown";
    {
        char path_copy[1024];
        strncpy(path_copy, kchain_file, sizeof(path_copy)-1);
        path_copy[sizeof(path_copy)-1] = '\0';
        char *base = strrchr(path_copy, '/');
        if (!base) base = path_copy; else base++;
        char *ts_start = strstr(base, "pqc_master_");
        if (ts_start) {
            ts_start += 11;
            strncpy(timestamp, ts_start, 15);
            timestamp[15] = '\0';
        }
    }

    unsigned char btc_priv_bytes[32];
    for (size_t i = 0; i < 32; i++) sscanf(btc_priv_hex + 2*i, "%2hhx", &btc_priv_bytes[i]);

    unsigned char state[32];
    EVP_MD_CTX* ctx = EVP_MD_CTX_new();
    EVP_DigestInit_ex(ctx, EVP_sha3_256(), NULL);
    EVP_DigestUpdate(ctx, btc_priv_bytes, 32);
    EVP_DigestUpdate(ctx, (const unsigned char*)"SPHINCS++SEEDv1", 15);
    EVP_DigestFinal_ex(ctx, state, NULL);
    EVP_MD_CTX_free(ctx);

    char* trits = malloc(513);
    for (int i = 0; i < 512; i++) trits[i] = '0' + (state[i % 32] % 3);
    trits[512] = '\0';
    char* cleaned = spx_qec_cleanup(trits, 20);
    free(trits);

    OQS_SIG* sig = OQS_SIG_new("SLH_DSA_PURE_SHA2_128S");
    if (!sig) { fprintf(stderr, "ERROR: SLH_DSA not available\n"); exit(1); }

    uint8_t* hybrid_sk = malloc(sig->length_secret_key);
    size_t sk_len = strlen(hybrid_sk_hex) / 2;
    for (size_t i = 0; i < sk_len; i++) sscanf(hybrid_sk_hex + 2*i, "%2hhx", &hybrid_sk[i]);

    uint8_t* sphincs_sig = malloc(sig->length_signature);
    size_t sphincs_sig_len = 0;
    OQS_SIG_sign(sig, sphincs_sig, &sphincs_sig_len, (const unsigned char*)cleaned, strlen(cleaned), hybrid_sk);

    free(cleaned);
    OQS_SIG_free(sig);
    free(hybrid_sk);

    unsigned char sphincs_plus_output[32];
    memcpy(sphincs_plus_output, sphincs_sig, 32);
    free(sphincs_sig);

    char* sphincs_plus_hex = bytes_to_hex(sphincs_plus_output, 32);

    unsigned char *real_priv = NULL, *real_pub = NULL;
    size_t real_priv_len, real_pub_len;
    generate_btc_keypair_from_seed(sphincs_plus_output, &real_priv, &real_priv_len, &real_pub, &real_pub_len);
    char* real_wif = base58_encode(real_priv, 32);

    unsigned char linked_seed[32];
    ctx = EVP_MD_CTX_new();
    EVP_DigestInit_ex(ctx, EVP_sha3_256(), NULL);
    EVP_DigestUpdate(ctx, sphincs_plus_output, 32);
    EVP_DigestUpdate(ctx, (const unsigned char*)"LINKEDv1", 8);
    EVP_DigestFinal_ex(ctx, linked_seed, NULL);
    EVP_MD_CTX_free(ctx);

    unsigned char *linked_priv = NULL, *linked_pub = NULL;
    size_t linked_priv_len, linked_pub_len;
    generate_btc_keypair_from_seed(linked_seed, &linked_priv, &linked_priv_len, &linked_pub, &linked_pub_len);
    char* linked_wif = base58_encode(linked_priv, 32);

    unsigned char sphincs_plus_sk[64];
    memcpy(sphincs_plus_sk, sphincs_plus_output, 32);
    ctx = EVP_MD_CTX_new();
    EVP_DigestInit_ex(ctx, EVP_sha3_256(), NULL);
    EVP_DigestUpdate(ctx, sphincs_plus_output, 32);
    EVP_DigestFinal_ex(ctx, sphincs_plus_sk + 32, NULL);
    EVP_MD_CTX_free(ctx);

    char* sphincs_plus_pk_hex = bytes_to_hex(sphincs_plus_output, 32);
    char* sphincs_plus_sk_hex = bytes_to_hex(sphincs_plus_sk, 64);

    char* taproot_addr = generate_taproot_address(sphincs_plus_output);

    const char *output_dir = "../svc-wallet";
    if (mkdir(output_dir, 0755) != 0 && errno != EEXIST) perror("WARNING: Could not create ../svc-wallet");

    char dir_path[512];
    snprintf(dir_path, sizeof(dir_path), "%s/%s", output_dir, timestamp);
    if (mkdir(dir_path, 0755) != 0 && errno != EEXIST) perror("WARNING: Could not create timestamp folder");

    char cmd[2048];
    snprintf(cmd, sizeof(cmd), "cp \"%s\" \"%s/\" 2>/dev/null || true", kchain_file, dir_path);
    system(cmd);
    snprintf(cmd, sizeof(cmd), "cp ../svc-wallet/*%s*.msg \"%s/\" 2>/dev/null || true", timestamp, dir_path);
    system(cmd);

    char short_addr[7] = {0};
    strncpy(short_addr, taproot_addr, 3);
    size_t addr_len = strlen(taproot_addr);
    if (addr_len > 3) strncpy(short_addr + 3, taproot_addr + addr_len - 3, 3);
    short_addr[6] = '\0';

    char filename[512];
    snprintf(filename, sizeof(filename), "%s_%s.sphincs++", short_addr, timestamp);

    json_t *out_json = json_object();
    json_object_set_new(out_json, "kchain_file", json_string(kchain_file));
    json_object_set_new(out_json, "role", json_integer(role));
    json_object_set_new(out_json, "timestamp", json_string(timestamp));
    json_object_set_new(out_json, "sphincs_plus_output", json_string(sphincs_plus_hex));

    char import_buf[1024];

    json_t *real_btc = json_object();
    json_object_set_new(real_btc, "private_key_hex", json_string(bytes_to_hex(real_priv, 32)));
    json_object_set_new(real_btc, "public_key_hex", json_string(bytes_to_hex(real_pub, real_pub_len)));
    json_object_set_new(real_btc, "wif", json_string(real_wif));
    json_object_set_new(real_btc, "descriptor", json_string(build_descriptor_pkh(real_wif)));
    json_object_set_new(real_btc, "alternative_descriptor", json_string(build_descriptor_tr(bytes_to_hex(sphincs_plus_output, 32))));
    snprintf(import_buf, sizeof(import_buf), "bitcoin-cli importdescriptors '[{\"desc\": \"%s\", \"timestamp\": \"now\", \"internal\": false}]'", build_descriptor_pkh(real_wif));
    json_object_set_new(real_btc, "import_command_pkh", json_string(import_buf));
    snprintf(import_buf, sizeof(import_buf), "bitcoin-cli importdescriptors '[{\"desc\": \"%s\", \"timestamp\": \"now\", \"internal\": false}]'", build_descriptor_tr(bytes_to_hex(sphincs_plus_output, 32)));
    json_object_set_new(real_btc, "import_command_tr", json_string(import_buf));
    json_object_set_new(out_json, "bitcoin_keyset_real", real_btc);

    json_t *linked_btc = json_object();
    json_object_set_new(linked_btc, "private_key_hex", json_string(bytes_to_hex(linked_priv, 32)));
    json_object_set_new(linked_btc, "public_key_hex", json_string(bytes_to_hex(linked_pub, linked_pub_len)));
    json_object_set_new(linked_btc, "wif", json_string(linked_wif));
    json_object_set_new(linked_btc, "descriptor", json_string(build_descriptor_pkh(linked_wif)));
    json_object_set_new(linked_btc, "alternative_descriptor", json_string(build_descriptor_tr(bytes_to_hex(linked_seed, 32))));
    snprintf(import_buf, sizeof(import_buf), "bitcoin-cli importdescriptors '[{\"desc\": \"%s\", \"timestamp\": \"now\", \"internal\": false}]'", build_descriptor_pkh(linked_wif));
    json_object_set_new(linked_btc, "import_command_pkh", json_string(import_buf));
    snprintf(import_buf, sizeof(import_buf), "bitcoin-cli importdescriptors '[{\"desc\": \"%s\", \"timestamp\": \"now\", \"internal\": false}]'", build_descriptor_tr(bytes_to_hex(linked_seed, 32)));
    json_object_set_new(linked_btc, "import_command_tr", json_string(import_buf));
    json_object_set_new(out_json, "bitcoin_keyset_linked", linked_btc);

    json_t *compact_spx = json_object();
    json_object_set_new(compact_spx, "public_key_hex", json_string(sphincs_plus_pk_hex));
    json_object_set_new(compact_spx, "secret_key_hex", json_string(sphincs_plus_sk_hex));
    json_object_set_new(out_json, "sphincs_plus_compact", compact_spx);

    json_t *proof = json_object();
    json_object_set_new(proof, "taproot_address", json_string(taproot_addr));
    json_object_set_new(proof, "taproot_internal_key_hex", json_string(bytes_to_hex(sphincs_plus_output, 32)));
    json_object_set_new(proof, "tapleaf_commitment", json_string(sphincs_plus_hex));
    json_object_set_new(proof, "proof_type", json_string("taproot_tapleaf_sphincs++"));
    json_object_set_new(out_json, "quantum_proof", proof);

    json_object_set_new(out_json, "generated_at", json_string("now"));
    json_object_set_new(out_json, "note", json_string("SPHINCS++ quantum-linked hybrid - verifiable with robust verifier"));

    char full_path[1024];
    snprintf(full_path, sizeof(full_path), "%s/%s/%s", "../svc-wallet", timestamp, filename);

    if (json_dump_file(out_json, full_path, JSON_INDENT(2)) == 0) {
        printf("✅ SPHINCS++ file saved:\n   %s\n\n", full_path);
        printf("   Quantum Taproot proof address: %s\n", taproot_addr);
        printf("   SPHINCS++ output (32 bytes): %s\n\n", sphincs_plus_hex);

        unlink(kchain_file);
        snprintf(cmd, sizeof(cmd), "rm -f ../svc-wallet/*%s*.msg 2>/dev/null", timestamp);
        system(cmd);
        printf("🧹 Original files cleaned up\n");
        printf("🎉 All files in ../svc-wallet/%s/\n", timestamp);
    } else {
        fprintf(stderr, "ERROR: Could not write %s\n", full_path);
        perror("Reason");
    }

    free(sphincs_plus_hex); free(sphincs_plus_pk_hex); free(sphincs_plus_sk_hex);
    free(taproot_addr);
    free(real_priv); free(real_pub); free(real_wif);
    free(linked_priv); free(linked_pub); free(linked_wif);
    json_decref(out_json);
    json_decref(root);

    return 0;
}
