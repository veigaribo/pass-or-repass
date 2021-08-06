#!/bin/env python

from argparse import ArgumentParser
import socket

## Protocol

# Common
ACK  = b'\xff'
DENY = b'\xfe'

# Client -> Server
JOIN_QUEUE  = b'\x01'
QUIT_QUEUE  = b'\x02' # not implemented
RESUME_GAME = b'\x05'
ASK_AGAIN   = b'\x06'

ANSWER_A    = b'\x10'
ANSWER_B    = b'\x11'
ANSWER_C    = b'\x12'

PASS        = b'\x1f'
QUIT_GAME   = b'\x1e'

# Server -> Client
GAME_FOUND      = b'\x81'

# Includes two null terminated strings:
# Match ID and player ID
# Player ID will be either 0x01 or 0x02,
# for player 1 and 2 respectively
JOINED_GAME     = b'\x82'

# Includes one null terminated string:
# question description
ASK_TO_01       = b'\x83'
ASK_TO_02       = b'\x84'
PASS_TO_01      = b'\x85'
PASS_TO_02      = b'\x86'

# Includes one null terminated strings:
# The player's score
SCORE_01        = b'\x87'
SCORE_02        = b'\x88'

GAME_END        = b'\x89'

GAME_ABORTED    = b'\x8a'

## Main logic

# States
STATE_HOME           = 0
STATE_QUEUE          = 1
STATE_AFK_CHECK      = 2
STATE_GAME_ANSWERING = 3

class DeniedEvent(Exception):
    pass

class GameClient:
    def __init__(self, socket):
        self.socket = socket
        self.match_id = None
        self.player_id = None
        self.my_score = 0
        self.other_score = 0

        self.state = STATE_HOME

    def is_in_state(self, state):
        return self.state == state

    def _send(self, message):
        self.socket.sendall(message)

    def _await_ack(self):
        event = self.socket.recv(1)

        if event == DENY:
            self._denied()
        
        elif event != ACK:
            self._panic(event)

    def _await_game(self):
        event = self.socket.recv(1)

        if event != GAME_FOUND:
            self._panic(event)
            return

        self.state = STATE_AFK_CHECK

    def _await_joined_game(self):
        event = self.socket.recv(1)

        if event != JOINED_GAME:
            self._panic(event)
            return

        params = self._read_params(2)
        self.match_id = params[0].decode('ascii')
        self.player_id = params[1][0]

    def _await_question(self):
        print('awaiting the first')
        event = self.socket.recv(1)
        print('got the first', event)

        # Game could end here as well
        if not event in [ASK_TO_01, ASK_TO_02, GAME_END]:
            self._panic(event)
            return

        if event == GAME_END:
            print("")
            print("Game ended. Your score", self.my_score)
            print("Opponent's score", self.other_score)
            print("")
            
            if self.my_score > self.other_score:
                print("You won.")
            elif self.my_score < self.other_score:
                print("You lose.")
            else:
                print("Draw.")

            print("")

            self.disconnect()
            return

        question = self._read_params(1)[0].decode('ascii')
        is_question_to_me = \
                event == ASK_TO_01 and self.player_id == 1 or \
                event == ASK_TO_02 and self.player_id == 2
        
        if is_question_to_me:
            print("")
            print("Answer the following question:")
            print("")
            print(question)
            print("")

            self.state = STATE_GAME_ANSWERING
        else:
            print("")
            print("Question to the opponent player:")
            print("")
            print(question)
            print("")
            self._await_score()
            self._send(ACK)
            print("* 01")
            self._await_question()

    def _await_score(self):
        event = self.socket.recv(1)

        if not event in [SCORE_01, SCORE_02, DENY]:
            self._panic(event)
            return

        if event == DENY:
            raise DeniedEvent()

        # Score comes +1 to avoid 0
        score = self._read_params(1)[0][0] - 1
        is_score_to_me = \
                event == SCORE_01 and self.player_id == 1 or \
                event == SCORE_02 and self.player_id == 2

        if is_score_to_me:
            self.my_score = score
            print("Your score", score)
            print("Opponent's score", self.other_score)
        else:
            print("Opponent's score", score)
            print("Your score", self.my_score)
            self.other_score = score

    def _read_params(self, amount):
        raw_params = b''
        params = []
        params_got = 0

        while(params_got < amount):
            while(len(raw_params) == 0 or raw_params[-1] != 0):
                raw_params += self.socket.recv(1024)
                print("* Read params", raw_params)

            split = raw_params.split(b'\x00')

            for param in split[0:-1]:
                params.append(param)
                params_got += 1

        return params

    def join_queue(self):
        print("* Asking to join MM queue")
        self._send(JOIN_QUEUE)
        self._await_ack()

        # will block so you can't quit without terminating
        # the process / thread
        print("* Awaiting game")
        self._await_game()

    # useless
    def quit_queue(self):
        self._send(QUIT_QUEUE)
        self._await_ack()

    def accept_game(self):
        self._send(ACK)
        self._await_joined_game()
        self._send(ACK)
        self._await_question()

    def deny_game(self):
        self._send(DENY)
        self.state = STATE_HOME

    def quit_game(self):
        print("Exiting")
        self.socket.sendall(QUIT_GAME)

    def answer_a(self):
        self._send(ANSWER_A)
        self._await_score()
        self._send(ACK)
        print("* 11")
        self._await_question()

    def answer_b(self):
        self._send(ANSWER_B)
        self._await_score()
        self._send(ACK)
        print("* 12")
        self._await_question()

    def answer_c(self):
        self._send(ANSWER_C)
        self._await_score()
        self._send(ACK)
        print("* 13")
        self._await_question()

    def pass_question(self):
        self._send(PASS)

        try:
            self._await_score()
        except DeniedEvent:
            print("Server denied. Can't rerepass")
            return

        self._send(ACK)
        self._await_question()

    def disconnect(self):
        if self.match_id:
            print(f"Match ID: {self.match_id}")
            self.quit_game()

        self.socket.shutdown(socket.SHUT_RDWR)
        self.socket.close()

    def _denied(self):
        raise DeniedEvent()

    def _panic(self, event):
        print("Shit happened: received event", event)
        self.disconnect()

