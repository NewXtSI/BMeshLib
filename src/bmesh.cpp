#include "../include/bmesh.h"

///////////////////////////////////////////////////////
// Node featurelist:
///////////////////////////////////////////////////////
// index    name                    implemented

BMesh::BMesh() {

}

bool
BMesh::init(String meshName, String meshPassword, uint16_t meshPort) {
    bool bRet = true;
    m_meshName = m_meshName;
    m_meshPassword = meshPassword;
    m_meshPort = meshPort;

    m_isGateway = false;
    m_SSID = "";
    m_Password = "";
    m_MQTTServer = "";
    m_MQTTPort = 0;
    m_MQTTUser = "";
    m_MQTTPassword = "";
    return bRet;
}

void
BMesh::setGatewayMode(String SSID, String Password, String MQTTServer,
        uint32_t MQTTPort, String MQTTUser, String MQTTPassword) {
    m_isGateway = true;
}

void
BMesh::loop() {

}

bool
BMesh::isGateway() {
    return m_isGateway;
}

bool
BMesh::isMeshConnected() {
    bool bRet = false;
    return bRet;
}

bool
BMesh::isGatewayConnected()
 {
    bool bRet = false;
    return bRet;
}