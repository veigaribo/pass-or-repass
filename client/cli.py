from __future__ import annotations

import asyncio
import os
import sys
from abc import ABC, abstractmethod
from getpass import getuser
from typing import assert_never

from client import (AfkCheckAbortedEvent, AskOpponentEvent, AskYouEvent,
                    ConnectedToGame, GameClient, GameClientState, GameEndEvent,
                    GameEvent, GameFoundEvent, JoinedGameEvent,
                    OpponentScoreEvent, PassEvent, UnavailableOperation,
                    YourScoreEvent)
from debug import debug


class GameCli:
    def __init__(self, client: GameClient, host: str):
        self.client = client
        self.host = host

        # Each handler handles a different state
        self.handler = self.get_handler()

        self.game_name = getuser() or 'anon'
        self.peer_name = 'connecting'
        self.match_id = None

        self.reader = None
        self.writer = None

        self.client.on_event_received(self._on_game_event)
        self.connected = asyncio.get_event_loop().create_future()

    async def start(self):
        self.reader, self.writer = await self._connect_stdin_stdout()
        await self.client.start()

    def get_handler(self) -> GameCliHandler:
        match self.client.state:
            case GameClientState.LATENT:
                return GameCliLatentHandler(self)
            case GameClientState.HOME:
                return GameCliHomeHandler(self)
            case GameClientState.QUEUE:
                return GameCliQueueHandler(self)
            case GameClientState.AFK_CHECK:
                return GameCliAfkCheckHandler(self)
            case GameClientState.GAME_ANSWERING:
                return GameCliAnsweringHandler(self)
            case GameClientState.GAME_WAITING_OPPONENT:
                return GameCliWaitingOpponentHandler(self)
            case GameClientState.WAITING_SERVER:
                return GameCliWaitingServerHandler(self)
            case _ as unreachable:
                assert_never(unreachable)

    async def _on_game_event(self, event: GameEvent):
        if self.writer is None:
            return

        debug('CLI', 'Game event received', event, '@', self.client.state)
        self.handler = self.get_handler()

        match event:
            case ConnectedToGame():
                self.connected.set_result(None)
                await self.display_greet()
            case GameFoundEvent():
                await self.display_ln()
                await self.display_help()
                await self.display_prompt()
            case JoinedGameEvent(id):
                self.match_id = id

                await self.display_ln()
                await self.display_help()
                await self.display_prompt()
            case AskYouEvent(question):
                potential_win = self.client.get_potential_score_win()
                potential_loss = self.client.get_potential_score_loss()

                self.writer.write(f"\n\nQuestion for you, worth +{potential_win}/-{potential_loss} points:\n\n".encode('utf8'))
                self.writer.write(question.encode('utf8'))
                self.writer.write("\n\n".encode('utf8'))
                await self.display_help()
                await self.display_prompt()
            case AskOpponentEvent(question):
                potential_win = self.client.get_potential_score_win()
                potential_loss = self.client.get_potential_score_loss()

                self.writer.write(f"\n\nQuestion for the opponent, worth +{potential_win}/-{potential_loss} points:\n\n".encode('utf8'))
                self.writer.write(question.encode('utf8'))
                self.writer.write("\n\n".encode('utf8'))
                await self.display_help()
            case PassEvent():
                potential_win = self.client.get_potential_score_win()
                potential_loss = self.client.get_potential_score_loss()

                if self.client.passes == 1:
                    await self.display_ln()
                    self.writer.write(f"Passed. Your turn, for +{potential_win}/-{potential_loss} points".encode('utf8'))
                    await self.display_ln()
                else:  # == 2
                    await self.display_ln()
                    self.writer.write(f"Repassed. Your turn, for +{potential_win}/-{potential_loss} points".encode('utf8'))
                    await self.display_ln()

                await self.display_help()
                await self.display_prompt()
            case YourScoreEvent(score):
                await self.display_ln()
                await self.display_ln()
                self.writer.write(f"Your score is now {score}".encode('utf8'))
                await self.display_ln()
            case OpponentScoreEvent(score):
                await self.display_ln()
                self.writer.write(f"Your opponent's score is now {score}".encode('utf8'))
                await self.display_ln()
            case GameEndEvent(your_score, opponents_score):
                self.writer.write("Game ended. Final scores:\n\n".encode('utf8'))
                self.writer.write(f"You: {your_score}\n".encode('utf8'))
                self.writer.write(f"Opponent: {opponents_score}\n".encode('utf8'))
                await self.display_ln()
                await self.display_help()
                await self.display_prompt()
            case AfkCheckAbortedEvent():
                await self.display_ln()
                await self.display_help()
                await self.display_prompt()
            case _:
                assert_never(event)

    async def display_greet(self):
        self.writer.write("You are connected to the server.\n".encode('utf8'))
        self.writer.write("To send a command, just type it in your terminal and press enter.\n".encode('utf8'))
        self.writer.write("\n".encode('utf8'))
        await self.writer.drain()

    async def display_help(self):
        await self.handler.display_help()
        await self.writer.drain()

    async def display_prompt(self):
        self.writer.write(self.game_name.encode('utf8'))
        self.writer.write(b'@')
        self.writer.write(self.host.encode('utf8'))

        if self.match_id is not None:
            self.writer.write(b'/')
            self.writer.write(self.match_id)

        self.writer.write(b'> ')
        await self.writer.drain()

    async def display_invalid_input(self):
        self.writer.write('Invalid input.\n'.encode('utf8'))

    async def display_ln(self):
        self.writer.write(b'\n')
        await self.writer.drain()

    async def loop(self):
        await self.connected

        while True:
            try:
                self.handler = self.get_handler()
                await self.display_help()
                await self.display_prompt()

                # TODO: Should this be just '\n'?
                input = await self.reader.readuntil(os.linesep.encode('utf8'))
                await self.handler.interpret_input(input.strip())
            except asyncio.IncompleteReadError:
                self.reader.feed_eof()
                break
            except KeyboardInterrupt:
                break

    # https://stackoverflow.com/a/64317899
    async def _connect_stdin_stdout(self):
        reader = asyncio.StreamReader()
        protocol = asyncio.StreamReaderProtocol(reader)

        loop = asyncio.get_event_loop()
        await loop.connect_read_pipe(lambda: protocol, sys.stdin)
        w_transport, w_protocol = await loop.connect_write_pipe(asyncio.streams.FlowControlMixin, sys.stdout)

        writer = asyncio.StreamWriter(w_transport, w_protocol, reader, loop)
        return reader, writer

    async def internal_error(self):
        self.writer.write('Oops. An internal error occurred. Aborting.\n'.encode('utf8'))
        await self.writer.drain()

        await self.client.abort()


