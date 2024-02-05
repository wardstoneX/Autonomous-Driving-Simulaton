/*
 * Copyright (C) 2021-2023, Hensoldt Cyber GmbH
 */

#include "OS_FileSystem.h"
#include "OS_Crypto.h"

#include "system_config.h"
#include "OS_Socket.h"
#include "interfaces/if_OS_Socket.h"

#include "lib_debug/Debug.h"
#include <string.h>

#include <camkes.h>




//----------------------------------------------------------------------
// Network
//----------------------------------------------------------------------


//by using the secureCommunication prefix, the app will talk to the secCom component through the socket API

static const if_OS_Socket_t networkStackCtx =
    IF_OS_SOCKET_ASSIGN(secureCommunication);
    

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



/*
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
*/





//------------------------------------------------------------------------------
int run()
{
    Debug_LOG_INFO("Starting test_app_server...");

    // Check and wait until the NetworkStack component is up and running.
    OS_Error_t ret = waitForNetworkStackInit(&networkStackCtx);
    if (OS_SUCCESS != ret)
    {
        Debug_LOG_ERROR("waitForNetworkStackInit() failed with: %d", ret);
        return -1;
    }



    Debug_LOG_INFO("sending an unencrypted message to python dummyserver");

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
        .addr = PYTHON_ADDR,
        .port = COMM_PORT
    };

    ret = OS_Socket_connect(hSocket, &dstAddr);
    if (ret != OS_SUCCESS)
    {
        Debug_LOG_ERROR("OS_Socket_connect() failed, code %d", ret);
        OS_Socket_close(hSocket);
        return ret;
    }
    Debug_LOG_INFO("Returned from OS_Socket_connect() in SecureCommunication");
    //everything runs fine until here!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!1

    /*
    ret = waitForConnectionEstablished(hSocket.handleID);
    if (ret != OS_SUCCESS)
    {
        Debug_LOG_ERROR("waitForConnectionEstablished() failed, error %d", ret);
        OS_Socket_close(hSocket);
        return -1;
    }
    */

    Debug_LOG_INFO("Send message to host...");

    char* request = "hello python!";

    size_t len_request = strlen(request);
    size_t n;

    do
    {
        seL4_Yield();
        ret = OS_Socket_write(hSocket, request, len_request, &n);
    }
    while (ret == OS_ERROR_TRY_AGAIN);

    if (OS_SUCCESS != ret)
    {
        Debug_LOG_ERROR("OS_Socket_write() failed with error code %d", ret);
    }

    Debug_LOG_INFO("Message successfully sent");

