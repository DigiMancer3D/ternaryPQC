/*
 * Diagnostic: List every signature scheme your liboqs supports
 */
#include <oqs/oqs.h>
#include <stdio.h>

int main(void) {
    printf("=== Supported PQC Signature Schemes in your liboqs ===\n\n");
    for (int i = 0; ; i++) {
        const char *name = OQS_SIG_alg_identifier(i);
        if (!name) break;

        OQS_SIG *sig = OQS_SIG_new(name);
        if (sig) {
            printf("  ✓ %s  (enabled)\n", name);
            OQS_SIG_free(sig);
        } else {
            printf("  ✗ %s  (not enabled in this build)\n", name);
        }
    }
    printf("\n=== End of list ===\n");
    return 0;
}
