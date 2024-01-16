#include "OS_Socket.h"
#include "OS_Dataport.h"
#include "OS_Socket.h"
#include "interfaces/if_OS_Socket.h"

#include "lib_debug/Debug.h"
#include "lib_macros/Check.h"

#include <string.h>

#include "OS_Crypto.h"
#include <camkes.h>


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

static OS_Crypto_Handle_t hCrypto;
static OS_CryptoKey_Handle_t hKey;

// Definition of a key-generation spec for a 256-bit AES key
static OS_CryptoKey_Spec_t aes_spec = {
    .type = OS_CryptoKey_SPECTYPE_BITS,
    .key = {
        .type = OS_CryptoKey_TYPE_AES,
        .params.bits = 256
    }
};

// Provide 12 bytes of IV for AES-GCM
size_t iv_size = 12;
uint8_t iv[12] = {0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b};




//------------------------------------------------------------------------------

void post_init(void) {
    printf("Initializing Crypto API.\n");

    OS_Crypto_init(&hCrypto, &cryptoCfg);

    //TODO: get the key from the TPM instead
    // Generate a new AES key based on the spec provided above
    OS_CryptoKey_generate(&hKey, hCrypto, &aes_spec);

}








//----------------------------------------------------------------------
// if_OS_Socket
//----------------------------------------------------------------------

//TODO: change retvals to match retvals from OS_Socket.h

/**
 * Create a socket.
 *
 * @retval OS_SUCCESS                        Operation was successful.
 * @retval OS_ERROR_ABORTED                  If the Network Stack has
 *                                           experienced a fatal error.
 * @retval OS_ERROR_NOT_INITIALIZED          If the function was called before
 *                                           the Network Stack was fully
 *                                           initialized.
 * @retval OS_ERROR_INVALID_PARAMETER        If one of the passed parameters is
 *                                           NULL or the handle context is
 *                                           invalid.
 * @retval OS_ERROR_NETWORK_PROTO_NO_SUPPORT If the passed domain or type
 *                                           are unsupported.
 * @retval OS_ERROR_NETWORK_UNREACHABLE      If the network is unreachable.
 * @retval OS_ERROR_INSUFFICIENT_SPACE       If no free sockets could be
 *                                           found.
 *
 * @param[in]     domain  Domain of the socket that should be created.
 * @param[in]     type    Type of the socket that should be created.
 * @param[in,out] pHandle Handle that will be assigned to the created
 *                        socket.
 */
OS_Error_t
secureCommunication_rpc_socket_create(
    const int domain,
    const int type,
    int* const pHandle
)
{
    OS_Socket_Handle_t apiHandle = {.ctx = networkStackCtx, .handleID = *pHandle};
    OS_Error_t ret = OS_Socket_create(&networkStackCtx, &apiHandle, domain, type);
    memmove(pHandle, &apiHandle.handleID, sizeof(*pHandle));
    return ret;
}

/**
 * Close a socket.
 *
 * @retval OS_SUCCESS                 Operation was successful.
 * @retval OS_ERROR_ABORTED           If the Network Stack has experienced a
 *                                    fatal error.
 * @retval OS_ERROR_NOT_INITIALIZED   If the function was called before the
 *                                    Network Stack was fully initialized.
 * @retval OS_ERROR_INVALID_HANDLE    If an invalid handle was passed.
 * @retval OS_ERROR_INVALID_PARAMETER If the handle context is invalid.
 *
 * @param[in] handle Handle of the socket that should be closed.
 */
OS_Error_t
secureCommunication_rpc_socket_close(
    const int handle
)
{
    OS_Socket_Handle_t apiHandle = {.ctx = networkStackCtx, .handleID = handle};
    OS_Error_t ret = OS_Socket_close(apiHandle);
    return ret;
}

/**
 * Write data on a socket. This function checks if the socket is bound,
 * connected and that it isn't shutdown locally.
 *
 * @retval OS_SUCCESS                 Operation was successful.
 * @retval OS_ERROR_INVALID_HANDLE    If an invalid handle was passed.
 * @retval OS_ERROR_INVALID_PARAMETER If the requested length exceeds the
 *                                    dataport size.
 * @retval other                      Each component implementing this might
 *                                    have additional error codes.
 *
 * @param[in]     handle Handle of the socket to write on.
 * @param[in,out] pLen   Length of the data that should be written on the
 *                       socket. Will be overwritten by the function with
 *                       the actual amount that was written before it
 *                       returns.
 */
