// KDBX 3.1 / 4.x database container (KeePass2Reader): signature + header fields, KDF parameters
// (variant map), cipher selection, HMAC/hashed block streams, decompression, KDBX4 inner header,
// then the XML payload. The key is the fixed password "secret" (the password of the shipped
// seeds utils/fuzz-testing/empty{3,4}.kdbx), so inputs derived from the seeds pass key checks.
#include "fuzz_common.h"

#include <QBuffer>

#include "core/Database.h"
#include "format/KeePass2Reader.h"
#include "keys/CompositeKey.h"
#include "keys/PasswordKey.h"

FUZZ_INIT_FUNCTION

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    QByteArray bytes(reinterpret_cast<const char*>(data), static_cast<qsizetype>(size));
    QBuffer buf(&bytes);
    buf.open(QIODevice::ReadOnly);

    auto key = QSharedPointer<CompositeKey>::create();
    key->addKey(QSharedPointer<PasswordKey>::create(QStringLiteral("secret")));

    Database db;
    KeePass2Reader reader;
    reader.readDatabase(&buf, key, &db);
    return 0;
}
