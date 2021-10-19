#include "main.hpp"
#include "timer.cpp"
#include "config.cpp"

/* forward-declaration */
void ConnectAndConfig(void);

/* Setup function */
void setup(void)
{
    Serial.begin(SERBAUD);
    Serial.println("Starting \"Homie-Aotoconfigurator\" V0.1");

    /* init holdtimer */
    Serial.println("[INFO] setting up Holdtimer");
    setup_HoldTimer(HOLD_TIMER_VALUE);

    /* init WiFi-Scan-Timer */
    Serial.println("[INFO] setting up WiFi-Scan-Timer");
    setup_ScanTimer(SCAN_TIMER_VALUE);
    enableScanTimer();

    /* parse json configs */
    parseBaseConfig();
    parseFriendlyNameConfig();

    /* setup WIFI */
    Serial.println("[INFO] setting up WIFI-Client");
    ClientHomieWiFi.mode(WIFI_STA);
    ClientHomieWiFi.disconnect();
    Serial.println("[INFO] setting up WIFI-Client done");
}

/* Mainloop */
void loop(void)
{
    /* check if Holdtimer-ISR occoured -> decrement holdcounter for each client-entry */
    if (HoldTimerEnd)
    {
        if (mapRecentlyClients.empty())
        {
            disableHoldTimer();
            Serial.println("SSID Map empty -> disable timer.");
        }
        else
        {
            /* iterate through the map and decrement the holdcounter
             * -> if holdcouter is zero, delete the entry */
            std::map<std::string, uint8_t>::iterator imapClients;
            std::list<std::map<std::string, uint8_t>::iterator> listToRemove;

            for (imapClients = mapRecentlyClients.begin(); imapClients != mapRecentlyClients.end(); imapClients++)
            {
                imapClients->second = (imapClients->second) - 1;
                Serial.printf("[DEBUG] processing %s, new value: %i\n", (imapClients->first).c_str(), imapClients->second);
                if (imapClients->second == 0) listToRemove.push_back(imapClients);
            }

            /* delete the entries */
            if (!listToRemove.empty())
            {
                std::list<std::map<std::string, uint8_t>::iterator>::iterator ilistRemove;
                for (ilistRemove = listToRemove.begin(); ilistRemove != listToRemove.end(); ilistRemove++)
                {
                    std::map<std::string, uint8_t>::iterator test = *ilistRemove;
                    Serial.printf("Removing %s from list\n",(test->first).c_str());
                    mapRecentlyClients.erase(*ilistRemove);
                }
            }
            else
            {
                Serial.println("[DEBUG] SSID remove-list empty");
            }
        }
        /* reset holdtimer-flag */
        HoldTimerEnd = false;
    }

    /* scan for Homie-Networks and configure them */
    if (ScanIntervallTimer)
    {
        Serial.println("[DEBUG] ScanIntervall reached");
        ConnectAndConfig();
        ScanIntervallTimer = false;
    }
}

/* scan for SSID, check for Homie-Networks and try
 * to configure the device */
