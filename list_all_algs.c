#include <oqs/oqs.h>
#include <stdio.h>

int main() {
    printf("Available signature algorithms:\n");
    
    const char *test_algs[] = {
        "Dilithium2", "Dilithium3", "Dilithium5",
        "Falcon-512", "Falcon-1024",
        "SPHINCS+-SHA2-128s", "SPHINCS+-SHA2-128f", "SPHINCS+-SHA2-192s",
        "SPHINCS+-SHAKE-128s", "SPHINCS+-SHAKE-128f", "SPHINCS+-SHAKE-192s",
        NULL
    };
    
    for (int i = 0; test_algs[i]; i++) {
        OQS_SIG *sig = OQS_SIG_new(test_algs[i]);
        printf("  %-30s %s\n", test_algs[i], sig ? "✓" : "✗");
        if (sig) OQS_SIG_free(sig);
    }
    return 0;
}
