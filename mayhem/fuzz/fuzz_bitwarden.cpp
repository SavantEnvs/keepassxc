// Bitwarden JSON export import (BitwardenReader::convert): plain and password-protected exports
// (PBKDF2 / Argon2id + AES-CBC + HMAC), folders, items, URIs, custom fields, TOTP, passkeys.
// The export password is "a", matching tests/data/bitwarden_encrypted_*.json.
#include "fuzz_import_file.h"

#include "core/Database.h"
#include "format/BitwardenReader.h"

FUZZ_INIT_FUNCTION

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    with_input_file(data, size, ".json", [](const QString& path) {
        BitwardenReader reader;
        auto db = reader.convert(path, QStringLiteral("a"));
    });
    return 0;
}
