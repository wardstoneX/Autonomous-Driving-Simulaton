/*
 * Copyright (C) 2021-2023, Hensoldt Cyber GmbH
 */



#include "system_config.h"
#include "OS_Socket.h"
#include "interfaces/if_OS_Socket.h"

#include "lib_debug/Debug.h"
#include <string.h>

#include <camkes.h>

#include <netinet/in.h>

#include "scv.h"
#include <stdlib.h>
#include <tgmath.h>

#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <tgmath.h>


#include "include/scv/scv.h"
#include "include/scv/scv.c"

#include <stdbool.h>

#define PORT 6061
struct ControlData {
    float throttle;
    float steer;
    float brake;
    int reverse;
    float time;
};

struct Tuple {
    float x;
    float y;
    float z;
};

struct PairOfTuples {
    struct Tuple mainVehiclePosition;
    struct Tuple OtherVehiclePosition;
};

int sock = 0;
struct PairOfTuples lastDetectedVehicle = {{0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}};
struct Tuple midpoint;
bool parkingSpotDetected = false;

struct scv_vector *vector;

struct scv_vector *vector_radar;

ssize_t leftover_size = 0;
char leftover[1024];
bool isZeroTuple(struct Tuple tuple) {
    return tuple.x == 0.0 && tuple.y == 0.0; //&& tuple.z == 0.0;
}

float calculateEuclideanDistance(struct Tuple* tuple1, struct Tuple*tuple2) {
    float dx = tuple2->x - tuple1->x;
    float dy = tuple2->y - tuple1->y;
    float dz = tuple2->z - tuple1->z;

    return sqrt(dx * dx + dy * dy + dz * dz);
}
double predict_power(double x) {
    // Use the parameters obtained from curve fitting
    double a = 1.27038607;
    double b = 0.77445365;

    // Calculate y based on the power function
    double y = a * pow(x, b);

    return y;
}
struct Tuple calculateMidpoint(struct Tuple *point1, struct Tuple *point2) {
    struct Tuple midpoint;
    midpoint.x = (point1->x + point2->x) / 2;
    midpoint.y = (point1->y + point2->y) / 2;
    midpoint.z = (point1->z + point2->z) / 2;
    return midpoint;
}

void send_control_data(int socket, struct ControlData data) {
    // Serialize the struct into a byte stream
    char buffer[sizeof(struct ControlData)];
    memcpy(buffer, &data, sizeof(struct ControlData));

    // Send the byte stream
    send(socket, buffer, sizeof(buffer), 0);
}
void send_parameters(int sock, float throttle, float steer, float brake, int reverse, float time) {
    struct ControlData data;
    data.throttle = throttle;
    data.steer = steer;
    data.brake = brake;
    data.reverse = reverse;
    data.time = time;

    send_control_data(sock, data);
}

void park() {
    printf("Parking initiated\n");
    // lane change
    send_parameters(sock,0.4, 0.3, 0,0, 3.35);
    send_parameters(sock,0.4, -0.3, 0,0, 1.65);
    send_parameters(sock,0.0, 0.0, 1.0,0, 2);
    double dist = lastDetectedVehicle.mainVehiclePosition.x - midpoint.x;
    double time = predict_power(dist);
    printf("Distance: %f, Time: %f\n", dist, time);
    send_parameters(sock,0.4, 0.0, 0.0,1, time - 5.25);
    // initiate the parking process
    send_parameters(sock,0.0, 0.0, 1.0,0, 2);

    send_parameters(sock,0.3, 0.6, 0.0,1, 3.75);

    send_parameters(sock,0.2, -0.6, 0.0,1, 2.25);

    send_parameters(sock,0.0, 0.0, 1.0,0, 5);



}

void checkForParking() {
    uint16_t t = 0;
    while(1) {
        ssize_t size = vector->size;
        ssize_t size_radar = vector_radar->size;
        if (size == 0 || size_radar == 0) {
            return;
        }
        struct Tuple *mainGNSSposition = (struct Tuple *) scv_front(vector);
        struct Tuple *radarDetection = (struct Tuple *) scv_front(vector_radar);

        //printf("radar detection:  (%f, %f, %f)\n", radarDetection->x, radarDetection->y, radarDetection->z);

        scv_pop_front(vector);
        scv_pop_front(vector_radar);

        if (isZeroTuple(*radarDetection)) {
            continue;
        }

        if (isZeroTuple(lastDetectedVehicle.mainVehiclePosition) && isZeroTuple(lastDetectedVehicle.OtherVehiclePosition)) {
            lastDetectedVehicle.OtherVehiclePosition = *radarDetection;
            lastDetectedVehicle.mainVehiclePosition = *mainGNSSposition;
            printf("main vehicle at position (%f, %f, %f) detected other vehicle at position (%f, %f, %f)\n",mainGNSSposition->x, mainGNSSposition->y, mainGNSSposition->z, radarDetection->x, radarDetection->y, radarDetection->z);
            continue;
        }

        float distance = calculateEuclideanDistance(&lastDetectedVehicle.OtherVehiclePosition, radarDetection);
        //printf("Distance: %f\n", distance);
        if (distance > 4.0) {
            if (distance > 12) {
                printf("Park spot found\n");
                send_parameters(sock, 0.0, 0.0, 1.0, 0, 3);

                // calculate the middle point of two detections here
                midpoint = calculateMidpoint(&lastDetectedVehicle.mainVehiclePosition, &lastDetectedVehicle.OtherVehiclePosition);
                printf("Midpoint: (%f, %f, %f)\n", midpoint.x, midpoint.y, midpoint.z);
                parkingSpotDetected = true;
                lastDetectedVehicle.OtherVehiclePosition = *radarDetection;
                lastDetectedVehicle.mainVehiclePosition = *mainGNSSposition;
                return;
            }
            lastDetectedVehicle.OtherVehiclePosition = *radarDetection;
            lastDetectedVehicle.mainVehiclePosition = *mainGNSSposition;
            printf("New vehicle detected!!!!!!! main vehicle at position (%f, %f, %f) detected other vehicle at position (%f, %f, %f)\n",mainGNSSposition->x, mainGNSSposition->y, mainGNSSposition->z, radarDetection->x, radarDetection->y, radarDetection->z);

            continue;
        }

    }
}


