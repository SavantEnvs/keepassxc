// Helper for the importers that take a path, not a device: write the input to a private
// scratch file under $TMPDIR (mkstemp), run the importer, delete the file.
#pragma once
#include "fuzz_common.h"

#include <unistd.h>

template <typename F> static void with_input_file(const uint8_t* data, size_t size, const char* suffix, F fn)
{
    QByteArray tmpl = (fuzz_tmpdir() + "/kpxc-fuzz-XXXXXX" + suffix).toLocal8Bit();
    int fd = mkstemps(tmpl.data(), static_cast<int>(strlen(suffix)));
    if (fd < 0) {
        abort();
    }
    size_t off = 0;
    while (off < size) {
        ssize_t n = write(fd, data + off, size - off);
        if (n <= 0) {
            abort();
        }
        off += static_cast<size_t>(n);
    }
    close(fd);
    fn(QString::fromLocal8Bit(tmpl));
    unlink(tmpl.constData());
}
