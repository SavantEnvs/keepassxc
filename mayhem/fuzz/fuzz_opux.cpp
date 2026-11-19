// 1Password .1pux export import (OPUXReader::convert): export.data JSON (accounts, vaults, items,
// sections, fields, OTP, dates, addresses) plus the attachment and vault-icon lookups under files/.
//
// A .1pux file is a zip archive. The zip layer is minizip (a Debian library, not KeePassXC code),
// and almost every byte mutation of a deflated archive fails its CRC or inflate before any importer
// code runs. So the fuzz input is not a zip; the harness packs it into one with STORED
// (uncompressed) entries and hands the importer that file:
//
//   <export.data> [ '\0' <name> '\0' <content> ]*
//
// The first NUL-separated segment becomes export.data. Each later (name, content) pair becomes
// the entry "files/<name>", which is where OPUXReader looks for attachments
// ("files/<documentId>__<fileName>") and vault avatars ("files/<avatar>"). Every input packs into a
// valid zip, so no input is rejected.
#include "fuzz_common.h"

#include "core/Database.h"
#include "format/OPUXReader.h"

#include <cstring>
#include <unistd.h>
#include <vector>

#include <zip.h>

FUZZ_INIT_FUNCTION

static void add_entry(zipFile zf, const QByteArray& name, const uint8_t* data, size_t size)
{
    zip_fileinfo info;
    memset(&info, 0, sizeof(info));
    // method 0 = stored, level 0, no zip64 (inputs are far below 4 GiB).
    if (zipOpenNewFileInZip64(zf, name.constData(), &info, nullptr, 0, nullptr, 0, nullptr, 0, 0, 0) != ZIP_OK) {
        abort();
    }
    if (size && zipWriteInFileInZip(zf, data, static_cast<unsigned>(size)) != ZIP_OK) {
        abort();
    }
    if (zipCloseFileInZip(zf) != ZIP_OK) {
        abort();
    }
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    // Split the input at NUL bytes.
    std::vector<std::pair<const uint8_t*, size_t>> seg;
    size_t start = 0;
    for (size_t i = 0; i <= size; ++i) {
        if (i == size || data[i] == 0) {
            seg.emplace_back(data + start, i - start);
            start = i + 1;
        }
    }

    QByteArray path = (fuzz_tmpdir() + "/kpxc-fuzz-XXXXXX.1pux").toLocal8Bit();
    int fd = mkstemps(path.data(), 5);
    if (fd < 0) {
        abort();
    }
    close(fd);

    zipFile zf = zipOpen64(path.constData(), APPEND_STATUS_CREATE);
    if (!zf) {
        abort();
    }
    add_entry(zf, "export.data", seg[0].first, seg[0].second);
    for (size_t i = 1; i + 1 < seg.size(); i += 2) {
        QByteArray name =
            "files/" + QByteArray(reinterpret_cast<const char*>(seg[i].first), static_cast<qsizetype>(seg[i].second));
        add_entry(zf, name, seg[i + 1].first, seg[i + 1].second);
    }
    if (zipClose(zf, nullptr) != ZIP_OK) {
        abort();
    }

    {
        OPUXReader reader;
        auto db = reader.convert(QString::fromLocal8Bit(path));
    }
    unlink(path.constData());
    return 0;
}
