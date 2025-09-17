#pragma once
#include "SinglePortModule.h"


class AutoReplyModule : public SinglePortModule {
public:
    AutoReplyModule() : SinglePortModule("auto_reply",meshtastic_PortNum_TEXT_MESSAGE_APP) {}

    virtual ProcessMessage handleReceived(const meshtastic_MeshPacket &mp) override;
};
