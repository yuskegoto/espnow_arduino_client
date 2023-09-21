#include <Arduino.h>
#include "M5Atom.h"
#include <esp_now.h>
#include <Wire.h>
#include <WiFi.h>

#include "def.h"

struct EspnowMessage
{
    uint8_t header;
    uint8_t myNum;
    uint8_t body[ESPNOW_MSG_LENGTH];
} lastMessage;

uint8_t mac[6];
uint8_t devNo;
QueueHandle_t messageQueue;
u32_t lastMsgTs;
u8_t lastmac[6];
bool espnowSendFail;
uint8_t sendRetry;
bool run = false;

u8_t messageRecvBuffer[sizeof(EspnowMessage)];
EspnowMessage outgoingMessage = {0};
const uint8_t BRIDGE_ADDRESS[] = BRIDGE_MAC_ADDRESS;
esp_now_peer_info_t peerInfo;

bool dataReceived = false;

// Prototype
void onDataRecv(const uint8_t *mac, const uint8_t *incomingData, int len);
void onDataSent(const uint8_t *mac_addr, esp_now_send_status_t status);

/*************************** Code ********************************/
esp_err_t sendEspnowMessage(EspnowMessage * msg, u8_t length)
{
    Serial.printf("Send ESPNOW Packet: Header:%X Num:%X Len:%d Data: ", msg->header, msg->myNum, length);
    for (u8_t i = 0; i < length; i++)
    {
        Serial.printf("%X ", msg->body[i]);
    }
    Serial.printf("\n");

    // Send message via ESP-NOW
    esp_err_t result = esp_now_send(BRIDGE_ADDRESS, (uint8_t *)msg, length);

    // Store latest outgoing message
    memcpy(&outgoingMessage, msg, sizeof(EspnowMessage));

    return result;
}

/**
 * @brief Change Machine state according to message
 *
 */
void readIncomingMessage()
{
    if (xQueueReceive(messageQueue, &messageRecvBuffer, (TickType_t)0))
    {
        u8_t msgSize = 0;

        Serial.printf("Got action message\n");

        EspnowMessage msg = {0};

        memcpy(&msg, &messageRecvBuffer, sizeof(EspnowMessage));

        switch (msg.header)
        {
        case ESPNOW_HEADER_RUN:
            Serial.printf("Run!\n");
            run = true;
            M5.dis.drawpix(0, CRGB(0, 0, 255));
            break;

        case ESPNOW_HEADER_RESET:
            Serial.printf("Resetting... Bye!\n");
            Serial.flush();
            ESP.restart();
            break;

        case ESPNOW_HEADER_MAC_QUERY:
            msg.header = ESPNOW_HEADER_MAC;
            msg.myNum = devNo;
            msgSize = 6;
            memcpy(msg.body, mac, 6);
            sendEspnowMessage(&msg, msgSize + 2);
            break;

        case ESPNOW_HEADER_STATUS_QUERY:
            msg.header = ESPNOW_HEADER_STATUS;
            msg.myNum = devNo;
            msgSize = 1;
            msg.body[0] = (u8_t)random(256);
            sendEspnowMessage(&msg, msgSize + 2);
            break;

        default:
            break;
        }
    }
}

esp_err_t resendMsg(const uint8_t *addr)
{
    esp_err_t result = esp_now_send(addr, (uint8_t *)&outgoingMessage, 3);
    return result;
}

esp_err_t sendBootMessage()
{
    memset(&outgoingMessage, 0, sizeof(EspnowMessage));
    outgoingMessage.header = ESPNOW_HEADER_BOOT;
    outgoingMessage.myNum = devNo;
    esp_err_t result = esp_now_send(BRIDGE_ADDRESS, (uint8_t *)&outgoingMessage, 2);

    return result;
}

esp_err_t sendStatusMessage()
{
    EspnowMessage msg = {0};
    msg.header = ESPNOW_HEADER_STATUS;
    msg.myNum = devNo;
    msg.body[0] = (u8_t)random(256);
    return sendEspnowMessage(&msg, 3);
}

void initComm()
{
    WiFi.mode(WIFI_MODE_STA);
    // Init ESP-NOW
    if (esp_now_init() != ESP_OK)
    {
        Serial.printf("Error initializing ESP-NOW\n");
        return;
    }
    WiFi.macAddress(mac);

    // Once ESPNow is successfully Init, register CB to
    // get the status of Trasnmitted packet
    esp_now_register_send_cb(onDataSent);
    esp_now_register_recv_cb(onDataRecv);

    // Register peer
    memcpy(peerInfo.peer_addr, BRIDGE_ADDRESS, 6);
    peerInfo.channel = 0;
    peerInfo.encrypt = false;

    // Add peer
    if (esp_now_add_peer(&peerInfo) != ESP_OK)
    {
        Serial.printf("Failed to add peer\n");
        return;
    }
}

