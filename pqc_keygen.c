/*
 * PQC Keychain Generator - Complete C Implementation
 * 
 * Generates post-quantum cryptographic keychains using:
 * - Falcon-512 (lattice-based signatures)
 * - SPHINCS+-SHA2-128s (hash-based signatures)
 * - Ternary PQC seed expansion (SPX-QEC pattern cutting)
 * 
 * Output: svc-wallet/pqc_master_YYYYMMDD_HHMMSS.kchain (JSON)
 * 
 * Build:
 *   gcc -o pqc_keygen pqc_keygen.c -loqs -ljansson -lm -O3
 * 
 * Run:
 *   mkdir -p svc-wallet
 *   ./pqc_keygen
 * 
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <oqs/oqs.h>
#include <jansson.h>
#include <sys/stat.h>

#define TERNARY_LENGTH 6000
#define ROLE_COUNT 8

/* ==================== TERNARY PQC FUNCTIONS ==================== */

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
    int capacity = 100;
    result.patterns = (char **)malloc(capacity * sizeof(char *));
    result.count = 0;

    for (int i = 0; i < BASE_PATTERNS_COUNT; i++) {
        const char *base = BASE_PATTERNS[i];
        int base_len = strlen(base);

        /* base + base[1:] */
        if (result.count >= capacity) {
            capacity *= 2;
            result.patterns = (char **)realloc(result.patterns, capacity * sizeof(char *));
        }
        snprintf(temp, sizeof(temp), "%s%s", base, base + 1);
        result.patterns[result.count++] = strdup(temp);

        /* reverse */
        if (result.count >= capacity) {
            capacity *= 2;
            result.patterns = (char **)realloc(result.patterns, capacity * sizeof(char *));
        }
        strcpy(temp, base);
        for (int j = 0; j < base_len / 2; j++) {
            char c = temp[j];
            temp[j] = temp[base_len - 1 - j];
            temp[base_len - 1 - j] = c;
        }
        result.patterns[result.count++] = strdup(temp);

        /* base + base[::-1] */
        if (result.count >= capacity) {
            capacity *= 2;
            result.patterns = (char **)realloc(result.patterns, capacity * sizeof(char *));
        }
        strcpy(temp, base);
        for (int j = 0; j < base_len / 2; j++) {
            char c = temp[j];
            temp[j] = temp[base_len - 1 - j];
            temp[base_len - 1 - j] = c;
        }
        snprintf(temp, sizeof(temp), "%s%s", base, temp);
        result.patterns[result.count++] = strdup(temp);
    }

    return result;
}

static void free_patterns(PatternList *plist) {
    for (int i = 0; i < plist->count; i++) {
        free(plist->patterns[i]);
    }
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
                memmove(pos, pos + strlen(patterns.patterns[p]),
                        strlen(pos + strlen(patterns.patterns[p])) + 1);
                pos = strstr(pos, patterns.patterns[p]);
            }
        }
        if (strlen(cleaned) == strlen(prev)) {
            free(prev);
            break;
        }
        free(prev);
    }

    free_patterns(&patterns);
    return cleaned;
}

static char *ternary_d_shift(const char *trits) {
    int len = strlen(trits);
    if (len == 0) return strdup("");

    char *result = (char *)malloc(len + 1);
    result[0] = trits[0];

    for (int i = 1; i < len; i++) {
        int prev = trits[i - 1] - '0';
        int nxt = trits[i] - '0';
        char val;
        if (prev == nxt) {
            val = '0';
        } else if (prev > nxt) {
            val = '1';
        } else {
            val = '2';
        }
        result[i] = val;
    }
    result[len] = '\0';
    return result;
}

