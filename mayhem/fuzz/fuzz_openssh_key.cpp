// SSH private keys stored in database entries (sshagent OpenSSHKey): PEM extraction, OpenSSH
// "openssh-key-v1" container, bcrypt-pbkdf + cipher decryption, PKCS#1 RSA/DSA DER (ASN1Key),
// then fingerprint / public key / private key re-serialization. The passphrase
// "correctpassphrase" matches the encrypted keys in tests/TestOpenSSHKey.cpp (the seeds).
#include "fuzz_common.h"

#include "sshagent/OpenSSHKey.h"

FUZZ_INIT_FUNCTION

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    QByteArray bytes(reinterpret_cast<const char*>(data), static_cast<qsizetype>(size));
    OpenSSHKey key;
    if (!key.parsePKCS1PEM(bytes)) {
        return 0;
    }
    (void)key.encrypted();
    (void)key.cipherName();
    if (key.openKey(QStringLiteral("correctpassphrase"))) {
        (void)key.type();
        (void)key.fingerprint();
        (void)key.fingerprint(QCryptographicHash::Md5);
        (void)key.comment();
        (void)key.publicKey();
        (void)key.privateKey();
    }
    return 0;
}
