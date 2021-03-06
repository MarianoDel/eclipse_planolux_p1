/*******************************************************************************
 * Copyright (c) 2014 IBM Corp.
 *
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * and Eclipse Distribution License v1.0 which accompany this distribution.
 *
 * The Eclipse Public License is available at
 *    http://www.eclipse.org/legal/epl-v10.html
 * and the Eclipse Distribution License is available at
 *   http://www.eclipse.org/org/documents/edl-v10.php.
 *
 * Contributors:
 *    Allan Stockdill-Mander/Ian Craggs - initial API and implementation and/or initial documentation
 *******************************************************************************/

#ifndef __MQTT_CLIENT_C_
#define __MQTT_CLIENT_C_

#include "MQTTPacket.h"
#include "stdio.h"
#include "MQTTConnect.h"
/* Platform Dependent Inlclude */ 
#include "MQTT_SPWF_interface.h"

#define MAX_PACKET_ID 65535
#define MAX_MESSAGE_HANDLERS 5
    

enum QoS { QOS0, QOS1, QOS2 };

// all failure return codes must be negative
enum returnCode { BUFFER_OVERFLOW = -2, FAILURE = -1, OK = 0 };

//void NewTimer(Timer*);

typedef struct MQTTmsg
{
    enum QoS qos;
    char retained;
    char dup;
    unsigned short id;
    void *payload;
    size_t payloadlen;
}MQTTMessage;

typedef struct MsgData
{
    MQTTMessage* message;
    MQTTString* topicName;
}MessageData;

typedef void (*messageHandler)(MessageData*);

typedef struct client {
    unsigned int next_packetid;
    unsigned int command_timeout_ms;
    size_t buf_size, readbuf_size;
    unsigned char *buf;  
    unsigned char *readbuf; 
    unsigned int keepAliveInterval;
    char ping_outstanding;
    int  isconnected;
    
    struct MessageHandlers
    {
        const char* topicFilter;
        void (*fp) (MessageData*);
    } messageHandlers[MAX_MESSAGE_HANDLERS];      // Message handlers are indexed by subscription topic
    
    void (*defaultMessageHandler) (MessageData*);
    
    Network* ipstack;
    Timer ping_timer;
 }Client;

int MQTTConnect (Client*, MQTTPacket_connectData*);
int MQTTPublish (Client*, const char*, MQTTMessage*);
int MQTTSubscribe (Client*, const char*, enum QoS, messageHandler);
int MQTTUnsubscribe (Client*, const char*);
int MQTTDisconnect (Client*);
int MQTTYield (Client*, int);

void setDefaultMessageHandler(Client*, messageHandler);

void MQTTClient(Client*, Network*, unsigned int, unsigned char*, size_t, unsigned char*, size_t);

//los mios para poder llamar desde afuera
int getNextPacketId(Client *);
int sendPacket(Client*, int, Timer*);
int waitfor(Client*, int , Timer*);
int cycle(Client*, Timer*);


#define DefaultClient {0, 0, 0, 0, NULL, NULL, 0, 0, 0}

#endif
