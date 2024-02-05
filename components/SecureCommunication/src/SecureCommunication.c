#include "sc_helpers.h"

#include "OS_Socket.h"
#include "OS_Dataport.h"
#include "interfaces/if_OS_Socket.h"

#include "lib_debug/Debug.h"

#include <string.h>

#include "OS_Crypto.h"
#include <camkes.h>
/*
#include "OS_FileSystem.h"
#include "OS_KeystoreFile.h"
*/

#include "SecureCommunication.h"


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




// Definition of a key-generation spec for a 256-bit AES key
static OS_CryptoKey_Spec_t rsa_prv_spec = {
    .type = OS_CryptoKey_SPECTYPE_BITS,
    .key = {
        .type = OS_CryptoKey_TYPE_RSA_PRV,
        .params.bits = 2048
    }
};



static OS_CryptoKey_Attrib_t attr = {.flags = 0, .keepLocal = 1};


// Provide 12 bytes of IV for AES-GCM
size_t iv_size = 12;
uint8_t iv[12] = {0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b};


static OS_Crypto_Handle_t hCrypto;

static OS_CryptoKey_Handle_t EK_prv;
static OS_CryptoKey_Handle_t EK_pub;

//static OS_CryptoKey_Handle_t SRK_prv;
//static OS_CryptoKey_Handle_t SRK_pub;






//------------------------------------------------------------------------------

void post_init(void) {
    Debug_LOG_INFO("post_init() called in SecureCommunication");

    OS_Error_t ret;
    do{
        ret = secureCommunication_setup();
    } while (ret != OS_SUCCESS);
    Debug_LOG_INFO("Setup completed.");

    /*/
    do{
        ret = exchange_keys();
    } while (ret != OS_SUCCESS);
    */

    
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
    } while (read < *pLen ||read > 0 || ret == OS_ERROR_TRY_AGAIN);

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
    OS_Crypto_init(&hCrypto, &cryptoCfg);


    //generate EK_pub, EK_prv
    if((ret = OS_CryptoKey_generate(&EK_prv, hCrypto, &rsa_prv_spec)) != OS_SUCCESS) {
        Debug_LOG_WARNING("Failed to generate EK_prv, err: %d", ret);
        return ret;
    }
    if((ret = OS_CryptoKey_makePublic(&EK_pub, hCrypto, EK_prv, &attr)) != OS_SUCCESS) {
        Debug_LOG_WARNING("Failed to generate EK_pub, err: %d", ret);
        return ret;
    }

    return ret;

}