void ConnectAndConfig(void)
{
    /* scan for Homie-Networks (= Homie configmode)
     * Homie-Networks aren't hidden and we want an async scan
     * -> use defaultparameters */
    int intNumNetworks;
    intNumNetworks = ClientHomieWiFi.scanNetworks();

    if (intNumNetworks > 0)
    {
        Serial.printf("[DEBUG] found %i SSID\n", intNumNetworks);
        bool bEnableTmr = false;

        /* go through the SSID-list */
        int intCtr;
        for (intCtr = 0; intCtr < intNumNetworks; ++intCtr)
        {
            /* ESP32: dummy-call / on other platforms maybe neccessary
             * Gives control to other process to avoid WTD-resets */
            yield();

            /* get SSID for the current network */
            std::string strSSID = ClientHomieWiFi.SSID(intCtr).c_str();
            /* check if PREFIX match the SSID */
            if (strSSID.rfind(SSIDPREFIX,0) == 0)
            {
                /* yes -> try to configure device */
                Serial.printf("[DEBUG] SSID: %s\n", strSSID.c_str());
                Serial.println("[DEBUG]   check if already in network map");

                std::map<std::string, uint8_t>::iterator imapInMap = mapRecentlyClients.find(strSSID);
                if (imapInMap != mapRecentlyClients.end())
                {
                    Serial.println("[DEBUG]   already in network map");
                }
                else
                {
                    Serial.println("[DEBUG]   try to connect");
                    ClientHomieWiFi.begin(strSSID.c_str());
                    uint8_t wait = 0;

                    while (ClientHomieWiFi.status() != WL_CONNECTED && wait < 200)
                    {
                        delay(1000);
                        wait++;
                        Serial.printf("    ... delaying ... %i\n",wait);
                    }

                    int intClWiFiStatus = ClientHomieWiFi.status();
                    if (intClWiFiStatus != WL_CONNECTED)
                    {
                        Serial.printf("[DEBUG]   failed to connect, code: %i\n", intClWiFiStatus);
                    }
                    else
                    {
                        /* create http-client object */
                        HTTPClient httpHomieClient;

                        /* variable for HTTP returncode */
                        int intHttpRetCode;

                        /* Homie Base-URL */
                        String astrBaseUrl = String(HOMIE_PROTO) + "://" + String(ClientHomieWiFi.gatewayIP().toString());   /* Arduino String-Object */

                        /* TODO: If desired, check Homie-Networklist again
                         * defined SSID and Encyrption and only configure the
                         * device, if the network is available */
                        #ifdef CONFIG_WITH_SSID_MATCH
                            httpHomieClient.begin(astrBaseUrl + "/" + HOMIE_NETWORK);

                            /* send GET-request */
                            intHttpRetCode = httpHomieClient.GET();
                            if (intHttpRetCode > 0)
                            {
                                String astrHomieRetPayload = httpHomieClient.getString();
                                /* TODO: here do the comparsation ... */
                            }
                            else
                            {
                                Serial.println("[DEBUG]   HTTP-Error (GET networks)");
                            }

                            /* end connection */                        
                            httpHomieClient.end();
                        #endif

                        /* generate Homie-Config */
                        std::string strHelper(SSIDPREFIX);
                        std::string strHelper1 = strSSID.substr(strHelper.length());
                        String astrCfgPayload = getHomieConfig(strHelper1);

                        if (astrCfgPayload != "")
                        {
                            httpHomieClient.begin(astrBaseUrl + "/" + HOMIE_CONFIG);
                            httpHomieClient.addHeader("Content-Type", "application/json");

                            intHttpRetCode = httpHomieClient.PUT(astrCfgPayload);
                            String blubb = httpHomieClient.getString();
                            Serial.println(blubb);

                            if (intHttpRetCode > 0)
                            {
                                if (intHttpRetCode == 200) { }
                                Serial.printf("Return Code: %i\n", intHttpRetCode);
                                /*  */
                            }
                            else
                            {
                                Serial.println("[DEBUG]   HTTP-Error (PUT config)");
                            }

                            /* end connection */                        
                            httpHomieClient.end();

                            /* not processed yet -> configure the device */
                            /* add network to map */
                            mapRecentlyClients.insert(std::pair<std::string, uint8_t>(strSSID, HOLDCTR));
                            Serial.println("[DEBUG]   added to network map");

                            /* enable holdtimer (only set flag) */
                            bEnableTmr = true;
                        }
                        else
                        {
                            /* Payload not acceptable... */
                            Serial.println("[DEBUG]   failed parse Homie-Payload - not configured");
                        }
                    }

                    ClientHomieWiFi.disconnect();
                }
            }
        }

        /* enable holdtimer, when neccessary */
        if (bEnableTmr && !timerAlarmEnabled(t_Hold_Timer)) enableHoldTimer();
    }
}
