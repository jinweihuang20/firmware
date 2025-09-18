#include "AutoReplyModule.h"
#include "Telemetry/DeviceTelemetry.h"
#include "MeshService.h"
#include "configuration.h"
#include "NodeDB.h"
#include <string.h>
#include <string>
#include <cstdint>
ProcessMessage AutoReplyModule::handleReceived(const meshtastic_MeshPacket &mp)
{
    NodeNum nodenum = nodeDB->getNodeNum();
    if (mp.to != nodenum)
    {
        return ProcessMessage::CONTINUE;
    }
    // 解析 incoming message
    std::string incoming;
    if (mp.decoded.payload.size > 0)
    {
        incoming.assign((const char *)mp.decoded.payload.bytes, mp.decoded.payload.size);
    }

    // 避免迴圈：收到已標記 [auto] 的訊息不回
    if (incoming.find("[AB]") != std::string::npos)
    {
        return ProcessMessage::CONTINUE;
    }

    auto *reply = allocDataPacket();
    reply->to = mp.from; // 回給發訊人
    reply->decoded.portnum = mp.decoded.portnum;

    // 使用 std::string 方便拼接
    std::string replyMessage = "[AB]";

    if (mp.via_mqtt)
    {
        replyMessage += "[MQTT]";
    }
    else
    {
        replyMessage += "[Lora]";
    }

    if (incoming.find("@ab") != std::string::npos)
    {
        replyMessage += ":" + incoming;
    }
    else if (incoming.find("ping") != std::string::npos)
    {
        replyMessage += "pong?";
    }
    else if (incoming.find("@dm") != std::string::npos)
    {

        LOG_DEBUG("@dm command->Reply device metrics");
        meshtastic_NodeInfoLite *node = nodeDB->getMeshNode(nodeDB->getNodeNum());
        meshtastic_DeviceMetrics t = node->device_metrics;
        std::string deviceMetricsReplyText = "Device Metrics-> Voltage=" + std::to_string(t.voltage) + "V," +
                                             "BatLv=" + std::to_string(t.battery_level) + "%," +
                                             "Channel Util=" + std::to_string(t.channel_utilization) + "%," +
                                             "Air Util=" + std::to_string(t.air_util_tx) + "%," +
                                             "Uptime=" + std::to_string(t.uptime_seconds) + "seconds";
        LOG_DEBUG(deviceMetricsReplyText.c_str());
        replyMessage += deviceMetricsReplyText;
    }
    else
    {
        return ProcessMessage::CONTINUE;
    }

    // 限制長度，避免 payload 溢位
    size_t len = std::min(replyMessage.size(), sizeof(reply->decoded.payload.bytes));
    reply->decoded.payload.size = len;
    memcpy(reply->decoded.payload.bytes, replyMessage.c_str(), len);
    service->sendToMesh(reply); // 正式發送
    return ProcessMessage::CONTINUE;
}
