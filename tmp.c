// Server side C program to demonstrate Socket
// programming
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <tgmath.h>
#include "c-vector/cvector.h"

#include "scv/scv.h"
#include "scv/scv.c"
#define PORT 6061
#include <stdbool.h>

struct ControlData {
    float throttle;
    float steer;
    float brake;
    bool reverse;
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

int socket = 0;
struct PairOfTuples lastDetectedVehicle = {{0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}};
struct Tuple midpoint;
bool parkingSpotDetected = false;

struct scv_vector *vector;

struct scv_vector *vector_radar;

ssize_t leftover_size = 0;
char leftover[1024];
bool isZeroTuple(struct Tuple tuple) {
    return tuple.x == 0.0 && tuple.y == 0.0; //&& tuple.z == 0.0;
}

float calculateEuclideanDistance(struct Tuple* tuple1, struct Tuple*tuple2) {
    float dx = tuple2->x - tuple1->x;
    float dy = tuple2->y - tuple1->y;
    float dz = tuple2->z - tuple1->z;

    return sqrt(dx * dx + dy * dy + dz * dz);
}

struct Tuple calculateMidpoint(struct Tuple *point1, struct Tuple *point2) {
    struct Tuple midpoint;
    midpoint.x = (point1->x + point2->x) / 2;
    midpoint.y = (point1->y + point2->y) / 2;
    midpoint.z = (point1->z + point2->z) / 2;
    return midpoint;
}

void park() {

}

void checkForParking() {
    uint16_t t = 0;
    while(1) {
        ssize_t size = vector->size;
        ssize_t size_radar = vector_radar->size;
        if (size == 0 || size_radar == 0) {
            return;
        }
        struct Tuple *mainGNSSposition = (struct Tuple *) scv_front(vector);
        struct Tuple *radarDetection = (struct Tuple *) scv_front(vector_radar);

        //printf("radar detection:  (%f, %f, %f)\n", radarDetection->x, radarDetection->y, radarDetection->z);

        scv_pop_front(vector);
        scv_pop_front(vector_radar);

        if (isZeroTuple(*radarDetection)) {
            continue;
        }

        if (isZeroTuple(lastDetectedVehicle.mainVehiclePosition) && isZeroTuple(lastDetectedVehicle.OtherVehiclePosition)) {
            lastDetectedVehicle.OtherVehiclePosition = *radarDetection;
            lastDetectedVehicle.mainVehiclePosition = *mainGNSSposition;
            printf("main vehicle at position (%f, %f, %f) detected other vehicle at position (%f, %f, %f)\n",mainGNSSposition->x, mainGNSSposition->y, mainGNSSposition->z, radarDetection->x, radarDetection->y, radarDetection->z);
            continue;
        }

        float distance = calculateEuclideanDistance(&lastDetectedVehicle.OtherVehiclePosition, radarDetection);
        printf("Distance: %f\n", distance);
        if (distance > 4.0) {
            if (distance > 12) {
                printf("Park spot found\n");
                send_parameters(socket, 0.0, 0.0, 1.0, false);

                // calculate the middle point of two detections here
                midpoint = calculateMidpoint(&lastDetectedVehicle.mainVehiclePosition, &lastDetectedVehicle.OtherVehiclePosition);
                printf("Midpoint: (%f, %f, %f)\n", midpoint.x, midpoint.y, midpoint.z);
                parkingSpotDetected = true;
                return;
            }
            lastDetectedVehicle.OtherVehiclePosition = *radarDetection;
            lastDetectedVehicle.mainVehiclePosition = *mainGNSSposition;
            printf("New vehicle detected!!!!!!! main vehicle at position (%f, %f, %f) detected other vehicle at position (%f, %f, %f)\n",mainGNSSposition->x, mainGNSSposition->y, mainGNSSposition->z, radarDetection->x, radarDetection->y, radarDetection->z);

            continue;
        }

    }
}


void process_buffer(char* buffer, ssize_t size) {
    int start = 0;
    while (start < size) {
        // Check if there's enough data to parse the header
        if (size - start < 5) {
            break; // Wait for the rest of the header to arrive
        }

        // Check the type of data
        if (buffer[start] == '\x13') {
            // Parse the size of the data
            int data_size;
            memcpy(&data_size, buffer + start + 1, sizeof(int));
            data_size = ntohl(data_size); // Convert from network to host byte order

            // Check if the complete message has been received
            if (start + 5 + data_size > size) {
                break; // Wait for the rest of the message to arrive
            }

            // Check for the end of the data pair
            if (buffer[start + 5 + data_size] != '\x14') {
                fprintf(stderr, "Error: Expected '\\x14' at the end of the data pair, but found '\\x%x'\n", buffer[start + 5 + data_size]);
                break; // Wait for the rest of the message to arrive
            }

            char* data = malloc(data_size + 1);
            memcpy(data, buffer + start + 5, data_size);
            data[data_size] = '\0'; // Null-terminate the string

            struct Tuple radar, gnss;
            sscanf(data, "%f,%f,%f-%f,%f,%f", &radar.x, &radar.y, &radar.z, &gnss.x, &gnss.y, &gnss.z);
            //printf("Radar: (%f, %f, %f), GNSS: (%f, %f, %f)\n", radar.x, radar.y, radar.z, gnss.x, gnss.y, gnss.z);

            scv_push_back(vector_radar, &radar);
            scv_push_back(vector, &gnss);

            start += 5 + data_size + 1; // Move to the start of the next message

            free(data);
        } else {
            // Handle other types of data
            start++;
        }
    }

    if(!parkingSpotDetected) {
        checkForParking();
    } else {
        park();
    }
    

    // Update the leftover data
    if (start < size) {
        memmove(buffer, buffer + start, size - start);
        leftover_size = size - start; // Update the size of the leftover data
    } else {
        leftover_size = 0; // No leftover data
    }
}

void send_parameters(int socket, float throttle, float steer, float brake, bool reverse) {
    struct ControlData data;
    data.throttle = throttle;
    data.steer = steer;
    data.brake = brake;
    data.reverse = reverse;
    send_control_data(socket, data);
}
void send_control_data(int socket, struct ControlData data) {
    // Serialize the struct into a byte stream
    char buffer[sizeof(struct ControlData)];
    memcpy(buffer, &data, sizeof(struct ControlData));

    // Send the byte stream
    send(socket, buffer, sizeof(buffer), 0);
}

int main(int argc, char const* argv[])
{
    vector = scv_new(sizeof(struct Tuple), 1000);
    vector_radar = scv_new(sizeof(struct Tuple), 1000);


    int server_fd, new_socket;
    ssize_t valread;
    struct sockaddr_in address;
    int opt = 1;
    socklen_t addrlen = sizeof(address);
    char buffer[4096] = { 0 };
    char* hello = "Hello from server";

    // Creating socket file descriptor
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    // Forcefully attaching socket to the port 8080
    if (setsockopt(server_fd, SOL_SOCKET,
                   SO_REUSEADDR , &opt,
                   sizeof(opt))) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }
    if (setsockopt(server_fd, SOL_SOCKET,
                   SO_REUSEPORT, &opt,
                   sizeof(opt))) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    // Forcefully attaching socket to the port 8080
    if (bind(server_fd, (struct sockaddr*)&address,
             sizeof(address))
        < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }
    if (listen(server_fd, 3) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }
    if ((new_socket
                 = accept(server_fd, (struct sockaddr*)&address,
                          &addrlen))
        < 0) {
        perror("accept");
        exit(EXIT_FAILURE);
    }
    socket = new_socket;
    printf("start reading\n");
    bool stop_sent = false;  // Add this line at the beginning of your function


    // make the car go forward
    send_parameters(socket, 0.2, 0.0, 0.0, false);


    while (1) {
        valread = read(new_socket, buffer, sizeof(buffer) - 1);
        if (valread < 0) {
            perror("read");
            break;
        } else if (valread == 0) {
            printf("Client disconnected\n");
            break;
        } else {
            char temp[sizeof(buffer) + leftover_size];
            if (leftover_size > 0) {
                memcpy(temp, leftover, leftover_size);
                memcpy(temp + leftover_size, buffer, valread);
            } else {
                memcpy(temp, buffer, valread);
            }
            process_buffer(temp, valread + leftover_size);

        }
    }





    send(new_socket, hello, strlen(hello), 0);
    printf("Hello message sent\n");

    // closing the connected socket
    close(new_socket);
    // closing the listening socket
    close(server_fd);
    scv_delete(vector);
    return 0;
}