//////////////////////////////////// ESP-NOW Send Callbacks ////////////////////////////////////////////

// callback function that will be executed when data is received
void onDataRecv(const uint8_t *mac, const uint8_t *incomingData, int len)
{
    Serial.printf("\nSender MAC: ");
    for (uint8_t i = 0; i < 6; i++)
    {
        Serial.printf("%02X ", mac[i]);
    }
    Serial.printf("Bytes: %d\nMessage: ", len);
    for (uint8_t i = 0; i < len; i++)
    {
        Serial.printf("%02X ", incomingData[i]);
    }
    Serial.printf("\n");

    // Detecting my message
    if ((incomingData[1] == devNo || incomingData[1] == DEV_NO_BROADCAST) && len <= sizeof(EspnowMessage))
    {
    Serial.printf("My No:%d, this is my msg.\n", devNo);

        EspnowMessage newMsg = {0};
        memcpy(&newMsg, incomingData, len);
        u8_t matchCounter = 0;
        if (newMsg.header == lastMessage.header)
            matchCounter++;

        for (uint8_t i = 0; i < len - 2; i++)
        {
            if (newMsg.body[i] == lastMessage.body[i])
                matchCounter++;
        }
        Serial.printf("ESPNOW:Match Count:%d\n", matchCounter);

        // Reject message duplication
        if (matchCounter == len - 1 && millis() - lastMsgTs < MESAGE_DUPLICATION_REJECT_TIME_ms)
        {
            Serial.printf("ESPNOW:Message Duplication. Rejected.\n");
            return;
        }

        // Stores the last mac address
        memcpy(&lastmac, mac, 6);
        lastMsgTs = millis();
        memcpy(&lastMessage, incomingData, len);

        if (lastMessage.myNum == DEV_NO_BROADCAST)
            lastMessage.myNum = devNo;

        Serial.printf("incoming msg header:%X\n", lastMessage.header);

        // Set message to machne class over messageQueue
        uint8_t txBuf[sizeof(EspnowMessage)] = {0};
        memcpy(&txBuf, &lastMessage, sizeof(EspnowMessage));
        xQueueSend(messageQueue, (void *)txBuf, (TickType_t)0);
    }
}

void onDataSent(const uint8_t *mac_addr, esp_now_send_status_t status)
{
    Serial.printf("\r\nLast Packet Send Status:\t");
    Serial.printf(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success\n" : "Delivery Fail\n");

    espnowSendFail = status == ESP_NOW_SEND_FAIL;

    if (espnowSendFail)
    {
        sendRetry++;
        memcpy(&lastmac, mac_addr, 6);
    }
    else
    {
        sendRetry = 0;
    }
}

void printMacAddress()
{
    Serial.printf("MAC Address: ");
    for (u8_t i = 0; i < 6; i++)
    {
        Serial.printf("%02X ", mac[i]);
    }
    Serial.printf("\n");
}

bool setNo(){
    uint8_t mac_list[DEVICE_COUNT][6] = DEVICE_MAC_ADDRESS_LIST;
    for (uint8_t i = 0; i < DEVICE_COUNT; i++)
    {
        u8_t matchCount = 0;
        if (mac_list[i][0] == mac[0])
        {
            for (u8_t j = 0; j < 6; j++)
            {
                if (mac_list[i][j] == mac[j])
                    matchCount++;
            }
            // If the match is found
            if (matchCount == 6)
            {
                devNo = i + 1;
                return true;
                break;
            }
        }
    }
    return false;
}

void setup()
{
    M5.begin(true, false, true);
    M5.dis.drawpix(0, CRGB(128, 128, 0));

    initComm();
    setNo();

    messageQueue = xQueueCreate(4, sizeof(EspnowMessage));

    delay(1000);
    printMacAddress();
    sendBootMessage();
}

void loop() {
    // Check new messages
    readIncomingMessage();

    // Do your control tasks!

    // Try to resend the last message
    if (espnowSendFail)
    {
        espnowSendFail = false;
        if (sendRetry <= ESPNOW_MAX_RETRY)
        {
            delay(1);
            resendMsg(lastmac);
        }
    }

    static bool pressed = false;

    if (M5.Btn.isPressed() || dataReceived)
    {
        if (!pressed)
        {
            M5.dis.drawpix(0, CRGB(0, 255, 0));
            pressed = true;
            dataReceived = false;
            run = false;
            sendStatusMessage();
        }
    }
    else {
        if (!run){
            pressed = false;
            M5.dis.drawpix(0, CRGB(50, 0, 0));
        }
    }

    M5.update();
    delay(50);
}
