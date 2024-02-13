#!/bin/env python

import asyncio
import socket
from argparse import ArgumentParser

from cli import GameCli
from client import GameClient
from debug import debug, set_debug_output


def make_debug(verbose: bool):
    def debug(*debug_args, **debug_kwargs):
        print(*debug_args, **debug_kwargs)

    def no_debug(*debug_args, **debug_kwargs):
        pass

    return debug if verbose else no_debug


async def main():
    parser = ArgumentParser(description='Talk to the beast')

    host_help = 'IP address or domain name of the server'
    parser.add_argument('-o', '--host', help=host_help,
                        default='localhost')

    port_help = 'Port of the server'
    parser.add_argument('-p', '--port', help=port_help,
                        default=10000, type=int)

    verbose_help = 'Enable verbose output'
    parser.add_argument('-v', '--verbose', help=verbose_help,
                        action='store_true')

    args = parser.parse_args()

    if args.verbose:
        set_debug_output(True)

    addrinfos = socket.getaddrinfo(args.host, args.port, type=socket.SOCK_STREAM)

    connected = False
    for addrinfo in addrinfos:
        family, addr_type, proto, _, address = addrinfo
        debug('Main', "Trying", family, addr_type, proto, address)

        try:
            sock = socket.socket(family, addr_type, proto)
            sock.connect(address)

            connected = True
            break
        except Exception as e:
            debug('Main', 'Error', e)
            continue

    if not connected:
        print(f"Could not connect to {args.host}:{args.port}")
        return

    sock.setblocking(False)
    game = GameClient(sock)
    cli = GameCli(game, args.host)

    try:
        await cli.start()
        await cli.loop()
    except (Exception, KeyboardInterrupt):
        try:
            socket.close()
        except Exception:
            pass

        raise


if __name__ == '__main__':
    asyncio.run(main())