void process_buffer(char* buffer, ssize_t size) {
    int start = 0;
    while (start < size) {
        // Check if there's enough data to parse the header
        if (size - start < 5) {
            break; // Wait for the rest of the header to arrive
        }

        // Check the type of data
        if (buffer[start] == '\x13') {
            // Parse the size of the data
            int data_size;
            memcpy(&data_size, buffer + start + 1, sizeof(int));
            data_size = ntohl(data_size); // Convert from network to host byte order

            // Check if the complete message has been received
            if (start + 5 + data_size > size) {
                break; // Wait for the rest of the message to arrive
            }

            // Check for the end of the data pair
            if (buffer[start + 5 + data_size] != '\x14') {
                fprintf(stderr, "Error: Expected '\\x14' at the end of the data pair, but found '\\x%x'\n", buffer[start + 5 + data_size]);
                break; // Wait for the rest of the message to arrive
            }

            char* data = malloc(data_size + 1);
            memcpy(data, buffer + start + 5, data_size);
            data[data_size] = '\0'; // Null-terminate the string

            struct Tuple radar, gnss;
            sscanf(data, "%f,%f,%f-%f,%f,%f", &radar.x, &radar.y, &radar.z, &gnss.x, &gnss.y, &gnss.z);
            //printf(" GNSS: (%f, %f, %f)\n", gnss.x, gnss.y, gnss.z);

            scv_push_back(vector_radar, &radar);
            scv_push_back(vector, &gnss);

            start += 5 + data_size + 1; // Move to the start of the next message

            free(data);
        } else {
            // Handle other types of data
            start++;
        }
    }

    if(!parkingSpotDetected)
        checkForParking();



    // Update the leftover data
    if (start < size) {
        memmove(buffer, buffer + start, size - start);
        leftover_size = size - start; // Update the size of the leftover data
    } else {
        leftover_size = 0; // No leftover data
    }
}



//----------------------------------------------------------------------
// Network
//----------------------------------------------------------------------

static const if_OS_Socket_t networkStackCtx =
    IF_OS_SOCKET_ASSIGN(networkStack);

//------------------------------------------------------------------------------
static OS_Error_t
waitForNetworkStackInit(
    const if_OS_Socket_t* const ctx)
{
    OS_NetworkStack_State_t networkStackState;

    for (;;)
    {
        networkStackState = OS_Socket_getStatus(ctx);
        if (networkStackState == RUNNING)
        {
            // NetworkStack up and running.
            return OS_SUCCESS;
        }
        else if (networkStackState == FATAL_ERROR)
        {
            // NetworkStack will not come up.
            Debug_LOG_ERROR("A FATAL_ERROR occurred in the Network Stack component.");
            return OS_ERROR_ABORTED;
        }

        // Yield to wait until the stack is up and running.
        seL4_Yield();
    }
}

