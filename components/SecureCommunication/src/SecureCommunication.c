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



//can't include "network_stack_core.h" for some reason
#define CHECK_IS_RUNNING(_currentState_)                                       \
    do                                                                         \
    {                                                                          \
        if (_currentState_ != RUNNING)                                         \
        {                                                                      \
            if (_currentState_ == FATAL_ERROR)                                 \
            {                                                                  \
                Debug_LOG_ERROR("%s: FATAL_ERROR occurred in the NetworkStack" \
                                , __func__);                                   \
                return OS_ERROR_ABORTED;                                       \
            }                                                                  \
            else                                                               \
            {                                                                  \
                Debug_LOG_TRACE("%s: NetworkStack currently not running",      \
                __func__);                                                     \
               return OS_ERROR_NOT_INITIALIZED;                                \
            }                                                                  \
        }                                                                      \
    } while (0)




//TODO: figure out if I need to implement this function myself
seL4_Word secureCommunication_rpc_get_sender_id(void);
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




//static OS_CryptoKey_Attrib_t attr = {.flags = 0, .keepLocal = 1};




static OS_Crypto_Handle_t hCrypto;

//static OS_Dataport_t entropyPort = OS_DATAPORT_ASSIGN(entropy_dp);
static OS_Dataport_t keystorePort = OS_DATAPORT_ASSIGN(keystore_dp);
static OS_Dataport_t cryptoPort = OS_DATAPORT_ASSIGN(crypto_dp);

if_KeyStore_t keystore = IF_KEYSTORE_ASSIGN(keystore_rpc, keystore_dp);
if_Crypto_t crypto = IF_CRYPTO_ASSIGN(crypto_rpc, crypto_dp);

static uint32_t hStoredKey;





//------------------------------------------------------------------------------

void post_init(void) {
    Debug_LOG_INFO("post_init() called in SecureCommunication");

    OS_Error_t ret;
    do{
        ret = secureCommunication_setup();
    } while (ret != OS_SUCCESS);
    Debug_LOG_INFO("Setup completed.");


    do{
        ret = exchange_keys();
    } while (ret != OS_SUCCESS);


    
    Debug_LOG_INFO("Waiting for NetworkStack_PicoTcp to be initialized");
    if ((ret = waitForNetworkStackInit(&networkStackCtx)) != OS_SUCCESS)
    {
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


    uint8_t *buf = secureCommunication_rpc_buf(secureCommunication_rpc_get_sender_id());
    OS_Socket_Handle_t apiHandle = {.ctx = networkStackCtx, .handleID = handle};

    /*
    OS_CryptoCipher_Handle_t hCipher;

    uint8_t enc_text[OS_DATAPORT_DEFAULT_SIZE];
    size_t enc_text_size = sizeof(enc_text);


    
    OS_CryptoKey_Data_t key_data;
    size_t key_size;
    OS_CryptoKey_Handle_t hKey;
    if((ret = OS_Keystore_loadKey(hKeys, "symmetric key", &key_data, &key_size)) != OS_SUCCESS) {
        Debug_LOG_WARNING("Failed to retrieve key from keystore, err: %d", ret);
    }
    if((ret = OS_CryptoKey_import(&hKey, hCrypto, &key_data)) != OS_SUCCESS) {
        Debug_LOG_WARNING("Failed to import data into key object, err: %d", ret);
    }



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
    OS_CryptoCipher_process(hCipher, buf, *pLen, enc_text, &enc_text_size);

    printf("%d of %ls bytes encrypted successfully.", enc_text_size, pLen);
    printf("Encrypted Text %s.\n", enc_text);
    

    size_t sentLength;
    ret = OS_Socket_write(apiHandle, enc_text, enc_text_size, &sentLength);
    

    printf("%d of %d bytes sent succesfully.", sentLength, enc_text_size);
    memmove(pLen, &sentLength, sizeof(*pLen));
    */
    size_t sentLength;
    ret = OS_Socket_write(apiHandle, buf, *pLen, &sentLength);
    

    printf("%d of %d bytes sent succesfully.", sentLength, *pLen);
    memmove(pLen, &sentLength, sizeof(*pLen));

    return ret;
}


OS_Error_t
secureCommunication_rpc_socket_read(
    const int       handle,
    size_t* const   pLen
)
{
    Debug_LOG_WARNING("read() is not implemented in the SecureCommunication component, use recvfrom() instead.");
    return OS_ERROR_NOT_IMPLEMENTED;
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
    static char buf[OS_DATAPORT_DEFAULT_SIZE];
    //uint8_t dec_text[OS_DATAPORT_DEFAULT_SIZE];
    size_t actualLen;
    

/*RECEIVING DATA FROM THE NETWORK STACK*/
    /**
     * The socket we get is of type STREAM (used for TCP connections)
     * Therefore, the call we make to the network stack is OS_Socket_read() instead of _recvfrom()
     * because _recvfrom() would expect a socket of type DGRAM (used for UDP connections)
     */
    char* position = buf;
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
            memmove(pLen, &read, sizeof(*pLen));
            return ret;
        case OS_ERROR_NETWORK_CONN_SHUTDOWN:
            Debug_LOG_WARNING("connection shut down");
            read = 0;
            memmove(pLen, &read, sizeof(*pLen));
            return ret;
        default:
            Debug_LOG_ERROR("Message retrieval failed while reading, "
                            "OS_Socket_read() returned error code %d, bytes read %zu",
                            ret, (size_t) (position - buf));
            read = 0;
            memmove(pLen, &read, sizeof(*pLen));
            return ret;
        }
    } while (read > 0 || ret == OS_ERROR_TRY_AGAIN);

    //TODO: erase this line before reactivating decryption
    actualLen = position - buf;