OS_Error_t
secureCommunication_rpc_socket_write(
    const int    handle,
    size_t* const pLen
)
{
    uint8_t *buf = secureCommunication_rpc_buf(secureCommunication_rpc_get_sender_id());
    OS_Socket_Handle_t apiHandle = {.ctx = networkStackCtx, .handleID = handle};

    OS_CryptoCipher_Handle_t hCipher;

    uint8_t enc_text[OS_DATAPORT_DEFAULT_SIZE];
    size_t enc_text_size = sizeof(enc_text);

    //TODO: get the key from the key exchange protocol instead
    // Generate a new AES key based on the spec provided above
    OS_CryptoKey_generate(&hKey, hCrypto, &aes_spec);

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
    OS_Error_t ret = OS_Socket_write(apiHandle, enc_text, enc_text_size, &sentLength);

    printf("%d of %d bytes sent succesfully.", sentLength, enc_text_size);
    memmove(pLen, &sentLength, sizeof(*pLen));

    return ret;
}

/**
 * Read data from a socket. This function checks whether or not the socket
 * is bound.
 *
 * @retval OS_SUCCESS                     Operation was successful.
 * @retval OS_ERROR_INVALID_HANDLE        If an invalid handle was passed.
 * @retval OS_ERROR_INVALID_PARAMETER     If the requested length exceeds
 *                                        the dataport size.
 * @retval OS_ERROR_NETWORK_CONN_SHUTDOWN If the connection got shut down.
 * @retval OS_ERROR_CONNECTION_CLOSED     If the connection got closed.
 * @retval other                          Each component implementing this
 *                                        might have additional error codes.
 *
 * @param[in]     handle Handle of the socket to read from.
 * @param[in,out] pLen   Length of the data that should be read from the
 *                       socket. Will be overwritten by the function with
 *                       the actual amount that was read before it returns.
 */
OS_Error_t
secureCommunication_rpc_socket_read(
    const int    handle,
    size_t* const pLen
)
{
    //return networkStack_rpc_socket_read(handle, pLen);
    return OS_ERROR_NOT_IMPLEMENTED;
}

/**
 * Connect a socket to a specified address.
 *
 * @retval OS_SUCCESS                 Operation was successful.
 * @retval OS_ERROR_INVALID_HANDLE    If an invalid handle was passed.
 * @retval OS_ERROR_INVALID_PARAMETER If the passed address is invalid.
 * @retval other                      Each component implementing this might
 *                                    have additional error codes.
 *
 * @param[in] handle  Handle of the socket to connect.
 * @param[in] dstAddr Address of the destination to connect to.
 */
OS_Error_t
secureCommunication_rpc_socket_connect(
    const int handle,
    const OS_Socket_Addr_t* const dstAddr
)
{
    OS_Socket_Handle_t apiHandle = {.ctx = networkStackCtx, .handleID = handle};
    OS_Error_t ret = OS_Socket_connect(apiHandle, dstAddr);
    return ret;
}

/**
 * Accept the next connection request on the queue of pending connections
 * for the listening socket.
 *
 * @retval OS_SUCCESS              Operation was successful.
 * @retval OS_ERROR_INVALID_HANDLE If an invalid handle was passed.
 * @retval other                   Each component implementing this might
 *                                 have additional error codes.
 *
 * @param[in]     handle        Handle of the listening socket.
 * @param[in,out] pHandleClient Handle that will be used to map the accepted
 *                              connection to.
 * @param[out]    srcAddr       Remote address of the incoming connection.
 */
OS_Error_t
secureCommunication_rpc_socket_accept(
    const int               handle,
    int* const              pHandleClient,
    OS_Socket_Addr_t* const srcAddr
)
{
    OS_Socket_Handle_t apiHandle = {.ctx = networkStackCtx, .handleID = handle};
    OS_Socket_Handle_t apiClientHandle = {.ctx = networkStackCtx, .handleID = *pHandleClient};

    OS_Error_t ret = OS_Socket_accept(apiHandle, &apiClientHandle, srcAddr);

    memmove(pHandleClient, &apiClientHandle.handleID, sizeof(*pHandleClient));

    return ret;
}

/**
 * Listen for connections on an opened and bound socket.
 *
 * @retval OS_SUCCESS              Operation was successful.
 * @retval OS_ERROR_INVALID_HANDLE If an invalid handle was passed.
 * @retval other                   Each component implementing this might
 *                                 have additional error codes.
 *
 * @param[in] handle  Handle of the socket to listen on.
 * @param[in] backlog Sets the maximum size to which the queue of pending
 *                    connections may grow.
 */
OS_Error_t
secureCommunication_rpc_socket_listen(
    const int handle,
    const int backlog
)
{   
    OS_Socket_Handle_t apiHandle = {.ctx = networkStackCtx, .handleID = handle};
    OS_Error_t ret = OS_Socket_listen(apiHandle, backlog);
    return ret;
}

/**
 * Bind a specified port to a socket.
 *
 * @retval OS_SUCCESS              Operation was successful.
 * @retval OS_ERROR_INVALID_HANDLE If an invalid handle was passed.
 * @retval other                   Each component implementing this might
 *                                 have additional error codes.
 *
 * @param[in] handle    Handle of the socket to bind.
 * @param[in] localAddr Address to bind the socket to.
 */
