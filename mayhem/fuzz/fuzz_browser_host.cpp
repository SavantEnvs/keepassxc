// Browser-extension native-messaging stream (BrowserHost): the bytes keepassxc-proxy forwards from
// the browser over the local socket, which BrowserHost::readProxyMessage hands to parseSocketData
// (coalesced messages, fragments buffered per socket, garbage before/after JSON; upstream fix
// 80e68bb4e8). The input is split at 0xFF bytes (never valid in UTF-8 JSON) into successive socket
// reads on one connection, so fragment reassembly across reads is exercised. A fresh BrowserHost
// is used per input, so the fragment buffer never carries over between inputs.
//
// parseSocketData is private; upstream's own tests reach it as `friend class TestBrowser`, and so
// does this harness (the class below is that friend), feeding it exactly what the socket read gives.
#include "fuzz_common.h"

#include <QJsonDocument>

#include "browser/BrowserHost.h"
#include "browser/BrowserMessageBuilder.h"

FUZZ_INIT_FUNCTION

class TestBrowser
{
public:
    static void feed(const QByteArray& bytes)
    {
        BrowserHost host;
        for (const QByteArray& chunk : bytes.split('\xff')) {
            if (chunk.isEmpty()) {
                continue;
            }
            const auto messages = host.parseSocketData(chunk, 0);
            for (const auto& msg : messages) {
                (void)QJsonDocument(msg).toJson(QJsonDocument::Compact);
                // What BrowserAction does first with every message: read action/nonce/message fields
                // and base64-decode them.
                const QString nonce = msg.value("nonce").toString();
                const QString message = msg.value("message").toString();
                (void)browserMessageBuilder()->getArrayFromBase64(nonce);
                (void)browserMessageBuilder()->getArrayFromBase64(message);
                (void)browserMessageBuilder()->incrementNonce(nonce);
            }
        }
    }
};

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    TestBrowser::feed(QByteArray(reinterpret_cast<const char*>(data), static_cast<qsizetype>(size)));
    return 0;
}