/////////////////////TESTING RECVFROM//////////////////////////////////////////////////////////////////////////////////
    Debug_LOG_INFO("Trying to receive reply");
    char buffer[OS_DATAPORT_DEFAULT_SIZE];
    char* position = buffer;
    size_t requestedLen = 11;
    size_t read = 0;
    OS_Socket_Addr_t srcAddr;

    do {
        ret = OS_Socket_recvfrom(hSocket, buffer, requestedLen, &read, &srcAddr);
        Debug_LOG_INFO("OS_Socket_recvfrom() - bytes read: %d, err: %d", read, ret);

        switch(ret) {
            case OS_SUCCESS:
                position += read;
                break;
            case OS_ERROR_TRY_AGAIN:
                Debug_LOG_WARNING("OS_Socket_recvfrom() reported try again");
                continue;
            case OS_ERROR_CONNECTION_CLOSED:
                Debug_LOG_WARNING("connection closed");
                read = 0;
                break;
            case OS_ERROR_NETWORK_CONN_SHUTDOWN:
                Debug_LOG_WARNING("connection shut down");
                read = 0;
                break;
            default:
                Debug_LOG_ERROR("Message retrieval failed while reading, "
                            "OS_Socket_recvfrom() returned error code %d, bytes read %zu",
                            ret, (size_t) (position - buffer));
                read = 0;
        }
    } while(read >0 || ret == OS_ERROR_TRY_AGAIN);

    // Ensure buffer is null-terminated before printing it
    buffer[sizeof(buffer) - 1] = '\0';
    printf("Received string: %s", buffer);

    OS_Socket_close(hSocket);
    Debug_LOG_INFO("Socket successfully closed");


    /*

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

    Debug_LOG_INFO("Send request to host...");

    char* request = "GET /index.html HTTP/1.0\r\n\r\n";

    size_t len_request = strlen(request);
    size_t n;

    do
    {
        seL4_Yield();
        ret = OS_Socket_write(hSocket, request, len_request, &n);
    }
    while (ret == OS_ERROR_TRY_AGAIN);

    if (OS_SUCCESS != ret)
    {
        Debug_LOG_ERROR("OS_Socket_write() failed with error code %d", ret);
    }

    Debug_LOG_INFO("HTTP request successfully sent");

    static char buffer[OS_DATAPORT_DEFAULT_SIZE];
    char* position = buffer;
    size_t read = 0;

    do {
        seL4_Yield();
        ret = OS_Socket_read(hSocket, position, sizeof(buffer) - (position - buffer), &read);

        Debug_LOG_INFO("OS_Socket_read() - bytes read: %d, err: %d", read, ret);

        switch (ret)
        {
        case OS_SUCCESS:
            position = &position[read];
            break;
        case OS_ERROR_CONNECTION_CLOSED:
            Debug_LOG_WARNING("connection closed");
            read = 0;
            break;
        case OS_ERROR_NETWORK_CONN_SHUTDOWN:
            Debug_LOG_WARNING("connection shut down");
            read = 0;
            break;
        case OS_ERROR_TRY_AGAIN:
                Debug_LOG_WARNING(
                    "OS_Socket_read() reported try again");
                seL4_Yield();
                continue;
        default:
            Debug_LOG_ERROR("HTTP page retrieval failed while reading, "
                            "OS_Socket_read() returned error code %d, bytes read %zu",
                            ret, (size_t) (position - buffer));
        }
    } while (read > 0 || ret == OS_ERROR_TRY_AGAIN);

    // Ensure buffer is null-terminated before printing it
    buffer[sizeof(buffer) - 1] = '\0';
    Debug_LOG_INFO("Got HTTP Page:\n%s", buffer);

    memcpy(fileData, buffer, sizeof(fileData));

    OS_Socket_close(hSocket);

    // ----------------------------------------------------------------------
    // Initialization of Crypto API
    // ----------------------------------------------------------------------


    printf("Initializing Crypto API.\n");

    // Definition of a key-generation spec for a 256-bit AES key
    static OS_CryptoKey_Spec_t aes_spec = {
        .type = OS_CryptoKey_SPECTYPE_BITS,
        .key = {
            .type = OS_CryptoKey_TYPE_AES,
            .params.bits = 256
        }
    };

    // Declare handles for API and objects
    OS_Crypto_Handle_t hCrypto;
    OS_CryptoKey_Handle_t hKey;
    OS_CryptoCipher_Handle_t hCipher;

    // Declare inputs and outputs for crypto operation
    uint8_t enc_text[OS_DATAPORT_DEFAULT_SIZE];
    size_t enc_text_size = sizeof(enc_text);
    uint8_t dec_text[OS_DATAPORT_DEFAULT_SIZE];
    size_t dec_text_size = sizeof(enc_text);

    OS_Crypto_init(&hCrypto, &cryptoCfg);

    // Generate a new AES key based on the spec provided above
    OS_CryptoKey_generate(&hKey, hCrypto, &aes_spec);

    // Provide 12 bytes of IV for AES-GCM
    size_t iv_size = 12;
    uint8_t iv[12] = {0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b};

    // ----------------------------------------------------------------------
    // Encryption
    // ----------------------------------------------------------------------

    printf("Encrypting String.\n");

    // Create a cipher object to encrypt data with AES-GCM (does require an IV!)
    OS_CryptoCipher_init(&hCipher,
                        hCrypto,
                        hKey,
                        OS_CryptoCipher_ALG_AES_GCM_ENC,
                        iv,
                        iv_size);

    // Start computation for the AES-GCM
    OS_CryptoCipher_start(hCipher, NULL, 0);

    // Encrypt received String
    OS_CryptoCipher_process(hCipher, fileData, fileSize, enc_text, &enc_text_size);

    printf("Encrypted Text %s.\n", enc_text);

    // ----------------------------------------------------------------------
    // Storage Test
    // ----------------------------------------------------------------------

    // Work on file system 1 (residing on partition 1)
    test_OS_FileSystem(&spiffsCfg_1);

    // Work on file system 2 (residing on partition 2)
    test_OS_FileSystem(&spiffsCfg_2);

    // ----------------------------------------------------------------------
    // Decryption
    // ----------------------------------------------------------------------

    printf("Decrypting String.\n");

    // Create a cipher object to decrypt data with AES-GCM (does require an IV!)
    OS_CryptoCipher_init(&hCipher,
                        hCrypto,
                        hKey,
                        OS_CryptoCipher_ALG_AES_GCM_DEC,
                        iv,
                        iv_size);

    // Start computation for the AES-GCM
    OS_CryptoCipher_start(hCipher, NULL, 0);

    // Decrypt loaded String
    OS_CryptoCipher_process(hCipher, enc_text, enc_text_size, dec_text, &dec_text_size);

    printf("Decrypted text: %s.\n", dec_text);

    // ----------------------------------------------------------------------
    // Deinitialization of Crypto API
    // ----------------------------------------------------------------------

    // Free everything
    OS_CryptoCipher_free(hCipher);
    OS_CryptoKey_free(hKey);
    OS_Crypto_free(hCrypto);

    */

    Debug_LOG_INFO("Demo completed successfully.");

    return 0;
}
