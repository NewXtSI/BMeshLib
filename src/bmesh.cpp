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
    m_SSID = "";
    m_Password = "";
    m_IP = IPAddress(0, 0 , 0, 0);
}

bool
BMesh::init(String meshName, String meshPassword, uint16_t meshPort, uint8_t meshChannel) {
    bool bRet = true;
    m_meshName = m_meshName;
    m_meshPassword = meshPassword;
    m_meshPort = meshPort;
    m_meshChannel = meshChannel;
    __mesh.setDebugMsgTypes(ERROR | STARTUP | CONNECTION | GENERAL | APPLICATION | DEBUG | MSG_TYPES);  // set before init() so that you can see startup messages
    __mesh.init(m_meshName, m_meshPassword, m_meshPort, WIFI_AP_STA, m_meshChannel);
    __mesh.setContainsRoot(true);
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