/*DECRYPTING DATA*/
    /*
    printf("Decrypting String.\n");

    OS_CryptoKey_Data_t key_data;
    size_t key_size;
    OS_CryptoKey_Handle_t hKey;
    if((ret = OS_Keystore_loadKey(hKeys, "symmetric key", &key_data, &key_size)) != OS_SUCCESS) {
        Debug_LOG_WARNING("Failed to retrieve key from keystore, err: %d", ret);
    }
    if((ret = OS_CryptoKey_import(&hKey, hCrypto, &key_data)) != OS_SUCCESS) {
        Debug_LOG_WARNING("Failed to import data into key object, err: %d", ret);
    }

    // Create a cipher object to decrypt data with AES-GCM (does require an IV!)
    OS_CryptoCipher_Handle_t hCipher;
    OS_CryptoCipher_init(&hCipher,
                        hCrypto,
                        hKey,
                        OS_CryptoCipher_ALG_AES_GCM_DEC,
                        iv,
                        iv_size);

    // Start computation for the AES-GCM
    OS_CryptoCipher_start(hCipher, NULL, 0);

    size_t dec_text_size = actualLen;
    // Decrypt loaded String
    OS_CryptoCipher_process(hCipher, buf, actualLen, dec_text, &dec_text_size);

    printf("%ls of %d bytes decrypted successfully.", &dec_text_size, actualLen);
    printf("Decrypted text: %s.\n", dec_text);
    */

    memmove(secureCommunication_rpc_buf(secureCommunication_rpc_get_sender_id()), buf, actualLen);
    memmove(pLen, &actualLen, sizeof(*pLen));
    //NOTE: the Socket API only copies the data in the dataport if the return code is OS_SUCCESS
    return OS_SUCCESS;
}


OS_NetworkStack_State_t
secureCommunication_rpc_socket_getStatus(
    void)
{
    Debug_LOG_INFO("get_Status() called");
    return OS_Socket_getStatus(&networkStackCtx);
}



//TODO: compare with networkStack implementation
OS_Error_t
secureCommunication_rpc_socket_getPendingEvents(
    size_t  bufSize,
    int*    pNumberOfEvents
)
{
    CHECK_IS_RUNNING(OS_Socket_getStatus(&networkStackCtx));

    uint8_t *buf = secureCommunication_rpc_buf(secureCommunication_rpc_get_sender_id());
    OS_Error_t ret = OS_Socket_getPendingEvents(&networkStackCtx, buf, bufSize, pNumberOfEvents);
    return ret;
}







/**
 * NOTE: The following functions are not implemented evn though they are used by the Socket API. 
 * This most likely creates problems with OS_Socket_wait()
 */

/*
void
secureCommunication_event_notify_wait(void){

}

int
secureCommunication_event_notify_poll(void){
    return -1;
}

int
secureCommunication_event_notify_reg_callback(
    void (*callback)(void*),
    void* arg
){
    return -1;
}

int
secureCommunication_shared_resource_mutex_lock(void){
    return -1;
}

int
secureCommunication_shared_resource_mutex_unlock(void){
    return -1;
}

void*
secureCommunication_rpc_get_buf(void){
    return NULL;
}

size_t
secureCommunication_rpc_get_size(void){
    return 0;
}
*/





//----------------------------------------------------------------------
// Other functions
//----------------------------------------------------------------------

static OS_Error_t secureCommunication_setup(){
    /*
    //TODO: change keystore functionality to the TPM instead
    */


    OS_Error_t ret;


    //setup crypto objects
    Debug_LOG_INFO("Initializing Crypto API.\n");
    ret = OS_Crypto_init(&hCrypto, &cryptoCfg);


    return ret;

}



