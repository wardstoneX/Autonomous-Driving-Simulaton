

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
    // Send the byte stream
    size_t n;
    OS_Socket_write(sock, &data, sizeof(data), &n);
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

//------------------------------------------------------------------------------
