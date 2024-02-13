# TestApp component

This component is used to process the sensor data from the vehicle and send appropriate commands to park the vehicle.



# How parking space is detected

Whenever we have a detection, if the detection is not (0,0,0), which signifies we didnt detect anything, we save the coordinate to lastDetectedVehicle variable. We compare the next detection with lastDetectedVehicle. If the distance is more than 4 meters, it means its a new car and we update the lastDetectedVehicle and keep processing the sent data. If the distance is more than 12 meters, we send command to stop vehicle and stop processing data. We dont receive anymore any data and just send the commands for parking procedure. Afterwards, the TestApp application can exit, since its job is done. The bridge  will continue applying the commands and it will also exit when  it finishes all the commands and it will clean up everything.


# Parking

The parking procedure is fixed. The times were calculated by trial and error and they work as intended more or the less. 