/*
static OS_Error_t exchange_keys(void) {
    OS_Error_t ret;

    //1.- take ownership of the tpm
    
    if((ret = OS_Keystore_wipeKeystore(hKeys)) != OS_SUCCESS) {
        Debug_LOG_WARNING("Failed to wipe keystore, err: %d", ret);
    }
    

    //2.- create new srk
    
    OS_CryptoKey_Handle_t SRK_prv;
    OS_CryptoKey_Handle_t SRK_pub;
    

    if((ret = OS_CryptoKey_generate(&SRK_prv, hCrypto, &rsa_prv_spec)) != OS_SUCCESS) {
        Debug_LOG_WARNING("Failed to generate SRK_prv, err: %d", ret);
    }
    if((ret = OS_CryptoKey_makePublic(&SRK_pub, hCrypto, SRK_prv, &attr)) != OS_SUCCESS) {
        Debug_LOG_WARNING("Failed to generate SRK_pub, err: %d", ret);
    }


    //3.- connect to python client and send EK_pub, SRK_pub
    Debug_LOG_INFO("Establishing connection to python client on keyexchange port");

    if ((ret = waitForNetworkStackInit(&networkStackCtx)) != OS_SUCCESS)
    {
        Debug_LOG_ERROR("waitForNetworkStackInit() failed with: %d", ret);
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
    }

    const OS_Socket_Addr_t dstAddr =
    {
        .addr = CFG_TEST_HTTP_SERVER,
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
        return -1;
    }

    Debug_LOG_INFO("Connection established, sending EK_pub and SRK_pub to python client");
    OS_CryptoKey_Data_t EK_pub_data;
    OS_CryptoKey_Data_t SRK_pub_data;
    if((ret = OS_CryptoKey_export(EK_pub, &EK_pub_data)) != OS_SUCCESS) {
        Debug_LOG_WARNING("EK_pub could not be exported, err: %d", ret);
    }
    if((ret = OS_CryptoKey_export(SRK_pub, &SRK_pub_data)) != OS_SUCCESS) {
        Debug_LOG_WARNING("SRK_pub could not be exported, err: %d", ret);
    }



    uint32_t EK_eLen = EK_pub_data.data.rsa.pub.eLen;
    uint32_t EK_nLen = EK_pub_data.data.rsa.pub.nLen;
    uint32_t SRK_eLen = SRK_pub_data.data.rsa.pub.eLen;
    uint32_t SRK_nLen = SRK_pub_data.data.rsa.pub.nLen;


    uint32_t EK_SSH_length = 19 + EK_eLen + EK_nLen;
    uint32_t SRK_SSH_length = 19 + SRK_eLen + SRK_nLen;

    //the 4 extra bytes at the beginning will let the python client separate the EK from the SRK by stating the size of the EK
    size_t len_message = 4 + EK_SSH_length + SRK_SSH_length;
    uint8_t* message = malloc(len_message); 

    memmove(message, &EK_SSH_length, 4);
    int ssh_written;
    if(EK_SSH_length != (ssh_written = toOpenSSHPublicRSA(EK_pub_data.data.rsa.pub.eBytes, EK_eLen, EK_pub_data.data.rsa.pub.nBytes, EK_nLen, message+4, EK_SSH_length))) {
        Debug_LOG_WARNING("EK could not be exported properly, %d out of %d bytes processed", ssh_written, EK_SSH_length);
    }
    if(SRK_SSH_length != (ssh_written = toOpenSSHPublicRSA(SRK_pub_data.data.rsa.pub.eBytes, SRK_eLen, SRK_pub_data.data.rsa.pub.nBytes, SRK_nLen, message+4+EK_SSH_length, SRK_SSH_length))) {
        Debug_LOG_WARNING("SRK could not be exported properly, %d out of %d bytes processed", ssh_written, SRK_SSH_length);
    }
    


    
    size_t n;

    do
    {
        seL4_Yield();
        ret = OS_Socket_write(hSocket, message, len_message, &n);
    }
    while (ret == OS_ERROR_TRY_AGAIN);
    free(message);

    if (OS_SUCCESS != ret)
    {
        Debug_LOG_ERROR("OS_Socket_write() failed with error code %d", ret);
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
    char outer_ciphertext[position - buffer];
    memmove(outer_ciphertext, buffer, sizeof(outer_ciphertext));


    //7.- decrypt the ciphertext to get K_sym
    Debug_LOG_INFO("Decrypting...");
    OS_CryptoCipher_Handle_t hCipher;
    uint8_t inner_ciphertext[OS_DATAPORT_DEFAULT_SIZE];
    size_t inner_length = sizeof(inner_ciphertext);
    if((ret = OS_CryptoCipher_init(&hCipher, hCrypto, EK_prv, OS_CryptoCipher_ALG_AES_GCM_DEC, NULL, 0)) != OS_SUCCESS) {
        Debug_LOG_WARNING("Cipher object could not be initialized, err: %d", ret);
    }
    if((ret = OS_CryptoCipher_start(hCipher, NULL, 0)) != OS_SUCCESS) {
        Debug_LOG_WARNING("Cipher processing could not be started, err: %d", ret);
    }
    if((ret = OS_CryptoCipher_process(hCipher, outer_ciphertext, sizeof(outer_ciphertext), inner_ciphertext, &inner_length)) != OS_SUCCESS) {
        Debug_LOG_WARNING("Decryption failed, err: %d", ret);
    }
    if((ret = OS_CryptoCipher_finalize(hCipher, NULL, 0)) != OS_SUCCESS) {
        Debug_LOG_WARNING("Cipher object could not be finalized, err: %d", ret);
    }

    uint8_t plaintext[OS_DATAPORT_DEFAULT_SIZE];
    size_t plain_length = sizeof(plaintext);
    if((ret = OS_CryptoCipher_init(&hCipher, hCrypto, SRK_prv, OS_CryptoCipher_ALG_AES_GCM_DEC, NULL, 0)) != OS_SUCCESS) {
        Debug_LOG_WARNING("Cipher object could not be initialized, err: %d", ret);
    }
    if((ret = OS_CryptoCipher_start(hCipher, NULL, 0)) != OS_SUCCESS) {
        Debug_LOG_WARNING("Cipher processing could not be started, err: %d", ret);
    }
    if((ret = OS_CryptoCipher_process(hCipher, inner_ciphertext, inner_length, plaintext, &plain_length)) != OS_SUCCESS) {
        Debug_LOG_WARNING("Decryption failed, err: %d", ret);
    }
    if((ret = OS_CryptoCipher_finalize(hCipher, NULL, 0)) != OS_SUCCESS) {
        Debug_LOG_WARNING("Cipher object could not be finalized, err: %d", ret);
    }



    Debug_LOG_INFO("Loading data into key object");
    OS_CryptoKey_Data_t K_data = {0};

    //  fields necessary for K_sym
    //uint32_t len of the key (in bytes)
    //uint8_t pointer to bytes of the key     

    //we already know we'll get a AES256 key, so the length will be 256/8=32 bytes
    K_data.type = OS_CryptoKey_TYPE_AES;
    K_data.attribs = attr;
    K_data.data.aes.len = 32;
    memmove(K_data.data.aes.bytes, plaintext, K_data.data.aes.len);


    //8.- store K_sym in the keystore
    
    if((ret = OS_Keystore_storeKey(hKeys, "symmetric key", &K_data, sizeof(K_data))) != OS_SUCCESS) {
        Debug_LOG_WARNING("Failed to store key data in the key store, err: %d", ret);
    }
    

    Debug_LOG_INFO("Key sucessfully exchanged");
    return ret;
}
*/


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


