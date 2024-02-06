import argparse

def parse_arguments():
    parser = argparse.ArgumentParser(
    usage="%(prog)s [OPTION] [FILE]...",
    description="Print or check SHA1 (160-bit) checksums."
    )
    #"10.149.55.108"  6061
    parser.add_argument('-h', '--host', type=str, default='localhost', help='Hostname (default: localhost)')
    parser.add_argument('-p', '--port', type=int, default=8080, help='Port number (default: 8080)')
    parser.add_argument('-hs', '--host-simulator', type=str, default='localhost', help='Simulator Host (default: localhost)')
    parser.add_argument('-ps', '--port-simulator', type=int, default=2000, help='Simulator Port (default: 2000)')
    parser.add_argument('-m', '--map', type=str, default='Town06', help='Simulator Map (default: Town06)')

    return parser.parse_args()