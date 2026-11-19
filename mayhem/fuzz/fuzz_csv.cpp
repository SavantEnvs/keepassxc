// CSV import tokenizer (CsvParser), as used by the CSV import wizard and `keepassxc-cli import`.
// Parsed with the default settings and with the alternative syntax (';' separator, '\'' qualifier,
// backslash escapes, '#' comments), then the table is walked.
#include "fuzz_common.h"

#include <QBuffer>

#include "format/CsvParser.h"

FUZZ_INIT_FUNCTION

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    QByteArray bytes(reinterpret_cast<const char*>(data), static_cast<qsizetype>(size));
    for (int variant = 0; variant < 2; ++variant) {
        QBuffer buf(&bytes);
        buf.open(QIODevice::ReadOnly);
        CsvParser parser;
        if (variant == 1) {
            parser.setFieldSeparator(';');
            parser.setTextQualifier('\'');
            parser.setBackslashSyntax(true);
            parser.setComment('#');
        }
        if (parser.parse(&buf)) {
            const auto table = parser.getCsvTable();
            qsizetype total = 0;
            for (const auto& row : table) {
                for (const auto& cell : row) {
                    total += cell.size();
                }
            }
            (void)total;
            (void)parser.getCsvRows();
            (void)parser.getCsvCols();
        }
    }
    return 0;
}
