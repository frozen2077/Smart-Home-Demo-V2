void serialEventBt() {
  char t;
  bool inputBool = false;
  String temp_strdata  = "";
  String input_strdata = "";

  while (Serial.available() > 0) {                                                          // Serial read data byte by byte 
    t = char(Serial.read());
    if (t == 0xAA || t == 0xBB) inputBool = true;
    if (inputBool == true) temp_strdata += t;
    delay(1);
  }
  //memset(input_strdata, 0, strlen(temp_strdata));
  //memcpy(input_strdata, temp_strdata + 3, strlen(temp_strdata) - 3);

  if (temp_strdata.length() > 3) {
    for (uint ii = 3; ii < temp_strdata.length(); ii++) {
      input_strdata += temp_strdata[ii];
    }
    str_complete = true;
    stringProcess();
  }
}



void stringProcess() {

  if (str_complete && input_strdata.length() >= 1){
    if (strcmp(input_strdata, ir_command_01) == 0) {
        digitalWrite(15, 1);                                                       // Switch on buzzer (Wireless doorbell)
        client.publish(pub_ding_topic, "1", false);
        input_strdata = "";
        str_complete = false;
    }
    else if (strcmp(input_strdata, ir_command_00) == 0) {
        digitalWrite(15, 0);                                                       // Switch off buzzer (Wireless doorbell)
        client.publish(pub_ding_topic, "1", false);
        input_strdata = "";
        str_complete = false;
    }
    else if (strcmp(input_strdata, ir_command_11) == 0) {
        digitalWrite(14, 1);                                                       // Switch on power relay (Smart plug)      
        relay_en = true;
        input_strdata = "";
        str_complete = false;
    }
    else if (strcmp(input_strdata, ir_command_10) == 0) {
        digitalWrite(14, 0);                                                       // Switch off power relay (Smart plug)
        relay_en = false;
        input_strdata = "";
        str_complete = false;
    }
    else if (input_strdata.indexOf("t") != -1) {
        hazardgas = input_strdata.substring(1).toInt();
        input_strdata = "";
        str_complete = false;
    }
    else if (input_strdata.indexOf("l") != -1) {
        lux = input_strdata.substring(1).toInt();
        input_strdata = "";
        str_complete = false;
    }
    else if (input_strdata.indexOf("a") != -1) {
        String wm = input_strdata.substring(1);
        PubMQTT(pub_wm_topic, wm.c_str(), false);
        input_strdata = "";
        str_complete = false;
    }
    else {
      pubUnknownCmd(String("USART - Unknow serial data received: ") + String(input_strdata));
      input_strdata = "";
      str_complete = false;      
    }
  }
}