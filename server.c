// Server side C program to demonstrate Socket
// programming
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <tgmath.h>
#include <arpa/inet.h>

#include "include/scv/scv.h"
#include "include/scv/scv.c"

#include <stdbool.h>

#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 6000

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

int sock = 0;
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

void send_control_data(int socket, struct ControlData data) {
    // Serialize the struct into a byte stream
    char buffer[sizeof(struct ControlData)];
    memcpy(buffer, &data, sizeof(struct ControlData));

    // Send the byte stream
    send(socket, buffer, sizeof(buffer), 0);
}
void send_parameters(int sock, float throttle, float steer, float brake, int reverse, float time) {
    struct ControlData data;
    data.throttle = throttle;
    data.steer = steer;
    data.brake = brake;
    data.reverse = reverse;
    data.time = time;

    send_control_data(sock, data);
}

void park() {
    printf("Parking initiated\n");
    // lane change
    send_parameters(sock,0.4, 0.3, 0,0, 3.35);
    send_parameters(sock,0.4, -0.3, 0,0, 1.65);
    send_parameters(sock,0.0, 0.0, 1.0,0, 2);
    double dist = lastDetectedVehicle.mainVehiclePosition.x - midpoint.x;
    double time = predict_power(dist);
    printf("Distance: %f, Time: %f\n", dist, time);
    send_parameters(sock,0.4, 0.0, 0.0,1, time - 5.25);
    // initiate the parking process
    send_parameters(sock,0.0, 0.0, 1.0,0, 2);

    send_parameters(sock,0.3, 0.6, 0.0,1, 3.75);

    send_parameters(sock,0.2, -0.6, 0.0,1, 2.25);

    send_parameters(sock,0.0, 0.0, 1.0,0, 5);



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
                send_parameters(sock, 0.0, 0.0, 1.0, 0, 3);

                // calculate the middle point of two detections here
                midpoint = calculateMidpoint(&lastDetectedVehicle.mainVehiclePosition, &lastDetectedVehicle.OtherVehiclePosition);
                printf("Midpoint: (%f, %f, %f)\n", midpoint.x, midpoint.y, midpoint.z);
                parkingSpotDetected = true;
                lastDetectedVehicle.OtherVehiclePosition = *radarDetection;
                lastDetectedVehicle.mainVehiclePosition = *mainGNSSposition;
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
            //printf(" Radar: (%f, %f, %f) GNSS: (%f, %f, %f)\n",radar.x,radar.y, radar.z, gnss.x, gnss.y, gnss.z);

            scv_push_back(vector_radar, &radar);
            scv_push_back(vector, &gnss);

            start += 5 + data_size + 1; // Move to the start of the next message

            free(data);
        } else {
            // Handle other types of data
            start++;
        }
    }

    
        checkForParking();



    // Update the leftover data
    if (start < size) {
        memmove(buffer, buffer + start, size - start);
        leftover_size = size - start; // Update the size of the leftover data
    } else {
        leftover_size = 0; // No leftover data
    }
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

   
    struct sockaddr_in server_address;
    
    // Create socket
    new_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (new_socket == -1) {
        perror("Could not create socket");
        exit(EXIT_FAILURE);
    }

    // Specify server address
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(SERVER_PORT);
    if (inet_pton(AF_INET, SERVER_IP, &server_address.sin_addr) <= 0) {
        perror("Invalid address or address not supported");
        exit(EXIT_FAILURE);
    }

    // Connect to server
    if (connect(new_socket, (struct sockaddr *)&server_address, sizeof(server_address)) < 0) {
        perror("Connection failed");
        exit(EXIT_FAILURE);
    }

    sock = new_socket;
    printf("start reading\n");
    bool stop_sent = false;  // Add this line at the beginning of your function


    // make the car go forward
        printf("initial control data sent\n");

    send_parameters(new_socket, 0.2, 0.0, 0.0, 0,0);


    while (!parkingSpotDetected) {
        valread = read(new_socket, buffer, sizeof(buffer) - 1);
        printf("valread: %ld\n", valread);
        if (valread > 0)  {
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
    printf("end reading\n"  );

    // initiating the parking process

    park();



   
    send_parameters(sock,0.0, 0.0, 0.0,0, 0);
    printf("Parking completed\n Command to end simulation sent\n");

    


   
    // closing the connected socket
    close(new_socket);
    // closing the listening socket

    scv_delete(vector);
    scv_delete(vector_radar);
    return 0;
}
