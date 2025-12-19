#include "handlers/tls/handshake/certificate_verify.h"

#include <openssl/evp.h>

uint32_t tls_write_certificate_verify(uint16_t signature_scheme, uint16_t signature_length, 
        void* signature, void* to_write) {
    struct tls_certificate_verify_msg_t* cert_verify = (struct tls_certificate_verify_msg_t*) to_write;
    cert_verify->signature_scheme = htons(signature_scheme);
    cert_verify->signature_length = htons(signature_length);
    memcpy((uint8_t*) to_write + sizeof(struct tls_certificate_verify_msg_t), signature, signature_length);

    return sizeof(struct tls_certificate_verify_msg_t) + signature_length;
}


void create_verify_signature() {

    // TODO: Implement signature creation logic
    // Use OpenSSL EVP_PKEY_sign or similar functions
    // Example:
    EVP_PKEY* pkey; // Load or generate your private key
    EVP_MD_CTX* mdctx = EVP_MD_CTX_new();
    if(!mdctx) {
        // Handle error
        return;
    }
    if(1 != EVP_DigestSignInit(mdctx, NULL, EVP_sha256(), NULL, pkey)) {
        // Handle error
        EVP_MD_CTX_free(mdctx);
        return;
    }
    // Add data to be signed
    // ...
    size_t siglen;
    if(1 != EVP_DigestSignFinal(mdctx, NULL, &siglen))
    {
        // Handle error
        EVP_MD_CTX_free(mdctx);
        return;
    }
    unsigned char* sig = (unsigned char*)OPENSSL_malloc(siglen);
    if(!sig) {
        // Handle error
        EVP_MD_CTX_free(mdctx);
        return;
    }
    if(1 != EVP_DigestSignFinal(mdctx, sig, &siglen))
    {
        // Handle error
        OPENSSL_free(sig);
        EVP_MD_CTX_free(mdctx);
        return;
    }
    // Use the signature (sig) and its length (siglen)
    // ...
    OPENSSL_free(sig);
    EVP_MD_CTX_free(mdctx);
}