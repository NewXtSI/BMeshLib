#include "../include/bmesh.h"

#include <painlessMesh.h>
#include <PubSubClient.h>

///////////////////////////////////////////////////////
// Node featurelist:
///////////////////////////////////////////////////////
// index    name                    implemented

painlessMesh  __mesh;


BMesh::BMesh() {
    m_meshName = "";
    m_meshPassword = "";
    m_meshPort = 0;
    m_meshChannel = 0;
    m_isGateway = false;
    m_gatewayNode = 0;
    m_SSID = "";
    m_Password = "";
    m_IP = IPAddress(0, 0 , 0, 0);
}

void
BMesh::receivedCallback(uint32_t from, String &msg) {
    StaticJsonDocument<256> doc;
    DeserializationError error = deserializeJson(doc, msg);
    bool bRestartNeeded = false;
    if (error) {
        Serial.print(F("deserializeJson() failed: "));
        Serial.println(error.f_str());
        Serial.println(msg);
        return;
    }
    Serial.println(msg);
    if (!doc.isNull()) {
        if (doc.is<JsonObject>()) {
            JsonObject obj = doc.as<JsonObject>();

            // Sollte nur bei Nodes ankommen, nicht beim Gateway
            if (obj.containsKey("gateway") && m_gatewayNode == 0) {
                m_gatewayNode = obj["gateway"].as<unsigned int>();
                sendHello();
                sendHeartbeat();
            }
        }
    }
}

void
BMesh::sendHello() {

}

void
BMesh::sendHeartbeat() {

}

bool
BMesh::init(String meshName, String meshPassword, uint16_t meshPort, uint8_t meshChannel) {
    bool bRet = true;
    m_meshName = meshName;
    m_meshPassword = meshPassword;
    m_meshPort = meshPort;
    m_meshChannel = meshChannel;
    __mesh.setDebugMsgTypes(ERROR | STARTUP | CONNECTION | GENERAL | APPLICATION | DEBUG | MSG_TYPES);  // set before init() so that you can see startup messages
    __mesh.init(m_meshName, m_meshPassword, m_meshPort, WIFI_AP_STA, m_meshChannel);
    __mesh.setContainsRoot(true);
    __mesh.onReceive([&](uint32_t from, String &msg) { receivedCallback(from, msg); });
    return bRet;
}

void
BMesh::setGatewayMode(String SSID, String Password) {
    m_isGateway = true;
    m_SSID = SSID;
    m_Password = Password;

    __mesh.stationManual(m_SSID, m_Password);
    __mesh.setRoot(true);
}

void
BMesh::loop() {
    __mesh.update();
    if (isGateway()) {
        if (m_IP != getlocalIP()) {
            m_IP = getlocalIP();
            if (m_IP == IPAddress(0,0,0,0)) {
                // Lost connection to station
                if (connectionCallBack)
                    connectionCallBack(false);
            } else {
                // Connection to Network established
                if (connectionCallBack)
                    connectionCallBack(true);
            }
        }
    }
}

bool
BMesh::isGateway() {
    return m_isGateway;
}

IPAddress
BMesh::getlocalIP() {
    return IPAddress(__mesh.getStationIP());
}

bool
BMesh::isMeshConnected() {
    bool bRet = false;
    return bRet;
}

bool
BMesh::isGatewayConnected() {
    bool bRet = false;
    return bRet;
}

void
BMesh::onAction(StringCallBack callBack) {
    this->actionCallBack = callBack;
}

void
BMesh::onConnection(BoolCallBack callBack) {
    this->connectionCallBack = callBack;
}
