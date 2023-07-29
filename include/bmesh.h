#ifndef __BMESH_H__
#define __BMESH_H__

#include <Arduino.h>

class BMesh {
 protected:
    using       BoolCallBack = void (*)(bool);
    using       StringCallBack = void (*)(String);

    StringCallBack  actionCallBack = nullptr;
 public:
                BMesh();
    bool        init(String meshName, String meshPassword, uint16_t meshPort, uint8_t meshChannel);
    void        setGatewayMode(String SSID, String Password);
    void        loop();

    bool        isGateway();
    IPAddress   getlocalIP();           // Bei Gateway, STATION IP

    bool        isMeshConnected();      // Minimum 1 node can be reached
    bool        isGatewayConnected();   // For nodes: Gateway is set and can be reached

    void        onAction(StringCallBack callBack);
 private:
    String      m_meshName;
    String      m_meshPassword;
    uint32_t    m_meshPort;
    uint8_t     m_meshChannel;
    IPAddress   m_IP;
    bool        m_isGateway;
    String      m_SSID;
    String      m_Password;
};

#endif // __BMESH_H__