static OS_Error_t
waitForConnectionEstablished(
    const int handleId)
{
    OS_Error_t ret;

    // Wait for the event letting us know that the connection was successfully
    // established.
    for (;;)
    {
        ret = OS_Socket_wait(&networkStackCtx);
        if (ret != OS_SUCCESS)
        {
            Debug_LOG_ERROR("OS_Socket_wait() failed, code %d", ret);
            break;
        }

        char evtBuffer[128];
        const size_t evtBufferSize = sizeof(evtBuffer);
        int numberOfSocketsWithEvents;

        ret = OS_Socket_getPendingEvents(
                  &networkStackCtx,
                  evtBuffer,
                  evtBufferSize,
                  &numberOfSocketsWithEvents);
        if (ret != OS_SUCCESS)
        {
            Debug_LOG_ERROR("OS_Socket_getPendingEvents() failed, code %d",
                            ret);
            break;
        }

        if (numberOfSocketsWithEvents == 0)
        {
            Debug_LOG_TRACE("OS_Socket_getPendingEvents() returned "
                            "without any pending events");
            continue;
        }

        // We only opened one socket, so if we get more events, this is not ok.
        if (numberOfSocketsWithEvents != 1)
        {
            Debug_LOG_ERROR("OS_Socket_getPendingEvents() returned with "
                            "unexpected #events: %d", numberOfSocketsWithEvents);
            ret = OS_ERROR_INVALID_STATE;
            break;
        }

        OS_Socket_Evt_t event;
        memcpy(&event, evtBuffer, sizeof(event));

        if (event.socketHandle != handleId)
        {
            Debug_LOG_ERROR("Unexpected handle received: %d, expected: %d",
                            event.socketHandle, handleId);
            ret = OS_ERROR_INVALID_HANDLE;
            break;
        }

        // Socket has been closed by NetworkStack component.
        if (event.eventMask & OS_SOCK_EV_FIN)
        {
            Debug_LOG_ERROR("OS_Socket_getPendingEvents() returned "
                            "OS_SOCK_EV_FIN for handle: %d",
                            event.socketHandle);
            ret = OS_ERROR_NETWORK_CONN_REFUSED;
            break;
        }

        // Connection successfully established.
        if (event.eventMask & OS_SOCK_EV_CONN_EST)
        {
            Debug_LOG_DEBUG("OS_Socket_getPendingEvents() returned "
                            "connection established for handle: %d",
                            event.socketHandle);
            ret = OS_SUCCESS;
            break;
        }

        // Remote socket requested to be closed only valid for clients.
        if (event.eventMask & OS_SOCK_EV_CLOSE)
        {
            Debug_LOG_ERROR("OS_Socket_getPendingEvents() returned "
                            "OS_SOCK_EV_CLOSE for handle: %d",
                            event.socketHandle);
            ret = OS_ERROR_CONNECTION_CLOSED;
            break;
        }

        // Error received - print error.
        if (event.eventMask & OS_SOCK_EV_ERROR)
        {
            Debug_LOG_ERROR("OS_Socket_getPendingEvents() returned "
                            "OS_SOCK_EV_ERROR for handle: %d, code: %d",
                            event.socketHandle, event.currentError);
            ret = event.currentError;
            break;
        }
    }

    return ret;
}

//------------------------------------------------------------------------------
int run()
{
    Debug_LOG_INFO("Starting test_app_server...");

    // dont forget to clean the vectors at the end
    vector = scv_new(sizeof(struct Tuple), 1000);
    vector_radar = scv_new(sizeof(struct Tuple), 1000);
    
    

    // Check and wait until the NetworkStack component is up and running.
    OS_Error_t ret = waitForNetworkStackInit(&networkStackCtx);
    if (OS_SUCCESS != ret)
    {
        Debug_LOG_ERROR("waitForNetworkStackInit() failed with: %d", ret);
        return -1;
    }

    OS_Socket_Handle_t hSocket;
    ret = OS_Socket_create(
              &networkStackCtx,
              &hSocket,
              OS_AF_INET,
              OS_SOCK_STREAM);
    if (ret != OS_SUCCESS)
    {
        Debug_LOG_ERROR("OS_Socket_create() failed, code %d", ret);
        return -1;
    }

    const OS_Socket_Addr_t dstAddr =
    {
        .addr = CFG_TEST_HTTP_SERVER,
        .port = EXERCISE_CLIENT_PORT
    };

    ret = OS_Socket_connect(hSocket, &dstAddr);
    if (ret != OS_SUCCESS)
    {
        Debug_LOG_ERROR("OS_Socket_connect() failed, code %d", ret);
        OS_Socket_close(hSocket);
        return ret;
    }

    ret = waitForConnectionEstablished(hSocket.handleID);
    if (ret != OS_SUCCESS)
    {
        Debug_LOG_ERROR("waitForConnectionEstablished() failed, error %d", ret);
        OS_Socket_close(hSocket);
        return -1;
    }

    send_parameters(new_socket, 0.2, 0.0, 0.0, 0,0);

    while (read > 0 || ret == OS_ERROR_TRY_AGAIN) {

        // break out if we detected parking spot
        if(parkingSpotDetected) {
            break;
        }

        valread = read(new_socket, buffer, sizeof(buffer) - 1);
        if (valread < 0) {
            perror("read");
            break;
        } else if (valread == 0) {
            printf("Client disconnected\n");
            break;
        } else {
            char temp[sizeof(buffer) + leftover_size];
            if (leftover_size > 0) {
                memcpy(temp, leftover, leftover_size);
                memcpy(temp + leftover_size, buffer, valread);
            } else {
                memcpy(temp, buffer, valread);
            }
            process_buffer(temp, valread + leftover_size);

        }
    }
    printf("end reading\n");

    // initiating the parking process

    park();


    /// there is endloss loop here
    while(1);


    OS_Socket_close(hSocket);

    Debug_LOG_INFO("Demo completed successfully.");

    return 0;
}