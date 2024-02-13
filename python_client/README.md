## Arguments for the `bridge.py`

The program accepts the following command-line arguments:

- `-hserv, --hostserver`: Hostname for the server (default: '10.0.0.10')
- `-pserv, --portserver`: Port number for the server (default: 6000)
- `-hsim, --hostsimulator`: Simulator host (default: 'localhost')
- `-psim, --portsimulator`: Simulator port (default: 2000)
- `-m, --map`: Simulator map (default: 'Town06')
- `-s, --scenario`: Simulator scenario file (default: 'default_scenario.txt')

### Example Usage

```sh
python3.7 bridge.py -hserv 10.0.0.20 -pserv 7000 -hsim localhost -psim 3000 -m Town07 -s custom_scenario.txt
```

```sh
python3.7 bridge.py  -s custom_scenario.txt
```

Beware, only the map `Town06` is supported currently! Also change the server adress in system_config if you change the host server adress.

## Scenario files


<p align="center">
  <img width="460" height="600" src="pictures/layout_explanation.jpeg">
  <br>
  <i>Layout Illustration</i>
</p>



If you want to create a new scenario, plase create a text file in folder `pythonClient/scenarios`, in which you need to add some numbers in the following format x<sub>0</sub>-x<sub>1</sub>-...-x<sub>n</sub> where each x<sub>i</sub> is the offset of a vehicle on the side of the road from a base point. The minimum allowed distance between x<sub>i</sub> and x<sub>i-1</sub> is 5. Preferable, please start with x<sub>0</sub> 0.

 Since the length of the used vehicle is around 5 meters and a parking spot should have the minimum length of 1.5 * VehicleLength, please make sure to type in the scenario file the minimum distance of 12  for the parking spot.

# Note:
I realized i dont really need the gps sensor. However the way it could have been used is, in the radar callback `location_ = radar_data.transform.location + fw_vec`, i could get the location from the gps and add the forward vector to it. However, since its the last date, i will refrain from it. This could have been done while pairing the gps data and radar data before sending it,i could have added the forward vector to the gps location....
