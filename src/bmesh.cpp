#ifdef ESP8266
#include <ESP8266WiFi.h>
#endif
#include "../include/bmesh.h"

#include <LittleFS.h>
#include <painlessMesh.h>

String getFilenamefromMD5(String md5);
String getFileMD5(String filename);

size_t                      fileSendCallback(painlessmesh::plugin::ota::DataRequest pkg, char* buffer);
#define OTA_PART_SIZE           512 //How many bytes to send per OTA data packet

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
    m_nodeVersion = "unknown";
    m_scheduler = new Scheduler();
    m_SSID = "";
    m_Password = "";
    m_IP = IPAddress(0, 0 , 0, 0);
}

void
BMesh::receivedCallback(uint32_t from, const String &msg) {
    bool bHandledInternal = false;
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
            if (obj.containsKey("action")) {
                String sAction = obj["action"].as<String>();
                Serial.printf("Performing action %s...\n", sAction.c_str());
                if (sAction == "OTA") {
                    Serial.printf("Enabling OTA receive...\n");
                    __mesh.initOTAReceive("basenode");
                }
                if (sAction == "restart") {
                    Serial.printf("Restarting in 5 seconds...\n");
                    std::shared_ptr<Task> restartTask  = __mesh.addTask(TASK_SECOND * 5, 1, [&]() {
#ifdef ESP8266
                        ESP.reset();
#else
                        ESP.restart();
#endif
                    });
                    restartTask->enableDelayed();
                }

            }
            // {"enablefeature": 1}
            if (obj.containsKey("enablefeature")) {
                Serial.printf("callback for enablefeature\n");
                if (enableFeatureCallBack)
                    enableFeatureCallBack(obj["enablefeature"].as<unsigned int>());
            }
            if (obj.containsKey("disablefeature")) {
                Serial.printf("callback for disablefeature\n");
                if (disableFeatureCallBack)
                    disableFeatureCallBack(obj["disablefeature"].as<unsigned int>());
            }
            if (obj.containsKey("configfeature")) {
                Serial.printf("callback for configfeature\n");
                Serial.printf("configfeature has to be parsed\n");
                if (configFeatureCallBack)
                    configFeatureCallBack(obj["configfeature"]["feature"].as<unsigned int>(),
                        obj["configfeature"]["config"].as<String>());
//                if (enableFeatureCallBack)
//                    enableFeatureCallBack(obj["disablefeature"].as<unsigned int>());
            }
            if (obj.containsKey("gateway")) {
                if (m_gatewayNode == 0) {
                    m_gatewayNode = obj["gateway"].as<unsigned int>();
                    m_gatewayCheckTask->enableDelayed(15 * TASK_SECOND);
                    m_sendValuesTask->enable();
                    sendHello();
                    sendHeartbeat();
                } else {
                    m_gatewayCheckTask->restartDelayed(15 * TASK_SECOND);
                }
                bHandledInternal = true;
            }
        }
    }
    if (!bHandledInternal)
        if (dataReceivedCallBack)
            dataReceivedCallBack(msg);
}

void
BMesh::sendHello() {
    if (!isGateway()) {
        StaticJsonDocument<256> doc;
        char buffer[256];
        JsonObject obj = doc.createNestedObject("statechange");
        obj["node"] =  __mesh.getNodeId();
        obj["action"] = "new";
        serializeJson(doc, buffer);
        __mesh.sendSingle(m_gatewayNode, buffer);
    }
}

void
BMesh::sendHeartbeat() {
    if (isGateway()) {
        String payload("{\"gateway\":");
        payload += __mesh.getNodeId();
        payload += "}";
        __mesh.sendBroadcast(payload);
    } else {
        StaticJsonDocument<512> doc;
        char buffer[512];
        JsonObject contents = doc.createNestedObject("heartbeat");
        contents["heap"] = ESP.getFreeHeap();
        contents["node"] = __mesh.getNodeId();
        contents["version"] = m_nodeVersion;
        contents["featureset"] = m_featureSet;
#ifdef ESP8266
        contents["model"] = "ESP8266";
#else
        contents["model"] = ESP.getChipModel();
#endif
#ifdef ESP8266
        contents["uptime"] = millis() / 1000;
#else
        contents["uptime"] = esp_timer_get_time() / 1000000;
#endif
        serializeJson(doc, buffer);
        __mesh.sendSingle(m_gatewayNode, buffer);
    }
    if (heartbeatCallBack)
        heartbeatCallBack();
}

