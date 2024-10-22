



String mqtt_topic_pub(String topic) {
  String ret_topic = mqtt_prefix;
  if (ret_topic.length() > 0 && !ret_topic.endsWith ("/")) ret_topic += "/";
  return ret_topic + topic;
}

String mqtt_topic_sub(String topic) {
  String ret_topic = mqtt_prefix;
  String sub_prefix = SUB_PREFIX;
  if (ret_topic.length() > 0 && !ret_topic.endsWith ("/")) ret_topic += "/";
  if (sub_prefix.length() > 0 && !ret_topic.endsWith ("/")) sub_prefix += "/";
  return ret_topic + sub_prefix + topic;
}

void mqttMonitor() {

  if (WiFi.status() != WL_CONNECTED) return;

  if (!mqtt.connected() && millis () < nextMqttConnectTime) return;

  if (!mqtt_topic_subscribed) {
    mqtt_topic_subscribed = subscribeMqttTopic();
  }

  if (mqtt_topic_subscribed) {
    processOutQueue();
  }

  // If the connection with the MQTT server is not established, reconnect to the server
  if (!mqtt.connected() && (mqtt_conn_last == 0 || (millis() - mqtt_conn_last > 2500))) {
    if (!mqtt_connecting) {
      DEBUG (F("\n MQTT server connecting.'"));
      DEBUG (mqtt_server);
      DEBUG (":");
      DEBUG (mqtt_port);
      DEBUG (F("'; ClientID -> '"));
      DEBUG (mqtt_client_name);
      DEBUG (F("' ... "));
    }
    mqtt_topic_subscribed = false ;
    mqtt_conn_last = millis();

    String topic = mqtt_topic_pub(PUB_TOPIC_WIL);
    uint32_t t = millis();

    mqtt_connected = mqtt.connect(
                      mqtt_client_id.c_str(),                   // Unique id to register the client with the MQTT server, does not allow duplicate!
                      mqtt_user,                                // Mqtt username
                      mqtt_pass, 
                      willTopic,                                // Topic for last-will message, sent when occasional unintended break happens due to loss of connection or depleted batteries. 
                      willQoS,                                  // QoS for the last-will message
                      willRetain)                               // Retain: True, message will remain persistent for new subscriber of the topic
    if (mqtt_connected) {
      mqtt_connecting = false ;     
      DEBUGLN (F("\n MQTT server connected."));
      if (outQueueLength > 0) {
        DEBUG (F("Messages in the send queue:"));  
        DEBUGLN (outQueueLength);  
      }
      PubMQTT(topic., "online", true); 
    } else {      
      DEBUG (".");
      mqtt_connecting = true;
      mqtt_conn_cnt++;
      if (mqtt_conn_cnt == 50) {
        mqtt_conn_cnt = 0;
      }
    }

    if (millis() - t > MQTT_CONNECT_TIMEOUT) {
      nextMqttConnectTime = millis() + MQTT_RECONNECT_PERIOD;
      DEBUGLN(F("\n MQTT server connection failed.'"));
      DEBUGLN(mqtt.state());
      DEBUGLN("Attempt again in 1 minutes");      
    }

  }
}

bool subscribeMqttTopic() {
  bool ok = false ;
  String sub_topic_wildcard = mqtt_topic_sub("#");
  if (mqtt.connected()) {
    DEBUG (F("Subscribe to topic='cmd' >> "));
    ok = mqtt.subscribe(sub_topic_wildcard.c_str());
    if (ok)  DEBUGLN (F("OK"));
    else     DEBUGLN (F("FAIL"));
  }
  return ok;
}


void putOutQueue(String topic, String message, bool retain) {
  bool ok = false ;
  ok = mqtt.beginPublish(topic.c_str(), message.length(), retain) ;
  if (ok) {
    // If the send initiation was successful - fill the send buffer with the transmitted message string
    mqtt.print(message.c_str());
    // Finish sending. If the packet was sent - 1 is returned, if the sending failed - 0 is returned
    ok = mqtt.endPublish() == 1 ;
    if (ok) {
      // Send successful
      DEBUG (F("MQTT >> OK >> "));
      DEBUG (topic);
      DEBUG (F("\t >> "));
      DEBUGLN (message);
    }
  }
  // If the sending did not occur and there is room in the queue, we put the message in the send queue
  if (!ok && outQueueLength < QSIZE_OUT) {
    outQueueLength++;
    tpcQueue[outQueueWriteIdx] = topic;
    msgQueue[outQueueWriteIdx] = message;
    rtnQueue[outQueueWriteIdx] = retain;
    outQueueWriteIdx++;
    if (outQueueWriteIdx >= QSIZE_OUT) outQueueWriteIdx = 0 ;
  }
}

void PubMQTT(String topic, String &message, bool retain = false) {
  putOutQueue(mqtt_topic_pub(topic), message, retain);
}

// Send messages from the queue to the server
void processOutQueue() {

  if (mqtt.connected() && outQueueLength > 0) {    

    // Topic and content of the sent message
    String topic   = tpcQueue[outQueueReadIdx];
    String message = outQueue[outQueueReadIdx];
    bool   retain  = rtnQueue[outQueueReadIdx];

    // Trying to send. If send initialization fails, false is returned; If successful - true
    bool ok = mqtt.beginPublish(topic.c_str(), message.length(), retain) ;
    if (ok) {
      // If the send initiation was successful - fill the send buffer with the transmitted message string
      mqtt.print(message.c_str());
      // Finish sending. If the packet was sent - 1 is returned, if the sending failed - 0 is returned
      ok = mqtt.endPublish() == 1 ;
    }
    if (ok) {      
      // Send successful
      DEBUG (F("MQTT >> OK >> "));
      DEBUG (topic);
      DEBUG ( F ("\t >> "));
      DEBUGLN (message);
      // Clear the message from the queue
      tpcQueue[outQueueReadIdx] = "" ;
      outQueue[outQueueReadIdx] = "" ;
      rtnQueue[outQueueReadIdx] = "" ;
      outQueueReadIdx++;
      if (outQueueReadIdx >= QSIZE_OUT) outQueueReadIdx = 0 ;
      outQueueLength--;
    } else {
      // Send failed
      DEBUG (F("MQTT >> FAIL >> "));
      DEBUG (topic);
      DEBUG (F("\t >> "));
      DEBUGLN (message);
    }
  }  
}

// Send error message to the app 
void pubUnknownCmd(char *text) {
  DynamicJsonDocument doc(256);
  String out;
  doc["message"] = F("Unknown command");
  doc["text"] = String(text) + String(".");
  serializeJson(doc, out);      
  PubMQTT(out, TOPIC_ERR);
}