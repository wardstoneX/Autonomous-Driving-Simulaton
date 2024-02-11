

#include "utils.h"


/*
This method is used to check if the given tuple is a zero tuple.
The only important values are x and y, as z is not used in the current implementation.
*/
bool isZeroTuple(struct Tuple tuple) {
    return tuple.x == 0.0 && tuple.y == 0.0; //&& tuple.z == 0.0;
}

/*
This method calculates the euclidean distance between two coordinates.
*/
float calculateEuclideanDistance(struct Tuple* tuple1, struct Tuple*tuple2) {
    float dx = tuple2->x - tuple1->x;
    float dy = tuple2->y - tuple1->y;
    float dz = tuple2->z - tuple1->z;

    return sqrt(dx * dx + dy * dy + dz * dz);
}

/*
This method sends the control data to the server.

*/
void send_control_data(OS_Socket_Handle_t sock, struct ControlData data) {
    // Send the byte stream
    size_t n;
    OS_Socket_write(sock, &data, sizeof(data), &n);
}

/*
This method is used to create a control data struct with given parameters and send it to the server.

*/
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

//------------------------------------------------------------------------------