void
BMesh::changedConnectionsCallback() {
    Serial.printf("%s\n", __mesh.subConnectionJson().c_str());
    this->checkDeadnodes();
}

bool
BMesh::init(String meshName, String meshPassword, uint16_t meshPort, uint8_t meshChannel) {
    bool bRet = true;
    m_meshName = meshName;
    m_meshPassword = meshPassword;
    m_meshPort = meshPort;
    m_meshChannel = meshChannel;
      // set before init() so that you can see startup messages
    __mesh.setDebugMsgTypes(ERROR | STARTUP | CONNECTION | GENERAL | APPLICATION | DEBUG | MSG_TYPES);
    __mesh.init(m_meshName, m_meshPassword, m_scheduler, m_meshPort, WIFI_AP_STA, m_meshChannel);
    __mesh.setContainsRoot(true);
    __mesh.onReceive([&](uint32_t from, String &msg) { receivedCallback(from, msg); });
    __mesh.onChangedConnections([&]() { changedConnectionsCallback(); });
    m_heartbeatTask = __mesh.addTask(TASK_SECOND * 10, TASK_FOREVER, [&]() { this->sendHeartbeat();});
    m_gatewayCheckTask  = __mesh.addTask(TASK_SECOND * 15, TASK_FOREVER, [&]() { this->checkGateway();});
    m_sendValuesTask  = __mesh.addTask(TASK_SECOND * 20, TASK_FOREVER, [&]() { this->sendValues();});
    m_checkDeadnodeTask = __mesh.addTask(TASK_SECOND * 30, TASK_FOREVER, [&]() { this->checkDeadnodes();});
    m_heartbeatTask->enable();
    m_checkDeadnodeTask->disable();
    return bRet;
}

void
BMesh::checkGateway() {
    if (isGateway()) {
        m_sendValuesTask->disable();
        return;
    }
    if (m_gatewayNode == 0) {
        m_sendValuesTask->disable();
        m_gatewayCheckTask->disable();
    }
}

void
BMesh::sendValues() {
    Serial.printf("Send values task...\n");
    if (sendValuesCallBack)
        sendValuesCallBack();
}

void
BMesh::sendRaw(String sData) {
    __mesh.sendSingle(m_gatewayNode, sData);
}

void
BMesh::sendMultipleValues(std::vector<valueset_t> valueSet) {
    if (m_gatewayNode > 0) {
        StaticJsonDocument<512> doc;
        char buffer[512];
        JsonObject contents = doc.createNestedObject("data");
        contents["node"] = __mesh.getNodeId();
        JsonArray values = contents.createNestedArray("values");

        for (auto it=valueSet.begin(); it != valueSet.end(); ++it) {
            JsonObject val = values.createNestedObject();
    //        val = uiValue;
            val[(*it).key] = (*it).value;
        }
        serializeJson(doc, buffer);
        __mesh.sendSingle(m_gatewayNode, buffer);
    }
}

void
BMesh::sendSingleValue(valueset_t valueSet) {
    std::vector<valueset_t> valueSetVector;
    valueSetVector.push_back(valueSet);
    sendMultipleValues(valueSetVector);
}

void
BMesh::sendSingleValue(String key, String value) {
    valueset_t  vSet;
    vSet.key = key;
    vSet.value = value;
    sendSingleValue(vSet);
}

void
BMesh::setGatewayMode(String SSID, String Password) {
    m_isGateway = true;
    m_SSID = SSID;
    m_Password = Password;

    __mesh.stationManual(m_SSID, m_Password);
    __mesh.setRoot(true);
    m_checkDeadnodeTask->enable();
}

