// KeeAgent.settings XML (KeeAgentSettings::fromXml): the SSH-agent settings attachment stored in
// (possibly shared) database entries; parsed, then written back.
#include "fuzz_common.h"

#include "sshagent/KeeAgentSettings.h"

FUZZ_INIT_FUNCTION

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    QByteArray bytes(reinterpret_cast<const char*>(data), static_cast<qsizetype>(size));
    KeeAgentSettings settings;
    if (settings.fromXml(bytes)) {
        (void)settings.toXml();
        (void)settings.fileName();
        (void)settings.attachmentName();
    }
    return 0;
}