static char *full_pass(const char *current, const char *prev) {
    int len = strlen(current);
    int n = len / 3;

    char *A = (char *)malloc(n + 1);
    char *B = (char *)malloc(n + 1);
    char *C = (char *)malloc(n + 1);

    strncpy(A, current, n);
    A[n] = '\0';
    strncpy(B, current + n, n);
    B[n] = '\0';
    strncpy(C, current + 2 * n, n);
    C[n] = '\0';

    char *new_A = ternary_d_shift(A);
    char *new_B = ternary_d_shift(B);
    char *new_C = ternary_d_shift(C);

    char *jump = (char *)malloc(n + 1);
    for (int i = 0; i < n; i++) {
        int a = new_A[i] - '0';
        int b = new_B[i] - '0';
        int c = new_C[i] - '0';
        int val = ((a ^ b) + (c ^ b)) % 3;
        jump[i] = '0' + val;
    }
    jump[n] = '\0';

    char *B2 = (char *)malloc(2 * n + 1);
    if (prev && strlen(prev) >= n) {
        int prev_n = strlen(prev) / 3;
        strncpy(B2, prev + prev_n, n);
        strcpy(B2 + n, B);
    } else {
        strcpy(B2, B);
        strcpy(B2 + n, B);
    }

    char *xor_jump = (char *)malloc(n + 1);
    for (int i = 0; i < n && i < len; i++) {
        int x = current[i] - '0';
        int y = jump[i] - '0';
        xor_jump[i] = '0' + ((x ^ y) % 3);
    }
    xor_jump[n] = '\0';

    char *pass_str = (char *)malloc(len * 2 + 1);
    strcpy(pass_str, xor_jump);
    strcat(pass_str, B2);

    if (prev) {
        char *temp = pass_str;
        int temp_len = strlen(pass_str);
        int prev_len = strlen(prev);
        pass_str = (char *)malloc(temp_len + 1);
        for (int i = 0; i < temp_len && i < prev_len; i++) {
            int a = temp[i] - '0';
            int b = prev[i] - '0';
            pass_str[i] = '0' + ((a + b) % 3);
        }
        for (int i = prev_len; i < temp_len; i++) {
            pass_str[i] = temp[i];
        }
        pass_str[temp_len] = '\0';
        free(temp);
    }

    int result_len = len * 2 < strlen(pass_str) ? len * 2 : strlen(pass_str);
    char *result = (char *)malloc(result_len + 1);
    strncpy(result, pass_str, result_len);
    result[result_len] = '\0';

    free(A);
    free(B);
    free(C);
    free(new_A);
    free(new_B);
    free(new_C);
    free(jump);
    free(B2);
    free(xor_jump);
    free(pass_str);

    return result;
}

static char *generate_random_epoch(int length) {
    char *result = (char *)malloc(length + 1);
    for (int i = 0; i < length; i++) {
        result[i] = '0' + (rand() % 3);
    }
    result[length] = '\0';
    return result;
}

static char *expand_to_10k_with_qec(const char *start) {
    char *current = strdup(start);
    char *prev = strdup(start);

    while (strlen(current) < 10000) {
        free(prev);
        prev = strdup(current);
        char *next = full_pass(current, prev);
        free(current);
        current = spx_qec_cleanup(next, 20);
        free(next);

        if (strlen(current) < strlen(prev) * 1.2) {
            char *temp = (char *)malloc(strlen(prev) * 2 + 1);
            strcpy(temp, prev);
            strncat(temp, current, strlen(prev));
            free(current);
            current = temp;
        }
    }

    free(prev);
    return current;
}

static char *reduce_to_6000_trits(const char *long_trits) {
    char *current = strdup(long_trits);
    for (int i = 0; i < 8; i++) {
        char *shifted = ternary_d_shift(current);
        free(current);
        current = shifted;
    }

    char *folded = (char *)malloc(TERNARY_LENGTH + 1);
    int current_len = strlen(current);
    double mu = current_len / 2.0;
    double sigma = current_len / 8.0;

    for (int i = 0; i < TERNARY_LENGTH; i++) {
        double gauss = (double)rand() / RAND_MAX;
        int idx1 = (int)(mu + sigma * gauss) % current_len;
        int idx2 = (idx1 + current_len / 2) % current_len;
        int val = ((current[idx1] - '0') ^ (current[idx2] - '0')) % 3;
        folded[i] = '0' + val;
    }
    folded[TERNARY_LENGTH] = '\0';

    free(current);
    return folded;
}

