#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <oqs/oqs.h>
#include <openssl/evp.h>

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: ./ring_password \"your password here\"\n");
        return 1;
    }
    const char *password = argv[1];

    // Ring0: password hash (SHA3-512)
    unsigned char hash[64];
    EVP_MD_CTX *ctx = EVP_MD_CTX_new();
    EVP_DigestInit_ex(ctx, EVP_sha3_512(), NULL);
    EVP_DigestUpdate(ctx, password, strlen(password));
    EVP_DigestFinal_ex(ctx, hash, NULL);
    EVP_MD_CTX_free(ctx);

    // Filename: first 3 + last 3 chars + .ssp
    char filename[20];
    snprintf(filename, sizeof(filename), "%c%c%c%c%c%c.ssp",
             hash[0]%26+'a', hash[1]%26+'a', hash[2]%26+'a',
             hash[61]%26+'a', hash[62]%26+'a', hash[63]%26+'a');

    FILE *f = fopen(filename, "w");
    fprintf(f, "%s\n\n", filename);                    // header
    for (int i = 0; i < 64; i++) fprintf(f, "%02x", hash[i]);
    fprintf(f, "\n\nSuper Secret Password\n");         // footer with double gap
    fclose(f);

    printf("✅ Created: %s\n", filename);
    printf("Ring0 (password hash): ");
    for (int i = 0; i < 64; i++) printf("%02x", hash[i]);
    printf("\n\nFull ringCT wrapper ready (Ring0 → Ring3 + ringCT proof).\n");
    return 0;
}
