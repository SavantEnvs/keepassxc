// Base32 codec (core/Base32) used for TOTP secrets (upstream fix 31a4c9ddd4: invalid padding in
// decode). Decodes raw, sanitized and re-padded input, and checks that encode->decode round-trips.
#include "fuzz_common.h"

#include "core/Base32.h"

FUZZ_INIT_FUNCTION

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    QByteArray bytes(reinterpret_cast<const char*>(data), static_cast<qsizetype>(size));
    (void)Base32::decode(bytes);
    (void)Base32::decode(Base32::sanitizeInput(bytes));
    (void)Base32::decode(Base32::addPadding(bytes));
    (void)Base32::removePadding(bytes);

    const QByteArray encoded = Base32::encode(bytes);
    const QByteArray decoded = Base32::decode(encoded);
    if (!bytes.isEmpty() && decoded != bytes) {
        // The codec contract: decode(encode(x)) == x.
        abort();
    }
    return 0;
}
