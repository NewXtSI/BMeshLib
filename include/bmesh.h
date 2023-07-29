#ifndef __BMESH_H__
#define __BMESH_H__

#include <Arduino.h>

class BMesh {
 public:
                BMesh();
    bool        init(String meshName, String meshPassword, uint16_t meshPort);
    void        setGatewayMode(String SSID, String Password, String MQTTServer, uint32_t MQTTPort, String MQTTUser, String MQTTPassword);
    void        loop();

    bool        isGateway();

    bool        isMeshConnected();      // Minimum 1 node can be reached
    bool        isGatewayConnected();   // For nodes: Gateway is set and can be reached
 private:
    String      m_meshName;
    String      m_meshPassword;
    uint32_t    m_meshPort;

    bool        m_isGateway;
    String      m_SSID;
    String      m_Password;
    String      m_MQTTServer;
    uint32_t    m_MQTTPort;
    String      m_MQTTUser;
    String      m_MQTTPassword;
};

#endif // __BMESH_H__