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

struct Tuple {
    float x;
    float y;
    float z;
};



struct scv_vector *vector;

struct scv_vector *vectors_radar;


ssize_t leftover_size = 0;
char leftover[1024];
struct scv_vector *inner_vector = NULL;



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

            // Parse the data
            struct Tuple tuple;
            
            sscanf(buffer + start + 5, "%f,%f,%f", &tuple.x, &tuple.y, &tuple.z);
            //Debug_LOG_INFO("Tuple: (%f, %f, %f)\n",  tuple.x, tuple.y, tuple.z);

            scv_push_back(vector, &tuple);
            

            start += 5 + data_size; // Move to the start of the next message
        } else if(buffer[start] == '\x14') {
			int data_size;
            memcpy(&data_size, buffer + start + 1, sizeof(int));
            data_size = ntohl(data_size); // Convert from network to host byte order

			 // Check if the complete message has been received
            if (start + 5 + data_size > size) {
                break; // Wait for the rest of the message to arrive
            }

            // Parse the data
            struct Tuple tuple;
            sscanf(buffer + start + 5, "%f,%f,%f",&tuple.x, &tuple.y, &tuple.z);

            //Debug_LOG_INFO("Tuple: (%f, %f, %f)\n",  tuple.x, tuple.y, tuple.z);

            if(inner_vector == NULL) {
                inner_vector = scv_new(sizeof(struct Tuple), 10);
            }

            //printf("Quadruple: distance = %f, x = %f, y = %f, z = %f\n", quadruple.distance, quadruple.x, quadruple.y, quadruple.z);

            scv_push_back(inner_vector, &tuple);

            start += 5 + data_size; 
		} else if(buffer[start] == '\x15') {
            // End of batch, prepare for the next one
            scv_push_back(vectors_radar, &inner_vector);


            inner_vector = NULL;
            start++;
        } else {
            // Handle other types of data
            start++;
        }
    }

     if (start < size) {
        memmove(leftover, buffer + start, size - start);
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
    vector = scv_new(sizeof(struct Tuple), 10);
	vectors_radar = scv_new(sizeof(struct scv_vector*), 10);

    // Check and wait until the NetworkStack component is up and running.
    OS_Error_t ret = waitForNetworkStackInit(&networkStackCtx);
    Debug_LOG_INFO("network stack is initialized...");

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
       Debug_LOG_INFO("socket is created...");
          
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
        Debug_LOG_INFO("socket is connected...");

    if (ret != OS_SUCCESS)
    {
        Debug_LOG_ERROR("OS_Socket_connect() failed, code %d", ret);
        OS_Socket_close(hSocket);
        return ret;
    }

    ret = waitForConnectionEstablished(hSocket.handleID);
        Debug_LOG_INFO("connection is established...");

    if (ret != OS_SUCCESS)
    {
        Debug_LOG_ERROR("waitForConnectionEstablished() failed, error %d", ret);
        OS_Socket_close(hSocket);
        return -1;
    }

    


    Debug_LOG_INFO("HTTP request successfully sent");

    char buffer[OS_DATAPORT_DEFAULT_SIZE];
   
    size_t read = 0;
  
	

  
   

    do {
        seL4_Yield();
        ret = OS_Socket_read(hSocket, buffer, sizeof(buffer)-1, &read);

        //Debug_LOG_INFO("OS_Socket_read() - bytes read: %d, err: %d", read, ret);
        


       if( read > 0) {
            Debug_LOG_INFO("Bytes read: %d", read);
           


            char temp[sizeof(buffer) + leftover_size];
			if (leftover_size > 0) {
				memcpy(temp, leftover, leftover_size);
				memcpy(temp + leftover_size, buffer, read);
			} else {
				memcpy(temp, buffer, read);
			}
			process_buffer(temp, read + leftover_size);
            
       }


    } while (read > 0 || ret == OS_ERROR_TRY_AGAIN);
    

            Debug_LOG_INFO("printign vectors..");

     for (size_t i = 0; i < vector->size; ++i) {
        struct Tuple *tuple = scv_at(vector, i);
        Debug_LOG_INFO("Tuple %zu: (%f, %f, %f)\n", i, tuple->x, tuple->y, tuple->z);
    }

    for (size_t i = 0; i < vectors_radar->size; ++i) {
        struct scv_vector *inner_vector = scv_at(vectors_radar, i);
    
        Debug_LOG_INFO("Inner vector %zu: ", i);
        for (size_t j = 0; j < inner_vector->size; ++j) {
            struct Tuple *tuple = scv_at(inner_vector, j);
        Debug_LOG_INFO("Tuple %zu: (%f, %f, %f)\n", j, tuple->x, tuple->y, tuple->z);
        }
        Debug_LOG_INFO("\n");
    }


    OS_Socket_close(hSocket);

    Debug_LOG_INFO("Demo completed successfully.");

    return 0;
}