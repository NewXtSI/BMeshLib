#ifndef __BMESH_H__
#define __BMESH_H__

#include <Arduino.h>
#include <memory>
#include <vector>

class Scheduler;
class Task;

typedef struct {
    String key;
    String value;
} valueset_t;

class BMesh {
 protected:
    using       BoolCallBack = void (*)(bool);
    using       StringCallBack = void (*)(String);
    using       ULongStringCallBack = void (*)(uint32_t, String);
    using       VoidCallBack = void (*)();
    using       ULongCallBack = void (*)(uint32_t);

    StringCallBack               dataReceivedCallBack = nullptr;
    StringCallBack               actionCallBack = nullptr;
    VoidCallBack                 heartbeatCallBack = nullptr;
    VoidCallBack                 sendValuesCallBack = nullptr;
    ULongCallBack                nodeDeletedCallBack = nullptr;
    ULongCallBack                nodeAddedCallBack = nullptr;

    ULongCallBack                enableFeatureCallBack = nullptr;
    ULongCallBack                disableFeatureCallBack = nullptr;
    ULongStringCallBack          configFeatureCallBack = nullptr;

    BoolCallBack                 connectionCallBack = nullptr;           // Connection to "world" established
 public:
                                BMesh();
    void                        setVersion(String sVersion);                                
    bool                        init(String meshName, String meshPassword, uint16_t meshPort, uint8_t meshChannel);
    void                        setGatewayMode(String SSID, String Password);
    void                        loop();
    uint32_t                    getId();
    bool                        isGateway();
    IPAddress                   getlocalIP();           // Bei Gateway, STATION IP

    bool                        isMeshConnected();      // Minimum 1 node can be reached
    bool                        isGatewayConnected();   // For nodes: Gateway is set and can be reached

    void                        onHeartbeat(VoidCallBack callBack);
    void                        onSendValues(VoidCallBack callBack);
    void                        onAction(StringCallBack callBack);
    void                        onDataReceived(StringCallBack callBack);
    void                        onConnection(BoolCallBack callBack);
    void                        onNodeAdded(ULongCallBack callBack);
    void                        onNodeDeleted(ULongCallBack callBack);

    void                        onEnableFeature(ULongCallBack callBack);
    void                        onDisableFeature(ULongCallBack callBack);
    void                        onConfigFeature(ULongStringCallBack callBack);
    
    String                      getFeatureName(uint32_t uiFeature);

    void                        sendNodeAction(uint32_t uiNodeID, String sAction);
    void                        enableNodeFeature(uint32_t uiNodeID, uint32_t uiFeature, bool bEnable);
    void                        configNodeFeature(uint32_t uiNodeID, uint32_t uiFeature, String sConfig);
    void                        sendMultipleValues(std::vector<valueset_t> valueSet);
    void                        sendSingleValue(valueset_t valueSet);
    void                        sendSingleValue(String key, String value);
    void                        sendRaw(String sData);
    void                        setFeatureSet(uint32_t fSet) { m_featureSet = fSet; }
    void                        offerOTA(String filename, String hardware);

    Scheduler*                  getScheduler() { return m_scheduler; }
 private:
    std::shared_ptr<Task>       m_heartbeatTask;
    std::shared_ptr<Task>       m_gatewayCheckTask;
    std::shared_ptr<Task>       m_sendValuesTask;
    std::shared_ptr<Task>       m_checkDeadnodeTask;
    uint32_t                    m_gatewayNode;

    uint32_t                    m_featureSet;

    String                      m_nodeVersion;
    String                      m_meshName;
    String                      m_meshPassword;
    uint32_t                    m_meshPort;
    uint8_t                     m_meshChannel;
    IPAddress                   m_IP;
    bool                        m_isGateway;
    String                      m_SSID;
    String                      m_Password;

    void                        receivedCallback(uint32_t from, const String &msg);
    void                        changedConnectionsCallback();
    void                        sendHello();
    void                        sendHeartbeat();
    void                        checkGateway();
    void                        sendValues();
    void                        checkDeadnodes();

    Scheduler*                  m_scheduler;

    std::vector<uint32_t>       m_connectedNodes;
};

#endif // __BMESH_H__