// Proton Pass JSON export import (ProtonPassReader::convert): vaults, items, extra fields, TOTP.
#include "fuzz_import_file.h"

#include "core/Database.h"
#include "format/ProtonPassReader.h"

FUZZ_INIT_FUNCTION

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    with_input_file(data, size, ".json", [](const QString& path) {
        ProtonPassReader reader;
        auto db = reader.convert(path);
    });
    return 0;
}