void
BMesh::loop() {
    __mesh.update();
    if (isGateway()) {
        if (m_IP != getlocalIP()) {
            m_IP = getlocalIP();
            if (m_IP == IPAddress(0, 0, 0, 0)) {
                // Lost connection to station
                if (connectionCallBack)
                    connectionCallBack(false);
                __mesh.stationManual(m_SSID, m_Password);
                __mesh.setRoot(true);
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
    if (!isGateway()) {
        if (m_gatewayNode != 0)
            bRet = true;
    }
    return bRet;
}

bool
BMesh::isGatewayConnected() {
    bool bRet = false;
    return bRet;
}

void                        
BMesh::checkDeadnodes() {
    std::list<uint32_t> actualNodes = __mesh.getNodeList();
    std::vector<uint32_t> newNodes;
    std::vector<uint32_t> deletedNodes;
    
    for (auto itAct = actualNodes.begin(); itAct != actualNodes.end(); ++itAct) {
        if (std::find(m_connectedNodes.begin(), m_connectedNodes.end(), (*itAct)) != m_connectedNodes.end()) {
            // Knoten in Liste, alles gut.
        } else {
            // Neuer Knoten!
            newNodes.push_back(*itAct);
        }
    }
    for (auto itAct = m_connectedNodes.begin(); itAct != m_connectedNodes.end(); ++itAct) {
        if (std::find(actualNodes.begin(), actualNodes.end(), (*itAct)) != actualNodes.end()) {
            // Knoten noch da, alles gut
        } else {
            deletedNodes.push_back(*itAct);
        }
    }
    if (newNodes.size() > 0)
        Serial.printf("Neue Knoten im Netz:\n");
    for (auto itAct = newNodes.begin(); itAct != newNodes.end(); ++itAct) {
        Serial.printf(" - %u\n", (*itAct));
        m_connectedNodes.push_back(*itAct);
        if (nodeAddedCallBack)
            nodeAddedCallBack(*itAct);
    }
    if (deletedNodes.size() > 0)
        Serial.printf("Nicht mehr erreichbare Knoten im Netz:\n");
    for (auto itAct = deletedNodes.begin(); itAct != deletedNodes.end(); ++itAct) {
        Serial.printf(" - %u\n", (*itAct));
        for (auto itRem = m_connectedNodes.begin(); itRem != m_connectedNodes.end(); ) {
            if ((*itRem) == (*itAct)) {
                itRem = m_connectedNodes.erase(itRem);
            } else {
                ++itRem;
            }
            if (nodeDeletedCallBack)
                nodeDeletedCallBack(*itAct);
        }
    }
}

extern size_t getFileSize(String filename);


void
BMesh::offerOTA(String filename, String hardware) {
    static bool bOffered = false;
    if (!bOffered) {
        __mesh.offerOTA("basenode", "ESP32", getFileMD5("/fw_esp32.bin"),
            ceil(((float)getFileSize("/fw_esp32.bin")) / OTA_PART_SIZE), false);
        __mesh.offerOTA("basenode", "ESP8266", getFileMD5("/fw_8266.bin"),
            ceil(((float)getFileSize("/fw_8266.bin")) / OTA_PART_SIZE), false);

        __mesh.initOTASend(fileSendCallback, OTA_PART_SIZE);
    }
}

void
BMesh::setVersion(String sVersion) {
    m_nodeVersion = sVersion;
}

void
BMesh::enableNodeFeature(uint32_t uiNodeID, uint32_t uiFeature, bool bEnable) {
    char msg[128];
    sprintf(msg, "{\"%s\": %u}", bEnable ? "enablefeature" : "disablefeature", uiFeature);
    __mesh.sendSingle(uiNodeID, msg);
}

void
BMesh::configNodeFeature(uint32_t uiNodeID, uint32_t uiFeature, String sConfig) {
    char msg[255];
    sprintf(msg, "{\"%s\": {\"feature\": %u, \"config\": %s}}", "configfeature", uiFeature, sConfig.c_str());
    __mesh.sendSingle(uiNodeID, msg);
}

void
BMesh::sendNodeAction(uint32_t uiNodeID, String sAction) {
    char msg[128];
    sprintf(msg, "{\"action\": \"%s\"}", sAction.c_str());
    __mesh.sendSingle(uiNodeID, msg);

}

void
BMesh::onHeartbeat(VoidCallBack callBack) {
    this->heartbeatCallBack = callBack;
}

void
BMesh::onAction(StringCallBack callBack) {
    this->actionCallBack = callBack;
}

void
BMesh::onConnection(BoolCallBack callBack) {
    this->connectionCallBack = callBack;
}

void
BMesh::onSendValues(VoidCallBack callBack) {
    this->sendValuesCallBack = callBack;
}

void
BMesh::onNodeAdded(ULongCallBack callBack) {
    this->nodeAddedCallBack = callBack;
}

void
BMesh::onNodeDeleted(ULongCallBack callBack) {
    this->nodeDeletedCallBack = callBack;
}

void
BMesh::onDataReceived(StringCallBack callBack) {
    this->dataReceivedCallBack = callBack;
}

void
BMesh::onEnableFeature(ULongCallBack callBack) {
    this->enableFeatureCallBack = callBack;
}

void
BMesh::onDisableFeature(ULongCallBack callBack) {
    this->disableFeatureCallBack = callBack;
}

void
BMesh::onConfigFeature(ULongStringCallBack callBack) {
    this->configFeatureCallBack = callBack;
}

String getFilenamefromMD5(String md5);
String getFileMD5(String filename);

String getFileMD5(String filename) {
    String sRet;
    File f = LittleFS.open(filename, "r");
    MD5Builder md5;
    md5.begin();
    md5.addStream(f, f.size());
    md5.calculate();
    sRet = md5.toString();
    f.close();
    return sRet;
}

String md58266 = "";
String md5esp32 = "";

String getFilenamefromMD5(String md5) {
    String sRet;
    if (md58266 == "")
        md58266 = getFileMD5("/fw_8266.bin");
    if (md5esp32 == "")        
        md5esp32 = getFileMD5("/fw_esp32.bin");
//    downloadFile("10.0.20.100", "/firmware/firmware_8266.bin", "/fw_8266.bin");
//    downloadFile("10.0.20.100", "/firmware/firmware_esp32.bin", "/fw_esp32.bin");
    if (md5esp32 == md5)
        sRet = "/fw_esp32.bin";
    if (md58266 == md5)
        sRet = "/fw_8266.bin";
    return sRet;
}

size_t 
fileSendCallback(painlessmesh::plugin::ota::DataRequest pkg,
                       char* buffer) {
    File f;
//    char filename[512];
    size_t sizeRet = 0;
    char debugmsg[512];

    sprintf(debugmsg, "OTA request from %s(%d/%d) node %u [%s]", pkg.md5.c_str(), pkg.partNo, pkg.noPart, pkg.from, getFilenamefromMD5(pkg.md5).c_str());

//    mqttClient.publish("iotmesh/gateway/debug", debugmsg);

    Serial.printf("OTA request from %s(%d/%d) node %u", pkg.md5.c_str(), pkg.partNo, pkg.noPart, pkg.from);

    if (getFilenamefromMD5(pkg.md5).length() == 0) {
        Serial.printf("File for requested MD5 not found.");
        return 0;
    }

    f = LittleFS.open("/"+getFilenamefromMD5(pkg.md5), "r");
    //fill the buffer with the requested data packet from the node.
    f.seek(OTA_PART_SIZE * pkg.partNo);
    f.readBytes(buffer, OTA_PART_SIZE);

        //The buffer can hold OTA_PART_SIZE bytes, but the last packet may
        //not be that long. Return the actual size of the packet.
    sizeRet =  min((unsigned)OTA_PART_SIZE,
        f.size() - (OTA_PART_SIZE * pkg.partNo));
    Serial.printf(" sent %d byte\n", sizeRet);
    f.close();
    return sizeRet;
}

String
BMesh::getFeatureName(uint32_t uiFeature) {
    switch (uiFeature) {
        case FEATURE_LOCATE:                return "Locate";
        case FEATURE_BUTTON:                return "Button";
        case FEATURE_BLETRACK:              return "BLE track";
        case FEATURE_MIYASCAN:              return "MiyaScan";
    }
    return "unknown";
}

