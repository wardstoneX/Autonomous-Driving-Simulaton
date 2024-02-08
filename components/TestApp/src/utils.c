

#include "utils.h"



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

void send_control_data(OS_Socket_Handle_t sock, struct ControlData data) {
    // Serialize the struct into a byte stream
    char buffer[sizeof(struct ControlData)];
    memcpy(buffer, &data, sizeof(struct ControlData));

    // Send the byte stream
     size_t n;
    OS_Socket_write(sock, buffer, sizeof(buffer), &n);
}


void send_parameters(OS_Socket_Handle_t sock, float throttle, float steer, float brake, int reverse, float time) {
    struct ControlData data;
    data.throttle = throttle;
    data.steer = steer;
    data.brake = brake;
    data.reverse = reverse;
    data.time = time;

    send_control_data(sock, data);
}

//----------------------------------------------------------------------
// Network
//----------------------------------------------------------------------

//------------------------------------------------------------------------------

 const if_OS_Socket_t networkStackCtx =
    IF_OS_SOCKET_ASSIGN(secureCommunication);

 OS_Error_t waitForNetworkStackInit(const if_OS_Socket_t* const ctx)
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


 OS_Error_t
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