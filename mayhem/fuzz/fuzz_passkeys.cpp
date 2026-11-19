// Browser passkey (WebAuthn) requests: the decrypted `passkeys-register` / `passkeys-get` message
// that BrowserAction::handlePasskeysRegister / handlePasskeysGet receive from the browser
// extension. The input is that message's JSON, {"publicKey": {...}, "origin": "..."}. Like the
// handlers, the harness checks the origin with PasskeyUtils::isOriginAllowedWithLocalhost
// (allowLocalhostWithPasskeys = true, the user setting that admits the most origins: https://,
// file:// and localhost) and then passes publicKey + origin to
// BrowserPasskeysClient::getCredentialCreationOptions (rp/rp.id validation, effective domain and
// registrable-suffix checks, pubKeyCredParams, attestation, authenticatorSelection, extensions,
// clientDataJSON) and BrowserPasskeysClient::getAssertionOptions (rpId, extensions,
// userVerification), plus getAllowedCredentialsFromAssertionOptions for allowCredentials.
//
// Neither the client nor PasskeyUtils keeps state between calls, and no database, GUI or browser
// connection is involved. The prompts that follow in BrowserService need an open database and a
// user, so they are not reached.
#include "fuzz_common.h"

#include <QJsonDocument>
#include <QJsonObject>

#include "browser/BrowserPasskeysClient.h"
#include "browser/PasskeyUtils.h"

FUZZ_INIT_FUNCTION

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    const auto doc =
        QJsonDocument::fromJson(QByteArray::fromRawData(reinterpret_cast<const char*>(data), static_cast<qsizetype>(size)));
    const auto request = doc.object();
    const auto publicKey = request.value("publicKey").toObject();
    const auto origin = request.value("origin").toString();
    if (publicKey.isEmpty() || !passkeyUtils()->isOriginAllowedWithLocalhost(true, origin)) {
        return 0;
    }

    QJsonObject creation;
    browserPasskeysClient()->getCredentialCreationOptions(publicKey, origin, &creation);
    QJsonObject assertion;
    browserPasskeysClient()->getAssertionOptions(publicKey, origin, &assertion);
    passkeyUtils()->getAllowedCredentialsFromAssertionOptions(publicKey);
    return 0;
}