# State-specific behavior
class GameCliHandler(ABC):
    def __init__(self, cli: GameCli):
        self.cli = cli

    # No need to drain
    @abstractmethod
    async def display_help(self):
        pass

    # No need to trim
    @abstractmethod
    async def interpret_input(self, input: str):
        pass


class GameCliLatentHandler(GameCliHandler):
    async def display_help(self):
        self.cli.writer.write('Not connected to the server yet. This is weird.\n'.encode('utf8'))

    async def interpret_input(self, input: str):
        if self.cli.writer is None:
            return

        self.cli.writer.write('Not available to accept input yet...'.encode('utf8'))
        await self.cli.writer.drain()


class GameCliHomeHandler(GameCliHandler):
    async def display_help(self):
        self.cli.writer.write('Available options: joinq / j (join queue)\n'.encode('utf8'))

    async def interpret_input(self, input: str):
        if input in [b'j', b'joinq']:
            await self.cli.client.join_queue()
        else:
            await self.cli.display_invalid_input()


class GameCliQueueHandler(GameCliHandler):
    async def display_help(self):
        self.cli.writer.write('Available options: quitq / q (quit queue)\n'.encode('utf8'))

    async def interpret_input(self, input: str):
        if input in [b'q', b'quitq']:
            await self.cli.client.quit_queue()
        else:
            await self.cli.display_invalid_input()


class GameCliAfkCheckHandler(GameCliHandler):
    async def display_help(self):
        self.cli.writer.write('Available options: accept / a (accept game), deny / d (deny game)\n'.encode('utf8'))

    async def interpret_input(self, input: str):
        if input in [b'a', b'accept']:
            await self.cli.client.accept_game()
        else:
            await self.cli.display_invalid_input()


class GameCliAnsweringHandler(GameCliHandler):
    async def display_help(self):
        if self.cli.client.passes < 2:
            self.cli.writer.write('Available options: a (answer a), b (answer b), c (answer c), p / pass (pass)\n'.encode('utf8'))
        else:
            self.cli.writer.write('Available options: a (answer a), b (answer b), c (answer c)\n'.encode('utf8'))

    async def interpret_input(self, input: str):
        if self.cli.writer is None:
            return

        if input in [b'a']:
            await self.cli.client.answer_a()
        elif input in [b'b']:
            await self.cli.client.answer_b()
        elif input in [b'c']:
            await self.cli.client.answer_c()
        elif input in [b'p', 'pass']:
            try:
                await self.cli.client.pazz()
            except UnavailableOperation:
                self.cli.writer.write('Cannot pass anymore. Gotta answer.\n'.encode('utf8'))
                await self.cli.writer.drain()
        else:
            await self.cli.display_invalid_input()


class GameCliWaitingOpponentHandler(GameCliHandler):
    async def display_help(self):
        self.cli.writer.write('Waiting for opponent.\n'.encode('utf8'))

    async def interpret_input(self, input: str):
        await self.cli.display_invalid_input()


class GameCliWaitingServerHandler(GameCliHandler):
    async def display_help(self):
        self.cli.writer.write('Waiting for server.\n'.encode('utf8'))

    async def interpret_input(self, input: str):
        await self.cli.display_invalid_input()