OS_Error_t
secureCommunication_rpc_socket_bind(
    const int              handle,
    const OS_Socket_Addr_t* const localAddr
)
{
    OS_Socket_Handle_t apiHandle = {.ctx = networkStackCtx, .handleID = handle};
    OS_Error_t ret = OS_Socket_bind(apiHandle, localAddr);
    return ret;
}

/**
 * Send data on a destination socket without checking if the destination is
 * connected or not and is therefore not connection-oriented.
 *
 * @retval OS_SUCCESS                 Operation was successful.
 * @retval OS_ERROR_INVALID_HANDLE    If an invalid handle was passed.
 * @retval OS_ERROR_INVALID_PARAMETER If the requested length exceeds the
 *                                    dataport size.
 * @retval other                      Each component implementing this might
 *                                    have additional error codes.
 *
 * @param[in]     handle  Handle to a previously created socket.
 * @param[in,out] pLen    Length of the data that should be written on the
 *                        destination socket. Will be overwritten by the
 *                        function with the actual amount that was written
 *                        before it returns.
 * @param[in]     dstAddr Address of the destination socket to send data on.
 */
OS_Error_t
secureCommunication_rpc_socket_sendto(
    const int              handle,
    size_t* const           pLen,
    const OS_Socket_Addr_t* const dstAddr
)
{
    //return networkStack_rpc_socket_sendto(handl, pLen, dstAddr);
    return OS_ERROR_NOT_IMPLEMENTED;
}

/**
 * Receive data from a specified socket. This operation checks if the socket
 * is bound but not if it is connected and is therefore not
 * connection-oriented.
 *
 * @retval OS_SUCCESS                 Operation was successful.
 * @retval OS_ERROR_INVALID_HANDLE    If an invalid handle was passed.
 * @retval OS_ERROR_INVALID_PARAMETER If the requested length exceeds
 *                                    the dataport size.
 * @retval other                      Each component implementing this
 *                                    might have additional error codes.
 *
 * @param[in]     handle  Handle to a previously created and bound socket.
 * @param[in,out] pLen    Length of the data that should be read from the
 *                        source socket. Will be overwritten by the
 *                        function with the actual amount that was read
 *                        before it returns.
 * @param[out]    srcAddr Address of the source socket that data was
 *                        received from.
 */
OS_Error_t
secureCommunication_rpc_socket_recvfrom(
    int                 handle,
    size_t*              pLen,
    OS_Socket_Addr_t*    srcAddr
)
{
    OS_Socket_Handle_t apiHandle = {.ctx = networkStackCtx, .handleID = handle};
    uint8_t buf[OS_DATAPORT_DEFAULT_SIZE];
    uint8_t dec_text[OS_DATAPORT_DEFAULT_SIZE];
    size_t actualLen;
    

    OS_Error_t ret = OS_Socket_recvfrom(apiHandle, buf, *pLen, &actualLen, srcAddr);

    printf("%d of %d bytes read.", actualLen, *pLen);
    printf("Received encrypted string: %s", buf);

    printf("Decrypting String.\n");

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

    memmove(pLen, &actualLen, sizeof(*pLen));
    return ret;
}

/**
 * Query the current state of the Network Stack component.
 *
     * @retval UNINITIALIZED Network Stack is uninitialized.
     * @retval INITIALIZED   Network Stack is initialized.
     * @retval RUNNING       Network Stack is running.
     * @retval FATAL_ERROR   Network Stack has experienced a fatal error.
 */
OS_NetworkStack_State_t
secureCommunication_rpc_socket_getStatus(
    void)
{
    return OS_Socket_getStatus(&networkStackCtx);
}

/**
 * Get all pending events for the client's sockets.
 *
 * @retval OS_SUCCESS                 Operation was successful.
 * @retval OS_ERROR_INVALID_PARAMETER If an empty pointer was passed.
 * @retval other                      Each component implementing this
 *                                    might have additional error codes.
 *
 * @param[in]  bufSize         Size of the caller buffer.
 * @param[out] pNumberOfEvents Will be overwritten with the number of
 *                             events.
 */
OS_Error_t
secureCommunication_rpc_socket_getPendingEvents(
    size_t  bufSize,
    int*    pNumberOfEvents
)
{
    uint8_t *buf = secureCommunication_rpc_buf(secureCommunication_rpc_get_sender_id());
    OS_Error_t ret = OS_Socket_getPendingEvents(&networkStackCtx, buf, bufSize, pNumberOfEvents);
    return ret;
}





//----------------------------------------------------------------------
// Other functions
//----------------------------------------------------------------------
/*
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
*/