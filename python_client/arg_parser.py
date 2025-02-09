import argparse

def parse_arguments():
    """
    Parse the arguments given to the program. The arguments are: 
    -hserv, --hostserver: Hostname (default: localhost) 
    -pserv, --portserver: Port number (default: 6000)
    -hsim, --hostsimulator: Simulator Host (default: localhost)
    -psim, --portsimulator: Simulator Port (default: 2000)
    -m, --map: Simulator Map (default: Town06)
    -s, --scenario: Simulator scenario (default: default_scenario.txt)
    """
    parser = argparse.ArgumentParser(
    usage="%(prog)s [OPTION] [FILE]...",
    description="This is a node between the CARLA simulator and the Raspberry Pi. See project readme for more information.")
    parser.add_argument('-hserv', '--hostserver', type=str, default='10.0.0.10', help='Hostname (default: localhost)')
    parser.add_argument('-pserv', '--portserver', type=int, default=6000, help='Port number (default: 6000)')
    parser.add_argument('-hsim', '--hostsimulator', type=str, default='localhost', help='Simulator Host (default: localhost)')
    parser.add_argument('-psim', '--portsimulator', type=int, default=2000, help='Simulator Port (default: 2000)')
    parser.add_argument('-m', '--map', type=str, default='Town06', help='Simulator Map (default: Town06)')
    parser.add_argument('-s', '--scenario', type=str, default='default_scenario.txt', help='Simulator scenario (default: default_scenaro.txt)')

    return parser.parse_args()