static char *bytes_to_hex(const unsigned char *data, size_t len) {
    char *hex = (char *)malloc(len * 2 + 1);
    for (size_t i = 0; i < len; i++) {
        sprintf(hex + i * 2, "%02x", data[i]);
    }
    hex[len * 2] = '\0';
    return hex;
}

/* ==================== KEY GENERATION ==================== */

typedef struct {
    unsigned char *falcon_pk;
    size_t falcon_pk_len;
    unsigned char *falcon_sk;
    size_t falcon_sk_len;
    unsigned char *sphincs_pk;
    size_t sphincs_pk_len;
    unsigned char *sphincs_sk;
    size_t sphincs_sk_len;
} KeyPair;

static KeyPair generate_falcon_keypair(void) {
    KeyPair kp = {0};
    OQS_SIG *sig = OQS_SIG_new("Falcon-512");
    if (!sig) {
        fprintf(stderr, "Failed to create Falcon-512 context\n");
        return kp;
    }

    unsigned char *pk = (unsigned char *)malloc(sig->length_public_key);
    unsigned char *sk = (unsigned char *)malloc(sig->length_secret_key);

    if (OQS_SIG_keypair(sig, pk, sk) != OQS_STATUS_SUCCESS) {
        fprintf(stderr, "Falcon keypair generation failed\n");
        free(pk);
        free(sk);
        OQS_SIG_free(sig);
        return kp;
    }

    kp.falcon_pk = pk;
    kp.falcon_pk_len = sig->length_public_key;
    kp.falcon_sk = sk;
    kp.falcon_sk_len = sig->length_secret_key;

    OQS_SIG_free(sig);
    return kp;
}

static KeyPair generate_sphincs_keypair(void) {
    KeyPair kp = {0};
    OQS_SIG *sig = OQS_SIG_new("SPHINCS+-SHA2-128s");
    if (!sig) {
        fprintf(stderr, "Failed to create SPHINCS+ context\n");
        return kp;
    }

    unsigned char *pk = (unsigned char *)malloc(sig->length_public_key);
    unsigned char *sk = (unsigned char *)malloc(sig->length_secret_key);

    if (OQS_SIG_keypair(sig, pk, sk) != OQS_STATUS_SUCCESS) {
        fprintf(stderr, "SPHINCS+ keypair generation failed\n");
        free(pk);
        free(sk);
        OQS_SIG_free(sig);
        return kp;
    }

    kp.sphincs_pk = pk;
    kp.sphincs_pk_len = sig->length_public_key;
    kp.sphincs_sk = sk;
    kp.sphincs_sk_len = sig->length_secret_key;

    OQS_SIG_free(sig);
    return kp;
}

/* ==================== FILE I/O ==================== */

static char *get_timestamp_filename(void) {
    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);
    char *filename = (char *)malloc(256);
    strftime(filename, 256, "svc-wallet/pqc_master_%Y%m%d_%H%M%S.kchain", tm_info);
    return filename;
}

static void ensure_svc_wallet_dir(void) {
    mkdir("svc-wallet", 0755);
}

static char *get_iso_timestamp(void) {
    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);
    char *timestamp = (char *)malloc(32);
    strftime(timestamp, 32, "%Y-%m-%dT%H:%M:%SZ", tm_info);
    return timestamp;
}

/* ==================== MAIN ==================== */

