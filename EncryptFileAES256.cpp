// WARNING: This is an educational file encryption utility. Use it only on files
// you own or have explicit permission to encrypt. Do not use this code for
// ransomware, unauthorized access, or any malicious purpose. The author and this
// project assume no liability for misuse.
//
// Build: cl.exe EncryptFileAES256.cpp /EHsc
//        x86_64-w64-mingw32-g++ EncryptFileAES256.cpp -o EncryptFileAES256.exe -lbcrypt
//
// Usage:
//   EncryptFileAES256.exe encrypt plaintext.txt ciphertext.blah mypassword
//   EncryptFileAES256.exe decrypt ciphertext.blah plaintext.txt mypassword

#include <windows.h>
#include <bcrypt.h>
#include <stdio.h>
#include <vector>
#include <string>
#include <iostream>
#include <fstream>

#pragma comment(lib, "bcrypt.lib")

constexpr ULONG AES_KEY_SIZE = 32;
constexpr ULONG AES_BLOCK_SIZE = 16;
constexpr ULONG SALT_SIZE = 16;
constexpr ULONG IV_SIZE = 16;
constexpr ULONG PBKDF2_ITERATIONS = 100000;
constexpr ULONG TAG_SIZE = 16;

struct CryptContext
{
    BCRYPT_ALG_HANDLE hAlg = NULL;
    BCRYPT_KEY_HANDLE hKey = NULL;
};

BOOL GenerateRandomBytes(PUCHAR pbBuffer, ULONG cbBuffer)
{
    return BCryptGenRandom(NULL, pbBuffer, cbBuffer, BCRYPT_USE_SYSTEM_PREFERRED_RNG);
}

BOOL DeriveKeyFromPassword(const std::string& password,
                           const std::vector<BYTE>& salt,
                           std::vector<BYTE>& key)
{
    key.resize(AES_KEY_SIZE);

    NTSTATUS status = BCryptDeriveKeyPBKDF2(
        NULL,
        (PUCHAR)password.data(),
        (ULONG)password.length(),
        (PUCHAR)salt.data(),
        (ULONG)salt.size(),
        PBKDF2_ITERATIONS,
        key.data(),
        AES_KEY_SIZE,
        0);

    return BCRYPT_SUCCESS(status);
}

BOOL CreateAesGcmKey(const std::vector<BYTE>& key, CryptContext& ctx)
{
    NTSTATUS status = BCryptOpenAlgorithmProvider(&ctx.hAlg, BCRYPT_AES_ALGORITHM, NULL, 0);
    if (!BCRYPT_SUCCESS(status))
        return FALSE;

    status = BCryptSetProperty(ctx.hAlg, BCRYPT_CHAINING_MODE,
                               (PUCHAR)BCRYPT_CHAIN_MODE_GCM,
                               sizeof(BCRYPT_CHAIN_MODE_GCM),
                               0);
    if (!BCRYPT_SUCCESS(status))
        return FALSE;

    status = BCryptGenerateSymmetricKey(ctx.hAlg, &ctx.hKey, NULL, 0,
                                        (PUCHAR)key.data(),
                                        (ULONG)key.size(),
                                        0);
    return BCRYPT_SUCCESS(status);
}

void DestroyCryptContext(CryptContext& ctx)
{
    if (ctx.hKey)
    {
        BCryptDestroyKey(ctx.hKey);
        ctx.hKey = NULL;
    }
    if (ctx.hAlg)
    {
        BCryptCloseAlgorithmProvider(ctx.hAlg, 0);
        ctx.hAlg = NULL;
    }
}

BOOL EncryptFileAes256(const std::string& inputPath,
                       const std::string& outputPath,
                       const std::string& password)
{
    std::ifstream inFile(inputPath, std::ios::binary);
    if (!inFile)
    {
        std::cerr << "Failed to open input file: " << inputPath << "\n";
        return FALSE;
    }

    std::vector<BYTE> plaintext((std::istreambuf_iterator<char>(inFile)),
                               std::istreambuf_iterator<char>());
    inFile.close();

    std::vector<BYTE> salt(SALT_SIZE);
    std::vector<BYTE> iv(IV_SIZE);
    if (!GenerateRandomBytes(salt.data(), SALT_SIZE) ||
        !GenerateRandomBytes(iv.data(), IV_SIZE))
    {
        std::cerr << "Failed to generate random bytes\n";
        return FALSE;
    }

    std::vector<BYTE> key;
    if (!DeriveKeyFromPassword(password, salt, key))
    {
        std::cerr << "Failed to derive key\n";
        return FALSE;
    }

    CryptContext ctx;
    if (!CreateAesGcmKey(key, ctx))
    {
        std::cerr << "Failed to create AES key\n";
        return FALSE;
    }

    BCRYPT_AUTHENTICATED_CIPHER_MODE_INFO authInfo;
    BCRYPT_INIT_AUTH_MODE_INFO(authInfo);
    authInfo.pbNonce = iv.data();
    authInfo.cbNonce = IV_SIZE;
    authInfo.pbTag = (PUCHAR)alloca(TAG_SIZE);
    authInfo.cbTag = TAG_SIZE;

    std::vector<BYTE> ciphertext(plaintext.size());
    ULONG cbCipher = 0;
    NTSTATUS status = BCryptEncrypt(ctx.hKey,
                                    plaintext.data(),
                                    (ULONG)plaintext.size(),
                                    &authInfo,
                                    NULL,
                                    0,
                                    ciphertext.data(),
                                    (ULONG)ciphertext.size(),
                                    &cbCipher,
                                    0);
    if (!BCRYPT_SUCCESS(status))
    {
        std::cerr << "BCryptEncrypt failed: 0x" << std::hex << status << "\n";
        DestroyCryptContext(ctx);
        return FALSE;
    }

    std::ofstream outFile(outputPath, std::ios::binary);
    if (!outFile)
    {
        std::cerr << "Failed to open output file: " << outputPath << "\n";
        DestroyCryptContext(ctx);
        return FALSE;
    }

    outFile.write("BLAH", 4);
    outFile.write((const char*)salt.data(), salt.size());
    outFile.write((const char*)iv.data(), iv.size());
    outFile.write((const char*)authInfo.pbTag, TAG_SIZE);
    outFile.write((const char*)ciphertext.data(), cbCipher);
    outFile.close();

    DestroyCryptContext(ctx);

    std::cout << "Encrypted " << inputPath << " -> " << outputPath << "\n";
    return TRUE;
}

