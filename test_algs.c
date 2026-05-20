#include <oqs/oqs.h>
#include <stdio.h>

int main() {
    printf("Testing SPHINCS+ variants:\n");
    const char *algs[] = {
        "SPHINCS+-SHA2-128s",
        "SPHINCS+-SHAKE-128s",
        "SPHINCS+-SHA2-128s-robust",
        "SPHINCS+-SHAKE-128s-robust"
    };
    
    for (int i = 0; i < 4; i++) {
        OQS_SIG *sig = OQS_SIG_new(algs[i]);
        printf("  %s: %s\n", algs[i], sig ? "✓ AVAILABLE" : "✗ NOT FOUND");
        if (sig) OQS_SIG_free(sig);
    }
    return 0;
}
