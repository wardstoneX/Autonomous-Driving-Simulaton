#include "sc_helpers.h"

#include "OS_Socket.h"
#include "OS_Dataport.h"
#include "interfaces/if_OS_Socket.h"

#include "lib_debug/Debug.h"

#include <string.h>

#include "OS_Crypto.h"
#include <camkes.h>

#include "SecureCommunication.h"

#include "interfaces/if_OS_Entropy.h"
#include "if_KeyStore.h"
#include "if_Crypto.h"


//------------------------------------------------------------------------------

static const if_OS_Socket_t networkStackCtx =
    IF_OS_SOCKET_ASSIGN(networkStack);


static const OS_Crypto_Config_t cryptoCfg =
{
    .mode = OS_Crypto_MODE_LIBRARY,
    .entropy = IF_OS_ENTROPY_ASSIGN(
        entropy_rpc,
        entropy_dp),
};



/*
// Definition of a key-generation spec for a 256-bit AES key
static OS_CryptoKey_Spec_t aes_spec = {
    .type = OS_CryptoKey_SPECTYPE_BITS,
    .key = {
        .type = OS_CryptoKey_TYPE_AES,
        .params.bits = 256
    }
};
*/




static OS_Crypto_Handle_t hCrypto;


if_OS_Entropy_t entropy = IF_OS_ENTROPY_ASSIGN(entropy_rpc, entropy_dp);
if_KeyStore_t keystore = IF_KEYSTORE_ASSIGN(keystore_rpc, keystore_dp);
if_Crypto_t crypto = IF_CRYPTO_ASSIGN(crypto_rpc, crypto_dp);

static uint32_t hStoredKey;

static int is_initialized = 0;





//------------------------------------------------------------------------------

void post_init(void) {
    Debug_LOG_INFO("post_init() called in SecureCommunication");
    OS_Error_t ret;

    //setup crypto objects
    Debug_LOG_INFO("Initializing Crypto API.\n");
    ret = OS_Crypto_init(&hCrypto, &cryptoCfg);
    if (ret != OS_SUCCESS) {
        Debug_LOG_ERROR("Call to OS_Crypto_init() failed in SecureComunication, err %d", ret);
    }

    Debug_LOG_INFO("Waiting for NetworkStack_PicoTcp to be initialized");
    if ((ret = waitForNetworkStackInit(&networkStackCtx)) != OS_SUCCESS) {
        Debug_LOG_ERROR("waitForNetworkStackInit() failed with: %d", ret);
    }

    Debug_LOG_INFO("post_init() finished in SecureCommunication");
}









//----------------------------------------------------------------------
// if_OS_Socket
//----------------------------------------------------------------------

OS_Error_t
secureCommunication_rpc_socket_create(
    const int domain,
    const int type,
    int* const pHandle
)
{
    if(!is_initialized){
        Debug_LOG_INFO("STARTING KEY EXCHANGE");
        is_initialized = exchange_keys() == OS_SUCCESS;
        if (!is_initialized) {
            Debug_LOG_ERROR("KEy exchange failed, aborting");
            exit(1);
        }
    }
    Debug_LOG_INFO("KEY EXCHANGE COMPLETED");
    CHECK_IS_RUNNING(OS_Socket_getStatus(&networkStackCtx));

    OS_Socket_Handle_t apiHandle = {.ctx = networkStackCtx, .handleID = *pHandle};
    OS_Error_t ret = OS_Socket_create(&networkStackCtx, &apiHandle, domain, type);
    memmove(pHandle, &apiHandle.handleID, sizeof(*pHandle));

    return ret;
}


OS_Error_t
secureCommunication_rpc_socket_accept(
    const int               handle,
    int* const              pHandleClient,
    OS_Socket_Addr_t* const srcAddr
)
{
    Debug_LOG_WARNING("accept() not implemented for client connections!");
    return OS_ERROR_NOT_IMPLEMENTED;
}


OS_Error_t
secureCommunication_rpc_socket_bind(
    const int              handle,
    const OS_Socket_Addr_t* const localAddr
)
{
    Debug_LOG_WARNING("bind() not implemented for client connections!");
    return OS_ERROR_NOT_IMPLEMENTED;
}


OS_Error_t
secureCommunication_rpc_socket_listen(
    const int handle,
    const int backlog
)
{   
    Debug_LOG_WARNING("listen() not implemented for client connections!");
    return OS_ERROR_NOT_IMPLEMENTED;
}


