// KDBX XML parser (KdbxXmlReader): the decrypted payload of every .kdbx, and the format
// `keepassxc-cli import` reads. Parses in non-strict and strict mode, then walks every entry
// (and every history item, which has no group: upstream 44be95cc1b) resolving placeholders and
// references, and re-serializes the database with KdbxXmlWriter.
#include "fuzz_common.h"

#include <QBuffer>

#include "core/Database.h"
#include "core/Entry.h"
#include "core/Group.h"
#include "format/KdbxXmlReader.h"
#include "format/KdbxXmlWriter.h"
#include "format/KeePass2.h"

FUZZ_INIT_FUNCTION

static void walk(const QSharedPointer<Database>& db)
{
    if (!db || !db->rootGroup()) {
        return;
    }
    for (Entry* entry : db->rootGroup()->entriesRecursive(true)) {
        QList<Entry*> all = entry->historyItems();
        all.prepend(entry);
        for (Entry* e : all) {
            (void)e->resolveMultiplePlaceholders(e->title());
            (void)e->resolveMultiplePlaceholders(e->username());
            (void)e->resolveMultiplePlaceholders(e->password());
            (void)e->resolveMultiplePlaceholders(e->url());
            (void)e->resolveMultiplePlaceholders(e->notes());
            (void)e->hasReferences();
            (void)e->hasTotp();
        }
    }
    QByteArray out;
    QBuffer outBuf(&out);
    outBuf.open(QIODevice::WriteOnly);
    // The one-argument constructor asserts version < 4 (KdbxXmlWriter.cpp); KDBX 4 needs a
    // binary index map, empty here as in tests/TestKdbx4.cpp.
    KdbxXmlWriter writer(KeePass2::FILE_VERSION_4, {});
    writer.writeDatabase(&outBuf, db.data());
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    QByteArray bytes(reinterpret_cast<const char*>(data), static_cast<qsizetype>(size));
    for (bool strict : {false, true}) {
        QBuffer buf(&bytes);
        buf.open(QIODevice::ReadOnly);
        KdbxXmlReader reader(KeePass2::FILE_VERSION_4);
        reader.setStrictMode(strict);
        auto db = reader.readDatabase(&buf);
        if (!reader.hasError()) {
            walk(db);
        }
    }
    return 0;
}
