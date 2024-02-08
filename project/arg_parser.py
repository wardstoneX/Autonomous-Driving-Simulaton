import argparse

def parse_arguments():
    parser = argparse.ArgumentParser(
    usage="%(prog)s [OPTION] [FILE]...",
    description="Print or check SHA1 (160-bit) checksums."
    )
    #"10.149.55.108"  6061
    parser.add_argument('-hserv', '--hostserver', type=str, default='10.0.0.10', help='Hostname (default: localhost)')
    #parser.add_argument('--host_server', type=str, help='The host server')
    parser.add_argument('-p', '--port', type=int, default=6000, help='Port number (default: 6000)')
    parser.add_argument('-hsim', '--hostsimulator', type=str, default='localhost', help='Simulator Host (default: localhost)')
    parser.add_argument('-ps', '--portsimulator', type=int, default=2000, help='Simulator Port (default: 2000)')
    parser.add_argument('-m', '--map', type=str, default='Town06', help='Simulator Map (default: Town06)')
    parser.add_argument('-s', '--scenario', type=str, default='default_scenario.txt', help='Simulator scenario (default: default_scenaro.txt)')

    return parser.parse_args()