static OS_Error_t exchange_keys(void) {
    OS_Error_t ret;

    //TODO: 1.- take ownership of the tpm
    
    
    

    //2.- create new srk
    uint32_t exp_SRK;
    keystore.getCSRK_RSA1024(&exp_SRK);
    uint8_t cSRK_ssh[7 + 3*sizeof(uint32_t) + 1024/8];
    if(toOpenSSHPublicRSA((uint8_t*)exp_SRK, 4, OS_Dataport_getBuf(keystorePort), 1024/8, cSRK_ssh, sizeof(cSRK_ssh)) != sizeof(cSRK_ssh)) {
        Debug_LOG_WARNING("Something went wrong when converting cSRK to OpenSSH format");
    }



    //3.- connect to python client and send EK_pub, SRK_pub
    uint32_t exp_EK;
    keystore.getCEK_RSA2048(&exp_EK);
    uint8_t cEK_ssh[7 + 3*sizeof(uint32_t) + 2048/8];
    if(toOpenSSHPublicRSA((uint8_t*)exp_EK, 4, OS_Dataport_getBuf(keystorePort), 2048/8, cEK_ssh, sizeof(cEK_ssh)) != sizeof(cEK_ssh)) {
        Debug_LOG_WARNING("Something went wrong when converting cEK to OpenSSH format");
    }



    Debug_LOG_INFO("Establishing connection to python client on keyexchange port");

    if ((ret = waitForNetworkStackInit(&networkStackCtx)) != OS_SUCCESS)
    {
        Debug_LOG_ERROR("waitForNetworkStackInit() failed with: %d", ret);
        return ret;
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
        return ret;
    }

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

    ret = waitForConnectionEstablished(hSocket.handleID);
    if (ret != OS_SUCCESS)
    {
        Debug_LOG_ERROR("waitForConnectionEstablished() failed, error %d", ret);
        OS_Socket_close(hSocket);
        return ret;
    }

    Debug_LOG_INFO("Connection established, sending EK_pub and SRK_pub to python client");
    
    /**
     * The payload consists of 4 bytes stating the length of cEK, followed by the bytes of cEK and cSRK in OpenSSH format.
     */
    uint32_t cEK_ssh_size = sizeof(cEK_ssh);
    char* payload[sizeof(cEK_ssh_size) + sizeof(cEK_ssh) + sizeof(cSRK_ssh)];
    memmove(payload, &cEK_ssh_size, sizeof(uint32_t));
    memmove(payload + sizeof(uint32_t), cEK_ssh, sizeof(cEK_ssh));
    memmove(payload + sizeof(uint32_t)+ sizeof(cEK_ssh), cSRK_ssh, sizeof(cSRK_ssh));

    Debug_LOG_INFO("PRINTING THE GENERATED EK SSH");
    printf("OpenSSH of cEK: %.*s", sizeof(cEK_ssh), cEK_ssh);
  
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

    Debug_LOG_INFO("Received %d bytes as a response", position - buffer);
    char ciphertext[position - buffer];
    memmove(ciphertext, buffer, sizeof(ciphertext));


    //7.- decrypt the ciphertext to get K_sym
    Debug_LOG_INFO("Decrypting...");

    int len = sizeof(ciphertext);
    memmove(OS_Dataport_getBuf(cryptoPort), ciphertext, len);
    crypto.decrypt_RSA_OAEP(IF_CRYPTO_KEY_CEK, &len);
    crypto.decrypt_RSA_OAEP(IF_CRYPTO_KEY_CSRK, &len);

    if(len != 12 + 32) {
        Debug_LOG_WARNING("Something might have gone wrong, received %d bytes instead of %d", len, 12+32);
    }

    //following block is for debug purposes
    Debug_LOG_INFO("PRINTING THE RECEIVED KEY DATA!!!!!!!!!!!!!!");
    Debug_LOG_INFO("Key:");
    Debug_DUMP_INFO(OS_Dataport_getBuf(cryptoPort), 32);
    Debug_LOG_INFO("IV:");
    Debug_DUMP_INFO(OS_Dataport_getBuf(cryptoPort) + 32, 12);
    /*debug ends here*/

     
    


    

    //8.- store K_sym in the keystore
    memmove(OS_Dataport_getBuf(keystorePort), OS_Dataport_getBuf(cryptoPort), 32 + 12);
    hStoredKey = keystore.storeKey(32, 12);
    if(hStoredKey == -1) {
        Debug_LOG_ERROR("The key could not be stored in the TPM!");
        //TODO: find a better return value on error
        return -1;
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