## Interpreter

class GameCLIInterpreter:
    def __init__(self, client, hostname):
        self.prompt_hostname = hostname

        self.client = client

        self.handlers = {
            'joinq': self.join_queue,
            'quitq': self.quit_queue,
            'accept': self.accept,
            'deny': self.deny,
            'a': self.answer_a,
            'b': self.answer_b,
            'c': self.answer_c,
            'pass': self.pass_question,
        }

        self.nargs = {
            'joinq': 0,
            'quitq': 0,
            'accept': 0,
            'deny': 0,
            'a': 0,
            'b': 0,
            'c': 0,
            'pass': 0,
        }

    def greet(self):
        print("You are connected to the server.")
        print("To send a command, just type it in your terminal")
        print("and press enter.")
        print("")

        self.display_options()

    def display_options(self):
        if self.client.is_in_state(STATE_HOME):
            print("Available options: joinq (join queue), resume <match_id> (resume match)")
        elif self.client.is_in_state(STATE_QUEUE):
            print("Available options: quitq (quit queue)")
        elif self.client.is_in_state(STATE_AFK_CHECK):
            print("Available options: accept (accept game), deny (deny game)")
        elif self.client.is_in_state(STATE_GAME_ANSWERING):
            print("Available options: a (answer a), b (answer b), c (answer c), pass (pass)")
        else:
            pass

    def trunc_match_id(self, match_id):
        return f'{match_id[0:7]}...'

    def prompt(self):
        user = self.client.player_id if self.client.player_id else 'none'
        hostname = f'@{self.prompt_hostname}'
        path = f'/{self.trunc_match_id(self.client.match_id)}' if self.client.match_id else ''
            
        return f'{user}{hostname}{path}> '

    def can_handle(self, action):
        return action in self.handlers

    def handle(self, command):
        parts = command.split(' ')
        action = parts[0]

        if not self.can_handle(action):
            print("Invalid command")
            return
        
        nargs = self.nargs[action]

        if len(parts) - 1 != nargs:
            print("Wrong parameters")
            return

        args = parts[1:nargs + 1]
        self.handlers[action](*args)

    def handling_deny(f):
        def wrapper(self):
            try:
                f(self)
            except DeniedEvent:
                print("Denied by server")

        return wrapper

    def displaying_options(f):
        def wrapper(self):
            f(self)
            self.display_options()

        return wrapper

    @displaying_options
    @handling_deny
    def join_queue(self):
        self.client.join_queue()

    @displaying_options
    @handling_deny
    def quit_queue(self):
        self.client.quit_queue()

    @displaying_options
    @handling_deny
    def accept(self):
        self.client.accept_game()

    @displaying_options
    @handling_deny
    def deny(self):
        self.client.deny_game()

    @displaying_options
    @handling_deny
    def answer_a(self):
        self.client.answer_a()

    @displaying_options
    @handling_deny
    def answer_b(self):
        self.client.answer_b()

    @displaying_options
    @handling_deny
    def answer_c(self):
        self.client.answer_c()

    @displaying_options
    @handling_deny
    def pass_question(self):
        self.client.pass_question()

## Main loop

def main():
    parser = ArgumentParser(description='Talk to the beast')

    host_help = 'IP address or domain name of the server'
    parser.add_argument('-o', '--host', help=host_help,
            default='127.0.0.1')

    port_help = 'Port of the server'
    parser.add_argument('-p', '--port', help=port_help,
            default=10000)
    
    args = parser.parse_args()

    addrinfos = socket.getaddrinfo(args.host, args.port)

    connected = False
    for addrinfo in addrinfos:
        family, addr_type, proto, _, address = addrinfo

        try:
            sock = socket.socket(family, addr_type, proto)
            sock.connect(address)
            connected = True
            break
        except:
            pass

    if not connected:
        print(f"Could not connect to {host}:{port}")
        return

    client = GameClient(sock)
    interpreter = GameCLIInterpreter(client, args.host)

    interpreter.greet()

    try:
        while(True):
            prompt = interpreter.prompt()

            command = input(prompt)
            interpreter.handle(command)
    except KeyboardInterrupt:
        print("Interrupted")
        client.disconnect()

if __name__ == '__main__':
    main()
