/*
 * Copyright (C) 2021-2023, Hensoldt Cyber GmbH
 */

#include "system_config.h"
#include "OS_Socket.h"
#include "interfaces/if_OS_Socket.h"
#include "lib_debug/Debug.h"
#include <string.h>
#include <camkes.h>
#include "utils.h"
#include "include/scv/scv.h"
#include <stdio.h>
#include <stdlib.h>

//#include <netinet/in.h>
//#include "scv.h"
//#include <tgmath.h>
//#include <netinet/in.h>
//#include <sys/socket.h>
//#include <unistd.h>
//#include <tgmath.h>
//#include <stdbool.h>

OS_Socket_Handle_t sock;

struct Tuple midpoint;
bool parkingSpotDetected = false;

struct scv_vector *vector;
struct scv_vector *vector_radar;

ssize_t leftover_size = 0;
char leftover[1024];

void park(OS_Socket_Handle_t sock) {
    printf("Parking initiated\n");
    // lane change
    send_parameters(sock,0.4, 0.3, 0,0, 3.35);
    send_parameters(sock,0.4, -0.3, 0,0, 1.65);
    send_parameters(sock,0.0, 0.0, 1.0,0, 2);

    //move to parking place
    
    //double dist = lastDetectedVehicle.mainVehiclePosition.x - midpoint.x;
    double dist1 = lastDetectedVehicle.mainVehiclePosition.x -lastDetectedVehicle.OtherVehiclePosition.x;
    double time = predict_power(dist1);

    printf("Distance: %f, Time: %f\n", dist1, time);
    send_parameters(sock,0.4, 0.0, 0.0,1, time);
    //send_parameters(sock,0.4, 0.0, 0.0,1, time - 5.25);



    // initiate the parking process
    send_parameters(sock,0.0, 0.0, 1.0,0, 2);

    send_parameters(sock,0.3, 0.6, 0.0,1, 3.75);

    send_parameters(sock,0.2, -0.6, 0.0,1, 2.25);

    send_parameters(sock,0.0, 0.0, 1.0,0, 5);

}

void checkForParking() {
    
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

    
    checkForParking();



    // Update the leftover data
    if (start < size) {
        memmove(buffer, buffer + start, size - start);
        leftover_size = size - start; // Update the size of the leftover data
    } else {
        leftover_size = 0; // No leftover data
    }
}

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

    OS_Socket_Handle_t new_socket;
    ret = OS_Socket_create(
              &networkStackCtx,
              &new_socket,
              OS_AF_INET,
              OS_SOCK_STREAM);
    if (ret != OS_SUCCESS)
    {
        Debug_LOG_ERROR("OS_Socket_create() failed, code %d", ret);
        return -1;
    }

    const OS_Socket_Addr_t dstAddr =
    {
        .addr = SERVER_IP,
        .port = SERVER_PORT
    };

    ret = OS_Socket_connect(new_socket, &dstAddr);
    if (ret != OS_SUCCESS)
    {
        Debug_LOG_ERROR("OS_Socket_connect() failed, code %d", ret);
        OS_Socket_close(new_socket);
        return ret;
    }

    ret = waitForConnectionEstablished(new_socket.handleID);
    if (ret != OS_SUCCESS)
    {
        Debug_LOG_ERROR("waitForConnectionEstablished() failed, error %d", ret);
        OS_Socket_close(new_socket);
        return -1;
    }
    sock = new_socket;
    char buffer[4096] = { 0 };
    send_parameters(new_socket, 0.2, 0.0, 0.0, 0,0);
    size_t valread = 0;

    while (1) {

        // break out if we detected parking spot
        if(parkingSpotDetected) {
            break;
        }

        ret = OS_Socket_read(new_socket, buffer, sizeof(buffer) - 1, &valread);
        
        if (valread > 0) {
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

    park(new_socket);


    /// there is endloss loop here
    send_parameters(new_socket,0.0, 0.0, 0.0,0, 0);
    printf("Parking completed\n Command to end simulation sent\n");


    OS_Socket_close(new_socket);
    scv_delete(vector);
    scv_delete(vector_radar);
    Debug_LOG_INFO("Demo completed successfully.");

    return 0;
}
