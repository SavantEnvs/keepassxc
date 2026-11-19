// TOTP settings strings stored in entries (Totp::parseSettings): otpauth:// URLs, legacy
// "TOTP Settings" + seed attributes, KeeOTP strings, Steam encoder; the secret goes through
// Base32. Valid settings are then used to generate a code and written back. The input is
// "<settings>\0<legacy seed>" (the seed part is optional).
#include "fuzz_common.h"

#include "core/Totp.h"

FUZZ_INIT_FUNCTION

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    QByteArray bytes(reinterpret_cast<const char*>(data), static_cast<qsizetype>(size));
    const qsizetype nul = bytes.indexOf('\0');
    const QString raw = QString::fromUtf8(nul < 0 ? bytes : bytes.left(nul));
    const QString seed = nul < 0 ? QString() : QString::fromUtf8(bytes.mid(nul + 1));

    auto settings = Totp::parseSettings(raw, seed);
    if (!settings) {
        return 0;
    }
    if (Totp::checkValidSettings(settings).isEmpty()) {
        bool valid = false;
        (void)Totp::generateTotp(settings, &valid, 1234567890ull);
    }
    (void)Totp::writeSettings(settings, QStringLiteral("title"), QStringLiteral("user"), false);
    (void)Totp::writeSettings(settings, QStringLiteral("title"), QStringLiteral("user"), true);
    (void)Totp::hasCustomSettings(settings);
    return 0;
}
