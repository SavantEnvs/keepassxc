// KeePass 1.x .kdb import (KeePass1Reader): header, key transform, group/entry field records,
// meta-streams and group-tree construction (upstream fixes 5d7f371b74, 6a534f38c6). Password
// "masterpw" matches the shipped seed tests/data/basic.kdb.
#include "fuzz_common.h"

#include <QBuffer>

#include "core/Database.h"
#include "format/KeePass1Reader.h"

FUZZ_INIT_FUNCTION

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    QByteArray bytes(reinterpret_cast<const char*>(data), static_cast<qsizetype>(size));
    QBuffer buf(&bytes);
    buf.open(QIODevice::ReadOnly);

    KeePass1Reader reader;
    auto db = reader.readDatabase(&buf, QStringLiteral("masterpw"), static_cast<QIODevice*>(nullptr));
    return 0;
}
