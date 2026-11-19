// Database merge (core/Merger), as done by "Merge from database", `keepassxc-cli merge` and
// KeeShare imports: two KDBX XML documents separated by a NUL byte (never valid inside XML) are
// parsed with KdbxXmlReader, then the first is merged into the second in every merge mode;
// history items of the result are resolved like the entry-history view does (upstream 44be95cc1b).
#include "fuzz_common.h"

#include <QBuffer>

#include "core/Database.h"
#include "core/Entry.h"
#include "core/Group.h"
#include "core/Merger.h"
#include "format/KdbxXmlReader.h"
#include "format/KeePass2.h"

FUZZ_INIT_FUNCTION

static QSharedPointer<Database> readXml(QByteArray bytes)
{
    QBuffer buf(&bytes);
    buf.open(QIODevice::ReadOnly);
    KdbxXmlReader reader(KeePass2::FILE_VERSION_4);
    auto db = reader.readDatabase(&buf);
    if (reader.hasError() || !db || !db->rootGroup()) {
        return {};
    }
    return db;
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    QByteArray bytes(reinterpret_cast<const char*>(data), static_cast<qsizetype>(size));
    const qsizetype nul = bytes.indexOf('\0');
    // Without a separator the document is merged into a second, independent parse of itself.
    const QByteArray first = nul < 0 ? bytes : bytes.left(nul);
    const QByteArray second = nul < 0 ? bytes : bytes.mid(nul + 1);

    for (int mode : {-1, int(Group::KeepNewer), int(Group::Synchronize)}) {
        auto src = readXml(first);
        auto dst = readXml(second);
        if (!src || !dst) {
            return 0;
        }
        Merger merger(src.data(), dst.data());
        if (mode >= 0) {
            merger.setForcedMergeMode(static_cast<Group::MergeMode>(mode));
        }
        (void)merger.merge(true);
        (void)merger.merge(false);
        for (Entry* entry : dst->rootGroup()->entriesRecursive(true)) {
            for (Entry* h : entry->historyItems()) {
                (void)h->resolveMultiplePlaceholders(h->title());
                (void)h->resolveMultiplePlaceholders(h->password());
            }
        }
    }
    return 0;
}
