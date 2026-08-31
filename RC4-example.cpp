#include <windows.h>
#include <wincrypt.h>
#include <stdio.h>
#include <string.h>

#pragma comment(lib, "advapi32.lib")

// Derive an RC4 session key from a password using SHA-1.
// The hash object is destroyed before returning; the caller owns hKey.
static BOOL DeriveRc4Key(HCRYPTPROV hProv, const char* password, HCRYPTKEY* hKey)
{
    HCRYPTHASH hHash = 0;

    if (!CryptCreateHash(hProv, CALG_SHA1, 0, 0, &hHash))
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

    // CALG_RC4 is a stream cipher: ciphertext is the same length as plaintext.
    if (!CryptDeriveKey(hProv, CALG_RC4, hHash, 0, hKey))
    {
        printf("CryptDeriveKey failed with error: %lu\n", GetLastError());
        CryptDestroyHash(hHash);
        return FALSE;
    }

    CryptDestroyHash(hHash);
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
    const char* plaintext = "Hello from RC4!";

    BYTE buffer[256] = { 0 };
    DWORD dataLen = (DWORD)strlen(plaintext);
    memcpy(buffer, plaintext, dataLen);

    // PROV_RSA_FULL provides CALG_RC4. CRYPT_VERIFYCONTEXT is for
    // ephemeral keys that do not need to be persisted in a key container.
    if (!CryptAcquireContextA(&hProv, NULL, NULL, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT))
    {
        printf("CryptAcquireContextA failed with error: %lu\n", GetLastError());
        return 1;
    }

    if (!DeriveRc4Key(hProv, password, &hKey))
    {
        CryptReleaseContext(hProv, 0);
        return 1;
    }

    printf("Plaintext: %s\n", plaintext);

    DWORD encryptLen = dataLen;
    if (!CryptEncrypt(hKey, 0, TRUE, 0, buffer, &encryptLen, sizeof(buffer)))
    {
        printf("CryptEncrypt failed with error: %lu\n", GetLastError());
        CryptDestroyKey(hKey);
        CryptReleaseContext(hProv, 0);
        return 1;
    }

    PrintHex("Ciphertext", buffer, encryptLen);

    // RC4 advances the keystream as it encrypts, so the same key object
    // cannot decrypt. Destroy it and derive a fresh key from the password.
    CryptDestroyKey(hKey);
    hKey = 0;

    if (!DeriveRc4Key(hProv, password, &hKey))
    {
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
