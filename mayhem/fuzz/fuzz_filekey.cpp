// Key files (FileKey::load): XML v1/v2 key files (with hash check), 32-byte binary, 64-char hex,
// and the hashed fallback for any other file.
#include "fuzz_common.h"

#include <QBuffer>

#include "keys/FileKey.h"

FUZZ_INIT_FUNCTION

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    QByteArray bytes(reinterpret_cast<const char*>(data), static_cast<qsizetype>(size));
    QBuffer buf(&bytes);
    buf.open(QIODevice::ReadOnly);
    FileKey key;
    QString err;
    if (key.load(&buf, &err)) {
        (void)key.rawKey();
        (void)key.type();
    }
    return 0;
}
