// Shared one-time setup for the KeePassXC libFuzzer harnesses (mayhem/fuzz).
//
// State kept across inputs, on purpose:
//  * one QCoreApplication (Qt requires exactly one per process; it holds no per-input data),
//  * Crypto::init() (Botan backend self-test; read-only afterwards),
//  * the Config singleton, pointed at a scratch ini under $TMPDIR so the user's real
//    ~/.config/keepassxc is never read. The harnesses never write to it.
// Everything a harness parses (Database, readers, parsers) is created per input and destroyed
// before LLVMFuzzerTestOneInput returns.
#pragma once

#include <QByteArray>
#include <QCoreApplication>
#include <QDir>
#include <QString>
#include <cstdint>
#include <cstdlib>

#include "core/Config.h"
#include "crypto/Crypto.h"

static QCoreApplication* g_app = nullptr;

static QString fuzz_tmpdir()
{
    const char* t = getenv("TMPDIR");
    return QString::fromLocal8Bit((t && *t) ? t : "/tmp");
}

static void fuzz_init(int* argc, char*** argv)
{
    qputenv("QT_QPA_PLATFORM", "offscreen");
    // Silence qWarning()/qDebug() floods from parsers on malformed input.
    qputenv("QT_LOGGING_RULES", "*=false");
    g_app = new QCoreApplication(*argc, *argv);
    QCoreApplication::setApplicationName("keepassxc-fuzz");
    Config::createConfigFromFile(fuzz_tmpdir() + "/keepassxc-fuzz-" + QString::number(getpid()) + ".ini");
    if (!Crypto::init()) {
        abort();
    }
}

#define FUZZ_INIT_FUNCTION                                                                                             \
    extern "C" int LLVMFuzzerInitialize(int* argc, char*** argv)                                                      \
    {                                                                                                                  \
        fuzz_init(argc, argv);                                                                                         \
        return 0;                                                                                                      \
    }
