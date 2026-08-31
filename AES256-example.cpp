#include <windows.h>
#include <wincrypt.h>
#include <stdio.h>
#include <string.h>

#pragma comment(lib, "advapi32.lib")

#define AES_BLOCK_SIZE 16
#define AES_256_KEY_SIZE 32

// Derive an AES-256 session key from a password using SHA-256.
// The hash object is destroyed before returning; the caller owns hKey.
static BOOL DeriveAes256Key(HCRYPTPROV hProv, const char* password, HCRYPTKEY* hKey)
{
    HCRYPTHASH hHash = 0;

    if (!CryptCreateHash(hProv, CALG_SHA_256, 0, 0, &hHash))
    {
        printf("CryptCreateHash failed with error: %lu\n", GetLastError());
        return FALSE;
    }

    if (!CryptHashData(hHash, (const BYTE*)password, (DWORD)strlen(password), 0))
    {
        printf("CryptHashData failed with error: %lu\n", GetLastError());
        CryptDestroyHash(hHash);
        return FALSE;
    }

    // CALG_AES_256 is a 256-bit block cipher (16-byte blocks, PKCS#5 padding).
    if (!CryptDeriveKey(hProv, CALG_AES_256, hHash, 0, hKey))
    {
        printf("CryptDeriveKey failed with error: %lu\n", GetLastError());
        CryptDestroyHash(hHash);
        return FALSE;
    }

    CryptDestroyHash(hHash);

    // CBC is the default; set it explicitly so the mode is obvious.
    DWORD mode = CRYPT_MODE_CBC;
    if (!CryptSetKeyParam(*hKey, KP_MODE, (BYTE*)&mode, 0))
    {
        printf("CryptSetKeyParam(KP_MODE) failed with error: %lu\n", GetLastError());
        CryptDestroyKey(*hKey);
        *hKey = 0;
        return FALSE;
    }

    return TRUE;
}

static void PrintHex(const char* label, const BYTE* data, DWORD length)
{
    printf("%s (%lu bytes): ", label, length);
    for (DWORD i = 0; i < length; i++)
    {
        printf("%02X ", data[i]);
    }
    printf("\n");
}

int main()
{
    HCRYPTPROV hProv = 0;
    HCRYPTKEY hKey = 0;

    const char* password = "MySecretKey";
    const char* plaintext = "Hello from AES-256!";

    BYTE buffer[256] = { 0 };
    DWORD dataLen = (DWORD)strlen(plaintext);
    memcpy(buffer, plaintext, dataLen);

    // PROV_RSA_AES provides CALG_AES_256. CRYPT_VERIFYCONTEXT is for
    // ephemeral keys that do not need to be persisted in a key container.
    if (!CryptAcquireContextA(&hProv, NULL, MS_ENH_RSA_AES_PROV, PROV_RSA_AES, CRYPT_VERIFYCONTEXT))
    {
        printf("CryptAcquireContextA failed with error: %lu\n", GetLastError());
        return 1;
    }

    if (!DeriveAes256Key(hProv, password, &hKey))
    {
        CryptReleaseContext(hProv, 0);
        return 1;
    }

    // CBC needs a random IV. Store it with the ciphertext; it is not secret.
    BYTE iv[AES_BLOCK_SIZE] = { 0 };
    if (!CryptGenRandom(hProv, AES_BLOCK_SIZE, iv))
    {
        printf("CryptGenRandom failed with error: %lu\n", GetLastError());
        CryptDestroyKey(hKey);
        CryptReleaseContext(hProv, 0);
        return 1;
    }

    if (!CryptSetKeyParam(hKey, KP_IV, iv, 0))
    {
        printf("CryptSetKeyParam(KP_IV) failed with error: %lu\n", GetLastError());
        CryptDestroyKey(hKey);
        CryptReleaseContext(hProv, 0);
        return 1;
    }

    printf("Plaintext: %s\n", plaintext);
    PrintHex("IV", iv, AES_BLOCK_SIZE);

    // AES-CBC pads to a 16-byte boundary, so the buffer must have room
    // for up to one extra block.
    DWORD encryptLen = dataLen;
    if (!CryptEncrypt(hKey, 0, TRUE, 0, buffer, &encryptLen, sizeof(buffer)))
    {
        printf("CryptEncrypt failed with error: %lu\n", GetLastError());
        CryptDestroyKey(hKey);
        CryptReleaseContext(hProv, 0);
        return 1;
    }

    PrintHex("Ciphertext", buffer, encryptLen);

    // Encrypting advances the CBC chain. Reset the same IV before decrypting.
    if (!CryptSetKeyParam(hKey, KP_IV, iv, 0))
    {
        printf("CryptSetKeyParam(KP_IV) reset failed with error: %lu\n", GetLastError());
        CryptDestroyKey(hKey);
        CryptReleaseContext(hProv, 0);
        return 1;
    }

    DWORD decryptLen = encryptLen;
    if (!CryptDecrypt(hKey, 0, TRUE, 0, buffer, &decryptLen))
    {
        printf("CryptDecrypt failed with error: %lu\n", GetLastError());
        CryptDestroyKey(hKey);
        CryptReleaseContext(hProv, 0);
        return 1;
    }

    buffer[decryptLen] = '\0';
    printf("Decrypted: %s\n", buffer);

    CryptDestroyKey(hKey);
    CryptReleaseContext(hProv, 0);
    return 0;
}