OS_Error_t
secureCommunication_rpc_socket_connect(
    const int handle,
    const OS_Socket_Addr_t* const dstAddr
)
{
    CHECK_IS_RUNNING(OS_Socket_getStatus(&networkStackCtx));

    OS_Socket_Handle_t apiHandle = {.ctx = networkStackCtx, .handleID = handle};
    OS_Error_t ret = OS_Socket_connect(apiHandle, dstAddr);
    if (ret != OS_SUCCESS)
    {
        Debug_LOG_ERROR("OS_Socket_connect() failed, code %d", ret);
        OS_Socket_close(apiHandle);
        return ret;
    }

    //in difference to OS_Socket_connect, this function will only return after the notification of successfull connection has been received
    Debug_LOG_INFO("Waiting for connection");
    ret = waitForConnectionEstablished(apiHandle.handleID);
    if (ret != OS_SUCCESS)
    {
        Debug_LOG_ERROR("waitForConnectionEstablished() failed, error %d", ret);
        OS_Socket_close(apiHandle);
        return -1;
    }
    Debug_LOG_INFO("Connenction established");

    return ret;
}


OS_Error_t
secureCommunication_rpc_socket_close(
    const int handle
)
{
    CHECK_IS_RUNNING(OS_Socket_getStatus(&networkStackCtx));

    OS_Socket_Handle_t apiHandle = {.ctx = networkStackCtx, .handleID = handle};
    OS_Error_t ret = OS_Socket_close(apiHandle);
    return ret;
}


//TODO: compare with networkStack implementation
OS_Error_t
secureCommunication_rpc_socket_write(
    const int    handle,
    size_t* const pLen
)
{
    OS_Error_t ret;
    CHECK_IS_RUNNING(OS_Socket_getStatus(&networkStackCtx));
    OS_Socket_Handle_t apiHandle = {.ctx = networkStackCtx, .handleID = handle};

    uint8_t plaintext[*pLen];
    memcpy(plaintext, secureCommunication_rpc_buf(secureCommunication_rpc_get_sender_id()), *pLen);
    Debug_LOG_DEBUG("Got %d bytes of plaintext:", *pLen);
    Debug_DUMP_DEBUG(plaintext, *pLen);

    uint32_t keyLen = 32;
    uint8_t K_sym[keyLen];
    if(keystore.loadKey(hStoredKey, &keyLen) != 0 || keyLen != 32) {
        Debug_LOG_ERROR("There was an error when loading the symmetric key for encryption");
        return -1;
    }
    memcpy(K_sym, OS_Dataport_getBuf(keystore.dataport), 32);
    Debug_LOG_DEBUG("Read symmetric key from NV storage");

    uint8_t iv[12];
    if(12 != entropy.read(12)){
        Debug_LOG_ERROR("There was an error when generating the IV for encryption");
        return -1;
    }
    memcpy(iv, OS_Dataport_getBuf(entropy.dataport), 12);
    Debug_LOG_DEBUG("Read IV from TPM");
    Debug_DUMP_DEBUG(iv, 12);

    size_t outputsize = OS_DATAPORT_DEFAULT_SIZE;
    uint8_t payload[outputsize];
    if(encrypt_AES_GCM(hCrypto, K_sym, plaintext, *pLen, iv, payload, outputsize) != 0) {
        Debug_LOG_ERROR("There was an error when encrypting the message");
        return -1;
    }
    Debug_LOG_DEBUG("Encrypted message.");
    Debug_DUMP_DEBUG(payload, 12 + *pLen);

    size_t sentLength;
    ret = OS_Socket_write(apiHandle, payload, 12 + *pLen, &sentLength);
    
    printf("%d of %d bytes sent succesfully.", sentLength, *pLen);
    *pLen = sentLength - 12;

    return ret;
}


