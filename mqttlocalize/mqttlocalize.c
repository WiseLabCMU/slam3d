//
//  mqttlocalize.c
//
//  Created by John Miller on 11/1/18.
//  Copyright � 2018 CMU. All rights reserved.
//

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <sys/time.h>
#include <unistd.h>
#include <time.h> 

#include "particleFilter.h"

#include "MQTTClient.h"

#define DATA_DIR            "../sampledata/"
#define TRACE_DIR           DATA_DIR "arena/"
#define NUM_BCNS            (12)
#define UWB_STD             (0.1f)
#define UWB_BIAS            (0.4f)
#define SKIP_TO_WAYPOINT    (1)

#define DEPLOY_FILE         TRACE_DIR "deploy.csv"
#define LINE_LEN            (1024)

#define ADDRESS     "oz.andrew.cmu.edu:1883"
#define CLIENTID    "localize"
#define QOS         1
#define TIMEOUT     10000L

#define FRMT_TAG_LOC_MSG "%f,%f,%f,%f,%f,%f,%f"

static void _getDeployment(FILE* deployFile, float deployment[NUM_BCNS][3]);
static void _publishLoc(MQTTClient client, char *topic, double t, float x, float y, float z, float theta);

void delivered(void *context, MQTTClient_deliveryToken dt);
int msgarrvd(void *context, char *topicName, int topicLen, MQTTClient_message *message);
void connlost(void *context, char *cause);

static particleFilterLoc_t _particleFilter;
static float deployment[NUM_BCNS][3];

static char *topicName_VIO;
static char *topicName_UWB;
static char *topicName_LocOut;
static char *topicName_RigOut;

static MQTTClient client;

int main(int argc, char* argv[])
{
    FILE* deployFile;
    double outT;
    float outX, outY, outZ, outTheta, dx, dy, dz, c, s, rigX, rigY, rigZ;
    char clientid[LINE_LEN];

    MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;
    int rc;
    int ch=0;

    if (argc <5) printf("Usage: %s <Subscribe_VIO_Topic> <Subscribe_UWB_Topic> <Publish_Loc_Topic> <Publish_Rig_Topic>\n", argv[0]);

    snprintf(clientid, LINE_LEN, "%s%ld", CLIENTID, time(NULL) % 1000);
    printf("Client ID:%s\n", clientid);
    MQTTClient_create(&client, ADDRESS, clientid,
        MQTTCLIENT_PERSISTENCE_NONE, NULL);
    conn_opts.keepAliveInterval = 20;
    conn_opts.cleansession = 1;

    MQTTClient_setCallbacks(client, NULL, connlost, msgarrvd, delivered);

    if ((rc = MQTTClient_connect(client, &conn_opts)) != MQTTCLIENT_SUCCESS)
    {
        printf("Failed to connect, return code %d\n", rc);
        exit(EXIT_FAILURE);
    }

    topicName_VIO = argv[1];
    topicName_UWB = argv[2];
    topicName_LocOut = argv[3];
    topicName_RigOut = argv[4];

    printf("\nSubscribing to topic %s\nfor client %s using QoS%d\n\n", topicName_VIO, clientid, QOS);
    MQTTClient_subscribe(client, argv[1], QOS);
    printf("Subscribing to topic %s\nfor client %s using QoS%d\n\n", topicName_UWB, clientid, QOS);    
    MQTTClient_subscribe(client, topicName_UWB, QOS);

    printf("Starting localization\n");
    particleFilterLoc_init(&_particleFilter);
    deployFile = fopen(DEPLOY_FILE, "r");
    _getDeployment(deployFile, deployment);
    fclose(deployFile);
    printf("Initialized\n");

    printf("'Ctrl+c' to quit.\n");

    do
    {
        if (particleFilterLoc_getTagLoc(&_particleFilter, &outT, &outX, &outY, &outZ, &outTheta))
        {
            dx = _particleFilter.lastX;
            dy = _particleFilter.lastY;
            dz = _particleFilter.lastZ;
            c = cosf(outTheta);
            s = sinf(outTheta);

            rigX = outX - (dx * c - dy * s);
            rigY = outY - (dx * s + dy * c);
            rigZ = outZ - dz;

            _publishLoc(client, topicName_RigOut, outT, rigX, rigY, rigZ, outTheta+1.57079632679f);
            _publishLoc(client, topicName_LocOut, outT, outX, outY, outZ, outTheta);
        }
        usleep(1000000);
    } while(1);

    MQTTClient_disconnect(client, 10000);
    MQTTClient_destroy(&client);
    return rc;
}

