
#ifndef UTILS_H
#define UTILS_H

#include <stdio.h>
#include <stdbool.h>
#include <math.h>
#include <string.h>
#include <arpa/inet.h>
#include "interfaces/if_OS_Socket.h"
#include "OS_Socket.h"
#include <camkes.h>

#include "OS_Socket.h"
#include "interfaces/if_OS_Socket.h"

#include "lib_debug/Debug.h"
#include <string.h>

#include <camkes.h>


// Define constants
#define PORT 6061

// Structures
struct ControlData {
    float throttle;
    float steer;
    float brake;
    int reverse;
    float time;
};

struct Tuple {
    float x;
    float y;
    float z;
};

struct PairOfTuples {
    struct Tuple mainVehiclePosition;
    struct Tuple OtherVehiclePosition;
};



struct PairOfTuples lastDetectedVehicle;

bool isZeroTuple(struct Tuple tuple);
float calculateEuclideanDistance(struct Tuple* tuple1, struct Tuple*tuple2);
double predict_power(double x);
struct Tuple calculateMidpoint(struct Tuple *point1, struct Tuple *point2);
void send_control_data(OS_Socket_Handle_t sock, struct ControlData data);
void send_parameters(OS_Socket_Handle_t sock, float throttle, float steer, float brake, int reverse, float time);

const if_OS_Socket_t networkStackCtx;
OS_Error_t waitForNetworkStackInit(const if_OS_Socket_t* const ctx);

#endif // UTILS_H