BOOL DecryptFileAes256(const std::string& inputPath,
                       const std::string& outputPath,
                       const std::string& password)
{
    std::ifstream inFile(inputPath, std::ios::binary);
    if (!inFile)
    {
        std::cerr << "Failed to open input file: " << inputPath << "\n";
        return FALSE;
    }

    std::vector<BYTE> fileData((std::istreambuf_iterator<char>(inFile)),
                              std::istreambuf_iterator<char>());
    inFile.close();

    if (fileData.size() < 4 + SALT_SIZE + IV_SIZE + TAG_SIZE)
    {
        std::cerr << "Input file is too small to be valid\n";
        return FALSE;
    }

    if (memcmp(fileData.data(), "BLAH", 4) != 0)
    {
        std::cerr << "Input file does not have the expected BLAH header\n";
        return FALSE;
    }

    size_t offset = 4;
    std::vector<BYTE> salt(fileData.begin() + offset, fileData.begin() + offset + SALT_SIZE);
    offset += SALT_SIZE;

    std::vector<BYTE> iv(fileData.begin() + offset, fileData.begin() + offset + IV_SIZE);
    offset += IV_SIZE;

    std::vector<BYTE> tag(fileData.begin() + offset, fileData.begin() + offset + TAG_SIZE);
    offset += TAG_SIZE;

    std::vector<BYTE> ciphertext(fileData.begin() + offset, fileData.end());

    std::vector<BYTE> key;
    if (!DeriveKeyFromPassword(password, salt, key))
    {
        std::cerr << "Failed to derive key\n";
        return FALSE;
    }

    CryptContext ctx;
    if (!CreateAesGcmKey(key, ctx))
    {
        std::cerr << "Failed to create AES key\n";
        return FALSE;
    }

    BCRYPT_AUTHENTICATED_CIPHER_MODE_INFO authInfo;
    BCRYPT_INIT_AUTH_MODE_INFO(authInfo);
    authInfo.pbNonce = iv.data();
    authInfo.cbNonce = IV_SIZE;
    authInfo.pbTag = tag.data();
    authInfo.cbTag = TAG_SIZE;

    std::vector<BYTE> plaintext(ciphertext.size());
    ULONG cbPlain = 0;
    NTSTATUS status = BCryptDecrypt(ctx.hKey,
                                    ciphertext.data(),
                                    (ULONG)ciphertext.size(),
                                    &authInfo,
                                    NULL,
                                    0,
                                    plaintext.data(),
                                    (ULONG)plaintext.size(),
                                    &cbPlain,
                                    0);
    if (!BCRYPT_SUCCESS(status))
    {
        std::cerr << "BCryptDecrypt failed (wrong password or corrupted file): 0x"
                  << std::hex << status << "\n";
        DestroyCryptContext(ctx);
        return FALSE;
    }

    std::ofstream outFile(outputPath, std::ios::binary);
    if (!outFile)
    {
        std::cerr << "Failed to open output file: " << outputPath << "\n";
        DestroyCryptContext(ctx);
        return FALSE;
    }

    outFile.write((const char*)plaintext.data(), cbPlain);
    outFile.close();

    DestroyCryptContext(ctx);

    std::cout << "Decrypted " << inputPath << " -> " << outputPath << "\n";
    return TRUE;
}

int main(int argc, char* argv[])
{
    if (argc != 5)
    {
        std::cerr << "Usage: " << argv[0] << " <encrypt|decrypt> <input> <output> <password>\n";
        return 1;
    }

    std::string mode = argv[1];
    std::string inputPath = argv[2];
    std::string outputPath = argv[3];
    std::string password = argv[4];

    BOOL result = FALSE;
    if (mode == "encrypt")
    {
        result = EncryptFileAes256(inputPath, outputPath, password);
    }
    else if (mode == "decrypt")
    {
        result = DecryptFileAes256(inputPath, outputPath, password);
    }
    else
    {
        std::cerr << "Invalid mode. Use 'encrypt' or 'decrypt'.\n";
        return 1;
    }

    return result ? 0 : 1;
}