OS_Error_t
secureCommunication_rpc_socket_read(
    const int       handle,
    size_t* const   pLen
)
{
    OS_Error_t ret;
    CHECK_IS_RUNNING(OS_Socket_getStatus(&networkStackCtx));

    OS_Socket_Handle_t apiHandle = {.ctx = networkStackCtx, .handleID = handle};
    uint8_t buf[OS_DATAPORT_DEFAULT_SIZE];
    uint8_t plaintext[OS_DATAPORT_DEFAULT_SIZE];
    
    uint8_t* position = buf;
    size_t read = 0;

    ret = OS_Socket_read(apiHandle, position, sizeof(buf) - (position - buf), &read);
    if(ret == OS_SUCCESS) {
        Debug_LOG_INFO("OS_Socket_read() - bytes read: %d, err: %d", read, ret);
    } else {
        read = 0;
        memcpy(pLen, &read, sizeof(*pLen));
        return ret;
    }

    Debug_LOG_DEBUG("GOT %d BYTES OF CIPHERTEXT", read);

    if(read < 12) {
        Debug_LOG_ERROR("Not enough bytes were read, read operation unsuccessful");
        read = 0;
        memcpy(pLen, &read, sizeof(*pLen));
        return -1;
    }

    Debug_LOG_DEBUG("loading key from TPM");
    uint32_t keyLen = 32;
    uint8_t K_sym[keyLen];
    if(keystore.loadKey(hStoredKey, &keyLen) != 0 || keyLen != 32) {
        Debug_LOG_ERROR("There was an error when loading the symmetric key for encryption");
        read = 0;
        memcpy(pLen, &read, sizeof(*pLen));
        return -1;

    }
    memcpy(K_sym, OS_Dataport_getBuf(keystore.dataport), 32);
    Debug_LOG_DEBUG("key loaded from TPM");

    if(decrypt(hCrypto, K_sym, buf, read, plaintext, sizeof(plaintext)) != 0) {
        Debug_LOG_ERROR("There was an error when decrypting the received message");
        read = 0;
        memcpy(pLen, &read, sizeof(*pLen));
        return -1;
    }
    Debug_LOG_DEBUG("message decrypted");

    read -= 12;
    memmove(secureCommunication_rpc_buf(secureCommunication_rpc_get_sender_id()), plaintext, read);
    memmove(pLen, &read, sizeof(*pLen));
    //NOTE: the Socket API only copies the data in the dataport if the return code is OS_SUCCESS
    return OS_SUCCESS;
}


OS_Error_t
secureCommunication_rpc_socket_sendto(
    const int               handle,
    size_t* const           pLen,
    const OS_Socket_Addr_t* const dstAddr
)
{
    Debug_LOG_WARNING("sendto() is not implemented in the SecureCommunication component, use write() instead.");
    return OS_ERROR_NOT_IMPLEMENTED;
}


//TODO: compare with networkStack implementation
OS_Error_t
secureCommunication_rpc_socket_recvfrom(
    int                 handle,
    size_t*             pLen,
    OS_Socket_Addr_t*   srcAddr
)
{
    OS_Error_t ret;
    CHECK_IS_RUNNING(OS_Socket_getStatus(&networkStackCtx));

    OS_Socket_Handle_t apiHandle = {.ctx = networkStackCtx, .handleID = handle};
    uint8_t buf[OS_DATAPORT_DEFAULT_SIZE];
    uint8_t plaintext[OS_DATAPORT_DEFAULT_SIZE];
    size_t actualLen;
    

/*RECEIVING DATA FROM THE NETWORK STACK*/
    /**
     * The socket we get is of type STREAM (used for TCP connections)
     * Therefore, the call we make to the network stack is OS_Socket_read() instead of _recvfrom()
     * because _recvfrom() would expect a socket of type DGRAM (used for UDP connections)
     */
    uint8_t* position = buf;
    size_t read = 0;

    do {
        seL4_Yield();
        ret = OS_Socket_read(apiHandle, position, sizeof(buf) - (position - buf), &read);

        Debug_LOG_INFO("OS_Socket_read() - bytes read: %d, err: %d", read, ret);

        switch (ret)
        {
        case OS_SUCCESS:
            position = &position[read];
            break;
        case OS_ERROR_TRY_AGAIN:
            Debug_LOG_WARNING("OS_Socket_read() reported try again");
            continue;
        case OS_ERROR_CONNECTION_CLOSED:
            Debug_LOG_WARNING("connection closed");
            read = 0;
            /*
            memmove(pLen, &read, sizeof(*pLen));
            return ret;
            */
        case OS_ERROR_NETWORK_CONN_SHUTDOWN:
            Debug_LOG_WARNING("connection shut down");
            read = 0;
            /*
            memmove(pLen, &read, sizeof(*pLen));
            return ret;
            */
        default:
            Debug_LOG_ERROR("Message retrieval failed while reading, "
                            "OS_Socket_read() returned error code %d, bytes read %zu",
                            ret, (size_t) (position - buf));
            read = 0;
            /*
            memmove(pLen, &read, sizeof(*pLen));
            return ret;
            */
        }
    } while (read > 0 || ret == OS_ERROR_TRY_AGAIN);

    actualLen = position - buf;
    Debug_LOG_DEBUG("GOT %d BYTES OF CIPHERTEXT", actualLen);

    if(actualLen < 12) {
        Debug_LOG_ERROR("Not enough bytes were read, read operation unsuccessful");
        actualLen = 0;
        memcpy(pLen, &actualLen, sizeof(*pLen));
        return -1;
    }

    uint8_t* K_sym = malloc(32);
    uint32_t keyLen = 32;
    if(keystore.loadKey(hStoredKey, &keyLen) != 0 || keyLen != 32) {
        Debug_LOG_ERROR("There was an error when loading the symmetric key for encryption");
        actualLen = 0;
        memcpy(pLen, &actualLen, sizeof(*pLen));
        return -1;

    }
    memcpy(K_sym, OS_Dataport_getBuf(keystore.dataport), 32);

    if(decrypt(hCrypto, K_sym, buf, actualLen, plaintext, sizeof(plaintext)) != 0) {
        Debug_LOG_ERROR("There was an error when decrypting the received message");
        actualLen = 0;
        memcpy(pLen, &actualLen, sizeof(*pLen));
        return -1;
    }

    actualLen -= 12;
    memmove(secureCommunication_rpc_buf(secureCommunication_rpc_get_sender_id()), plaintext, actualLen);
    memmove(pLen, &actualLen, sizeof(*pLen));
    //NOTE: the Socket API only copies the data in the dataport if the return code is OS_SUCCESS
    return OS_SUCCESS;
}


