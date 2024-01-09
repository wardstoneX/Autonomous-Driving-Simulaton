#include "OS_Socket.h"
#include "OS_Dataport.h"
#include "interfaces/if_OS_Socket.h"

#include "lib_debug/Debug.h"
#include "lib_macros/Check.h"

#include <string.h>

#include "OS_Crypto.h"
#include <camkes.h>


//------------------------------------------------------------------------------

/*
static const OS_Crypto_Config_t cryptoCfg =
{
    .mode = OS_Crypto_MODE_LIBRARY,
    .entropy = IF_OS_ENTROPY_ASSIGN(
        entropy_rpc,
        entropy_dp),
};
*/



//------------------------------------------------------------------------------



/**
 * Create a socket.
 *
 * @retval OS_SUCCESS                        Operation was successful.
 * @retval OS_ERROR_NETWORK_PROTO_NO_SUPPORT If the passed domain or type
 *                                           are unsupported.
 * @retval OS_ERROR_INSUFFICIENT_SPACE       If no free sockets could be
 *                                           found.
 * @retval other                             Each component implementing
 *                                           this might have additional
 *                                           error codes.
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
    //return networkStack_rpc_socket_create(domain, type, pHandle);
    return OS_ERROR_NOT_IMPLEMENTED;
}

/**
 * Close a socket.
 *
 * @retval OS_SUCCESS              Operation was successful.
 * @retval OS_ERROR_INVALID_HANDLE If an invalid handle was passed.
 * @retval other                   Each component implementing this might
 *                                 have additional error codes.
 *
 * @param[in] handle Handle of the socket that should be closed.
 */
OS_Error_t
secure_Communication_socket_close(
    const int handle
)
{
    //return networkStack_rpc_socket_close(handle);
    return OS_ERROR_NOT_IMPLEMENTED;
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
secure_Communication_socket_write(
    const int    handle,
    size_t* const pLen
)
{
    //TODO: encrypt the message before forwarding to the network stack

    //return NetworkStack_PicoTcp_networkStack_socket_write(handle, pLen);
    return OS_ERROR_NOT_IMPLEMENTED;
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
secure_Communication_socket_connect(
    const int handle,
    const OS_Socket_Addr_t* const dstAddr
)
{
    //return networkStack_rpc_socket_connect(handle, dstAddr);
    return OS_ERROR_NOT_IMPLEMENTED;
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
    //return networkStack_rpc_socket_accept(handle, pHandleClient, srcAddr);
    return OS_ERROR_NOT_IMPLEMENTED;
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
    //return networkStack_rpc_(handle, backlog);
    return OS_ERROR_NOT_IMPLEMENTED;
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
secure_Communication_socket_bind(
    const int              handle,
    const OS_Socket_Addr_t* const localAddr
)
{
    //return networkStack_rpc_socket_bind(handle, localAddr);
    return OS_ERROR_NOT_IMPLEMENTED;
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
    //TODO: decrypt the message before from the networkstack before returning it
    return OS_ERROR_NOT_IMPLEMENTED;
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
secure_Communication_socket_getStatus(
    void)
{
    //return networkStack_rpc_socket_getStatus();
    return OS_ERROR_NOT_IMPLEMENTED;
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
    int*     pNumberOfEvents
)
{
    //return networkStack_rpc_socket_getPendingEvents(bufSize, pNumberOfEvents);
    return OS_ERROR_NOT_IMPLEMENTED;
}