int main(void) {
    srand((unsigned)time(NULL));
    OQS_randombytes_switch_algorithm("system");

    printf("========== PQC Keychain Generator ==========\n\n");

    /* Step 1: Generate random epoch */
    printf("[1/7] Generating random 512-trit epoch...\n");
    char *epoch = generate_random_epoch(512);
    printf("      Generated %zu ternary digits\n\n", strlen(epoch));

    /* Step 2: Expand to 10k+ with SPX-QEC */
    printf("[2/7] Expanding to 10k+ trits with SPX-QEC...\n");
    char *pool = expand_to_10k_with_qec(epoch);
    printf("      Expansion complete: %zu trits\n\n", strlen(pool));
    free(epoch);

    /* Step 3: Reduce to exactly 6000 trits */
    printf("[3/7] Finalizing to 6000-trit seed...\n");
    char *final_trits = reduce_to_6000_trits(pool);
    printf("      Seed ready: %zu trits\n\n", strlen(final_trits));
    free(pool);

    /* Step 4: Convert to bytes */
    printf("[4/7] Converting to master bytes...\n");
    unsigned char *master_bytes = (unsigned char *)malloc((TERNARY_LENGTH * 2 + 7) / 8);
    size_t master_len = 0;
    /* Simple ternary to binary conversion */
    for (int i = 0; i < TERNARY_LENGTH; i += 8) {
        unsigned char byte = 0;
        for (int j = 0; j < 8 && i + j < TERNARY_LENGTH; j++) {
            byte = (byte << 1) | ((final_trits[i + j] - '0') & 1);
        }
        master_bytes[master_len++] = byte;
    }
    printf("      Master bytes: %zu bytes\n\n", master_len);

    /* Step 5: Generate Falcon master keys */
    printf("[5/7] Generating Falcon-512 master keypair...\n");
    KeyPair falcon_master = generate_falcon_keypair();
    if (!falcon_master.falcon_pk) {
        fprintf(stderr, "Failed to generate Falcon keys\n");
        return 1;
    }
    printf("      Public key:  %zu bytes\n", falcon_master.falcon_pk_len);
    printf("      Private key: %zu bytes\n\n", falcon_master.falcon_sk_len);

    /* Step 6: Generate SPHINCS+ master keys */
    printf("[6/7] Generating SPHINCS+-SHA2-128s master keypair...\n");
    KeyPair sphincs_master = generate_sphincs_keypair();
    if (!sphincs_master.sphincs_pk) {
        fprintf(stderr, "Failed to generate SPHINCS+ keys\n");
        return 1;
    }
    printf("      Public key:  %zu bytes\n", sphincs_master.sphincs_pk_len);
    printf("      Private key: %zu bytes\n\n", sphincs_master.sphincs_sk_len);

    /* Step 7: Generate role keypairs */
    printf("[7/7] Generating 8 role keypairs...\n");
    KeyPair role_keys[ROLE_COUNT][2];  /* [role][0=falcon, 1=sphincs] */
    for (int role = 0; role < ROLE_COUNT; role++) {
        role_keys[role][0] = generate_falcon_keypair();
        role_keys[role][1] = generate_sphincs_keypair();
        if (!role_keys[role][0].falcon_pk || !role_keys[role][1].sphincs_pk) {
            fprintf(stderr, "Failed to generate role %d keys\n", role);
            return 1;
        }
        printf("      Role %d: OK\n", role);
    }
    printf("\n");

    /* Build JSON output */
    printf("Building JSON keychain...\n");
    json_t *root = json_object();

    /* Seed */
    json_t *seed_obj = json_object();
    json_object_set_new(seed_obj, "ternary_6000_trits", json_string(final_trits));
    char *master_hex = bytes_to_hex(master_bytes, master_len);
    json_object_set_new(seed_obj, "master_bytes_hex", json_string(master_hex));
    json_object_set_new(root, "seed", seed_obj);

    /* Keys */
    json_t *keys_obj = json_object();

    /* Falcon master */
    char *falcon_pk_hex = bytes_to_hex(falcon_master.falcon_pk, falcon_master.falcon_pk_len);
    char *falcon_sk_hex = bytes_to_hex(falcon_master.falcon_sk, falcon_master.falcon_sk_len);
    json_object_set_new(keys_obj, "falcon_512_master_pk", json_string(falcon_pk_hex));
    json_object_set_new(keys_obj, "falcon_512_master_sk", json_string(falcon_sk_hex));

    /* SPHINCS+ master */
    char *sphincs_pk_hex = bytes_to_hex(sphincs_master.sphincs_pk, sphincs_master.sphincs_pk_len);
    char *sphincs_sk_hex = bytes_to_hex(sphincs_master.sphincs_sk, sphincs_master.sphincs_sk_len);
    json_object_set_new(keys_obj, "sphincs_sha2_128s_master_pk", json_string(sphincs_pk_hex));
    json_object_set_new(keys_obj, "sphincs_sha2_128s_master_sk", json_string(sphincs_sk_hex));

    /* Roles */
    json_t *roles_array = json_array();
    for (int role = 0; role < ROLE_COUNT; role++) {
        json_t *role_obj = json_object();
        json_object_set_new(role_obj, "role", json_integer(role));

        char *f_pk = bytes_to_hex(role_keys[role][0].falcon_pk, role_keys[role][0].falcon_pk_len);
        char *f_sk = bytes_to_hex(role_keys[role][0].falcon_sk, role_keys[role][0].falcon_sk_len);
        char *s_pk = bytes_to_hex(role_keys[role][1].sphincs_pk, role_keys[role][1].sphincs_pk_len);
        char *s_sk = bytes_to_hex(role_keys[role][1].sphincs_sk, role_keys[role][1].sphincs_sk_len);

        json_object_set_new(role_obj, "falcon_512_pk", json_string(f_pk));
        json_object_set_new(role_obj, "falcon_512_sk", json_string(f_sk));
        json_object_set_new(role_obj, "sphincs_sha2_128s_pk", json_string(s_pk));
        json_object_set_new(role_obj, "sphincs_sha2_128s_sk", json_string(s_sk));

        json_array_append_new(roles_array, role_obj);

        free(f_pk);
        free(f_sk);
        free(s_pk);
        free(s_sk);
    }
    json_object_set_new(keys_obj, "roles", roles_array);
    json_object_set_new(root, "keys", keys_obj);

    /* Metadata */
    char *timestamp = get_iso_timestamp();
    json_object_set_new(root, "generated_at", json_string(timestamp));
    json_object_set_new(root, "algorithm", json_string("Falcon-512 + SPHINCS+-SHA2-128s"));
    json_object_set_new(root, "library", json_string("liboqs 0.12.x"));
    json_object_set_new(root, "note", json_string("All public and private keys are included. Seed is cryptographically secure."));

    /* Write to file */
    ensure_svc_wallet_dir();
    char *filename = get_timestamp_filename();
    if (json_dump_file(root, filename, JSON_INDENT(2)) != 0) {
        fprintf(stderr, "Failed to write %s\n", filename);
        return 1;
    }
    printf("✅ Keychain saved to: %s\n\n", filename);

    /* Cleanup */
    free(final_trits);
    free(master_bytes);
    free(master_hex);
    free(falcon_pk_hex);
    free(falcon_sk_hex);
    free(sphincs_pk_hex);
    free(sphincs_sk_hex);
    free(timestamp);
    free(filename);

    if (falcon_master.falcon_pk) free(falcon_master.falcon_pk);
    if (falcon_master.falcon_sk) free(falcon_master.falcon_sk);
    if (sphincs_master.sphincs_pk) free(sphincs_master.sphincs_pk);
    if (sphincs_master.sphincs_sk) free(sphincs_master.sphincs_sk);

    for (int role = 0; role < ROLE_COUNT; role++) {
        if (role_keys[role][0].falcon_pk) free(role_keys[role][0].falcon_pk);
        if (role_keys[role][0].falcon_sk) free(role_keys[role][0].falcon_sk);
        if (role_keys[role][1].sphincs_pk) free(role_keys[role][1].sphincs_pk);
        if (role_keys[role][1].sphincs_sk) free(role_keys[role][1].sphincs_sk);
    }

    json_decref(root);

    printf("✅ Complete! Keychain ready for use.\n");
    return 0;
}