OS_NetworkStack_State_t
secureCommunication_rpc_socket_getStatus(
    void)
{
    Debug_LOG_DEBUG("get_Status() called");
    return OS_Socket_getStatus(&networkStackCtx);
}



OS_Error_t
secureCommunication_rpc_socket_getPendingEvents(
    size_t  bufSize,
    int*    pNumberOfEvents
)
{
    Debug_LOG_WARNING("getPendingEvents() not implemented!");
    return OS_ERROR_NOT_IMPLEMENTED;
}












//----------------------------------------------------------------------
// Other functions
//----------------------------------------------------------------------




static OS_Error_t exchange_keys(void) {
    Debug_LOG_INFO("KEY EXCHANGE ALGORITHM CALLED");
    OS_Error_t ret;

    //2.- create new srk
    Debug_LOG_DEBUG("Creating SRK");
    uint32_t exp_SRK;
    keystore.getCSRK_RSA1024(&exp_SRK);
    uint8_t cSRK_ssh[8 + CSRK_SIZE];
    uint32_t converted;
    if((converted = formatForPython(exp_SRK, CSRK_SIZE, OS_Dataport_getBuf(keystore.dataport), sizeof(cSRK_ssh), cSRK_ssh)) != sizeof(cSRK_ssh)) {
        Debug_LOG_WARNING("SOMETHING WENT WRONG when converting cSRK to OpenSSH format, converted %d of %d bytes", converted, sizeof(cSRK_ssh));
        return -1;
    }
    Debug_LOG_DEBUG("SRK created");
    Debug_DUMP_DEBUG(cSRK_ssh, sizeof(cSRK_ssh));

    //3.- connect to python client and send EK_pub, SRK_pub
    Debug_LOG_DEBUG("Creating EK");
    uint32_t exp_EK;
    keystore.getCEK_RSA2048(&exp_EK);
    uint8_t cEK_ssh[8 + CEK_SIZE];
    if((converted = formatForPython(exp_EK, CEK_SIZE, OS_Dataport_getBuf(keystore.dataport), sizeof(cEK_ssh), cEK_ssh)) != sizeof(cEK_ssh)) {
        Debug_LOG_WARNING("SOMETHING WENT WRONG when converting cEK to OpenSSH format, converted %d of %d bytes", converted, sizeof(cEK_ssh));
        return -1;
    }
    Debug_LOG_DEBUG("EK created");
    Debug_DUMP_DEBUG(cEK_ssh, sizeof(cEK_ssh));

    Debug_LOG_INFO("Establishing connection to python client on keyexchange port");

    ret = waitForNetworkStackInit(&networkStackCtx);
    if (ret != OS_SUCCESS)
    {
        Debug_LOG_ERROR("waitForNetworkStackInit() failed with: %d", ret);
        return ret;
    }
    Debug_LOG_DEBUG("stack is initialized");

    OS_Socket_Handle_t hSocket;
    ret = OS_Socket_create(
              &networkStackCtx,
              &hSocket,
              OS_AF_INET,
              OS_SOCK_STREAM);
    if (ret != OS_SUCCESS)
    {
        Debug_LOG_ERROR("OS_Socket_create() failed, code %d", ret);
        return ret;
    }
    Debug_LOG_DEBUG("socket is created");

    const OS_Socket_Addr_t dstAddr =
    {
        .addr = PYTHON_ADDR,
        .port = CRYPTO_PORT
    };

    ret = OS_Socket_connect(hSocket, &dstAddr);
    if (ret != OS_SUCCESS)
    {
        Debug_LOG_ERROR("OS_Socket_connect() failed, code %d", ret);
        OS_Socket_close(hSocket);
        return ret;
    }
    Debug_LOG_DEBUG("returned from connect call!");

    ret = waitForConnectionEstablished(hSocket.handleID);
    if (ret != OS_SUCCESS)
    {
        Debug_LOG_ERROR("waitForConnectionEstablished() failed, error %d", ret);
        OS_Socket_close(hSocket);
        return ret;
    }

    Debug_LOG_INFO("Connection established, sending EK_pub and SRK_pub to python client");
    
    char payload[sizeof(cEK_ssh) + sizeof(cSRK_ssh)];
    memmove(payload, cEK_ssh, sizeof(cEK_ssh));
    memmove(payload + sizeof(cEK_ssh), cSRK_ssh, sizeof(cSRK_ssh));
    Debug_LOG_DEBUG("PRINTING THE PAYLOAD");
    Debug_DUMP_DEBUG(payload, sizeof(payload));

    size_t n;
    size_t payload_len = sizeof(payload);
    do
    {
        seL4_Yield();
        ret = OS_Socket_write(hSocket, payload, payload_len, &n);
    }
    while (ret == OS_ERROR_TRY_AGAIN);

    if (OS_SUCCESS != ret)
    {
        Debug_LOG_ERROR("OS_Socket_write() failed with error code %d", ret);
        return ret;
    }

    Debug_LOG_INFO("Public keys successfully sent");

    //6.- receive response from python client
    Debug_LOG_INFO("Waiting for response from python client");
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
            position = &position[read];
            read = 0;
            break;
        case OS_ERROR_NETWORK_CONN_SHUTDOWN:
            Debug_LOG_WARNING("connection shut down");
            position = &position[read];
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

    Debug_LOG_DEBUG("Received %d bytes as a response", position - buffer);
    char ciphertext[position - buffer];
    memmove(ciphertext, buffer, sizeof(ciphertext));
    Debug_LOG_DEBUG("DUMPING RECEIVED CIPHERTEXT");
    Debug_DUMP_DEBUG(ciphertext, sizeof(ciphertext));

    //7.- decrypt the ciphertext to get K_sym
    Debug_LOG_INFO("Decrypting symmetric key");
    Debug_LOG_DEBUG("Decrypting...\n len of cyphertext is %d", sizeof(ciphertext));
    int len = sizeof(ciphertext);
    memmove(OS_Dataport_getBuf(crypto.dataport), ciphertext, len);
    crypto.decrypt_RSA_OAEP(IF_CRYPTO_KEY_CEK, &len);
    crypto.decrypt_RSA_OAEP(IF_CRYPTO_KEY_CSRK, &len);
    if(len != 256) {
        Debug_LOG_WARNING("Something might have gone wrong, received %d bytes instead of %d", len, 32);
    }

    //following block is for debug purposes
    Debug_LOG_DEBUG("PRINTING THE RECEIVED KEY DATA!!!!!!!!!!!!!!");
    Debug_LOG_DEBUG("Key:");
    Debug_DUMP_DEBUG(OS_Dataport_getBuf(crypto.dataport), 32);
    /*debug ends here*/


    //8.- store K_sym in the keystore
    Debug_LOG_INFO("Storing key into the TPM");
    memmove(OS_Dataport_getBuf(keystore.dataport), OS_Dataport_getBuf(crypto.dataport), 32);
    hStoredKey = keystore.storeKey(32);
    if(hStoredKey == -1) {
        Debug_LOG_ERROR("The key could not be stored in the TPM!");
        return -1;
    }  

    ret = OS_Socket_close(hSocket);
    if(ret != OS_SUCCESS) {
        Debug_LOG_INFO("Failed to close socket after key exchange");
    }


    Debug_LOG_INFO("Key sucessfully exchanged");
    return OS_SUCCESS;
}


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