static uint8_t _getVio(char *_lineBuf, double* t, float* x, float* y, float* z)
{
    struct timeval tv;
    gettimeofday(&tv, NULL); 
    
    strtok(_lineBuf, ",");
    *t = tv.tv_sec + tv.tv_usec / 1000000.0;
    *y = (float)atof(strtok(NULL, ","));    
    *z = (float)atof(strtok(NULL, ","));
    *x = (float)atof(strtok(NULL, ","));

    return 1;
}

static uint8_t _getUwb(char *_lineBuf, double* t, uint8_t* b, float* r)
{
    struct timeval tv;
    gettimeofday(&tv, NULL); 
    
    *t = tv.tv_sec + tv.tv_usec / 1000000.0;
    *b = atoi(strtok(_lineBuf, ",")); 
    *r = (float)atof(strtok(NULL, ","));

    return 1;
}

static void _getDeployment(FILE* deployFile, float deployment[NUM_BCNS][3])
{
    static char _lineBuf[LINE_LEN];
    int i;
    uint8_t b;

    for (i = 0; i < NUM_BCNS; ++i)
    {
        if (fgets(_lineBuf, LINE_LEN, deployFile) == NULL)
            return;
        b = (uint8_t)atoi(strtok(_lineBuf, ","));
        assert(b < NUM_BCNS);
        deployment[b][0] = (float)atof(strtok(NULL, ","));
        deployment[b][1] = (float)atof(strtok(NULL, ","));
        deployment[b][2] = (float)atof(strtok(NULL, ",\n"));
    }
}

static void _writeTagLoc(FILE* outFile, double t, float x, float y, float z, float theta)
{
    static uint8_t printedHeaders = 0;
    if (!printedHeaders)
    {
        fprintf(outFile, "t,x,y,z,theta\n");
        printedHeaders = 1;
    }
    fprintf(outFile, "%lf,%f,%f,%f,%f\n", t, x, y, z, theta);
}

static void _publishLoc(MQTTClient client, char *topic, double t, float x, float y, float z, float theta)
{
    MQTTClient_deliveryToken token;
    MQTTClient_message pubmsg = MQTTClient_message_initializer;
    char str_msg[100];

    snprintf(str_msg, 100, FRMT_TAG_LOC_MSG, y, z, x, 0.0f, sinf(theta/2), 0.0f, cosf(theta/2));

    pubmsg.payload = str_msg;
    pubmsg.payloadlen = strlen(str_msg);
    pubmsg.qos = QOS;
    pubmsg.retained = 0;
    printf("UPDT : [%s] %s\n", topic, str_msg);
    MQTTClient_publishMessage(client, topic, &pubmsg, &token);
}

void delivered(void *context, MQTTClient_deliveryToken dt)
{
    //printf("Message with token value %d delivery confirmed\n", dt);
    //deliveredtoken = dt;
}

int msgarrvd(void *context, char *topicName, int topicLen, MQTTClient_message *message)
{
    double vioT, uwbT, outT;
    float vioX, vioY, vioZ, uwbR, outX, outY, outZ, outTheta;
    uint8_t uwbB;
    char payload_str[LINE_LEN];

    assert(message->payloadlen < LINE_LEN-1);
    memcpy(payload_str, message->payload, message->payloadlen);
    payload_str[message->payloadlen] = '\0';

    //printf("#topic: %s; message:%s", topicName, payload_str);  
    printf("#topic: %s ", topicName);  

    if (strncmp(topicName, topicName_VIO, strlen(topicName_VIO)) == 0) {
        _getVio(payload_str, &vioT, &vioX, &vioY, &vioZ);
        printf("VIO  :%lf,%f,%f,%f\n", vioT, vioX, vioY, vioZ);
        particleFilterLoc_depositVio(&_particleFilter, vioT, vioX, vioY, vioZ, 0.0f);
    } else if (strncmp(topicName, topicName_UWB, strlen(topicName_UWB)) == 0) {
        _getUwb(payload_str, &uwbT, &uwbB, &uwbR);
        printf("UWB  :%lf,%d,%f\n", uwbT, uwbB, uwbR);
        assert(uwbB<NUM_BCNS);
        uwbR -= UWB_BIAS;
        if (uwbR > 0.0f && uwbR < 30.0f)
            particleFilterLoc_depositRange(&_particleFilter, deployment[uwbB][0], deployment[uwbB][1], deployment[uwbB][2], uwbR, UWB_STD);
    }
    MQTTClient_freeMessage(&message);
    MQTTClient_free(topicName);
    return 1;
}

void connlost(void *context, char *cause)
{
    printf("\nConnection lost\n");
    printf("     cause: %s\n", cause);
}
