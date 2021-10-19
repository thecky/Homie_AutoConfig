#include "main.hpp"

/* generate the JSON-config-string, which is transmitted
 * to the Homie-Client. In case that the provided informations
 * are invalid or mandantory keys are missing, an empty string
 * is returned */
String getHomieConfig(std::string &strClient)
{
    String astrRetValue;
    DynamicJsonDocument djsonHomieCfg(2048);

    /* get friendly-name */
    std::map<std::string, std::string>::iterator imapMAC = mapMAC2FriendlyName.find(strClient);
    if (imapMAC != mapMAC2FriendlyName.end())
    {
        djsonHomieCfg["name"] = imapMAC->second;
    }
    else
    {
        #ifdef CONFIG_WITHOUT_NAME
            djsonHomieCfg["name"] = "give-me-a-name";
        #else
            return "";
        #endif
    }

    /* check for mandantory values */
    if (s_baseCfg.astrWifiName == "--" || s_baseCfg.astrMQTTHost == "--") return "";

    /* WiFi infos */
    djsonHomieCfg["wifi"]["ssid"] = s_baseCfg.astrWifiName;
    djsonHomieCfg["wifi"]["password"] = s_baseCfg.astrWifiPass;
    /* MQTT-Host infos */
    djsonHomieCfg["mqtt"]["host"] = s_baseCfg.astrMQTTHost;
    djsonHomieCfg["mqtt"]["port"] = s_baseCfg.intMQTTPort;
    djsonHomieCfg["mqtt"]["auth"] = s_baseCfg.bMQTTAuth;

    if (s_baseCfg.bMQTTAuth && s_baseCfg.astrMQTTUsername != "--")
    {
        djsonHomieCfg["mqtt"]["username"] = s_baseCfg.astrMQTTUsername;
        djsonHomieCfg["mqtt"]["password"] = s_baseCfg.astrMQTTPassword;
    }

    /* OTA info */
    djsonHomieCfg["ota"]["enabled"] = s_baseCfg.bOTA;
    /* TODO: OTA more configurable */

    /* create JSON string and return it */
    serializeJson(djsonHomieCfg, astrRetValue);
    return astrRetValue;
}

/* parse base-configuration file */
void parseBaseConfig(void)
{
    s_baseCfg.bvalidCfg = false;

    if (!SPIFFS.begin())
    {
        Serial.println("[ERR] failed to initialize SPIFFS");
        return;
    }

    File baseCfgFile = SPIFFS.open(BASECFG_FILENAME, "r");

    /* skip bug, see https://github.com/espressif/arduino-esp32/issues/681 */
    if (baseCfgFile && !baseCfgFile.isDirectory() && baseCfgFile.size() > 0)
    {
        /* config found */
        DynamicJsonDocument djsonBaseCfg(2048);
        DeserializationError jsonErr = deserializeJson(djsonBaseCfg, baseCfgFile);

        if (!jsonErr)
        {
            /* could parse json */
            s_baseCfg.astrWifiName = djsonBaseCfg["wifi"]["ssid"] | "--";
            s_baseCfg.astrWifiPass = djsonBaseCfg["wifi"]["password"] | "";
            s_baseCfg.astrMQTTHost = djsonBaseCfg["mqtt"]["host"] | "--";
            s_baseCfg.intMQTTPort = djsonBaseCfg["mqtt"]["port"] | 1883;
            s_baseCfg.bMQTTAuth = djsonBaseCfg["mqtt"]["auth"] | false;
            s_baseCfg.astrMQTTUsername = djsonBaseCfg["mqtt"]["username"] | "invalid_MQTT_User_Config";
            s_baseCfg.astrMQTTPassword = djsonBaseCfg["mqtt"]["password"] | "";
            s_baseCfg.bOTA = djsonBaseCfg["ota"]["enabled"] | false;

            s_baseCfg.bvalidCfg = true;
        }
        else
        {
            Serial.println("[ERR] failed to parse base-config file");
        }
    }
    else
    {
        /* config not found */
        Serial.println("[ERR] failed to load base-config file");
    }
}

/* parse base-configuration */
void parseFriendlyNameConfig(void)
{
    if (!SPIFFS.begin())
    {
        Serial.println("[ERR] failed to initialize SPIFFS");
        
    }

    File nameCfgFile = SPIFFS.open(NAMECFG_FILENAME, "r");

    /* skip bug, see https://github.com/espressif/arduino-esp32/issues/681 */
    if (nameCfgFile && !nameCfgFile.isDirectory() && nameCfgFile.size() > 0)
    {
        /* config found */
        DynamicJsonDocument djsonNameCfg(2048);
        DeserializationError jsonErr = deserializeJson(djsonNameCfg, nameCfgFile);

        if (!jsonErr)
        {
            /* could parse json */
            JsonObject nameObject = djsonNameCfg.as<JsonObject>();
            std::string strVal;
            std::string strKey;

            for (JsonObject::iterator itJsonNameCfg=nameObject.begin(); itJsonNameCfg != nameObject.end(); ++itJsonNameCfg)
            {
                strKey = itJsonNameCfg->key().c_str();
                strVal = itJsonNameCfg->value().as<std::string>();
                mapMAC2FriendlyName.insert(std::pair<std::string, std::string>(strKey, strVal));
            }
        }
        else
        {
            Serial.println("[ERR] failed to parse base-config file");
        }
    }
    else
    {
        /* config not found */
        Serial.println("[ERR] failed to load name-config file");
    }
}