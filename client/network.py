from __future__ import annotations

import asyncio
from abc import ABC, abstractmethod
from dataclasses import dataclass
from inspect import iscoroutinefunction
from socket import socket as Socket
from typing import Awaitable, Callable, Type

from debug import debug
from protocol import (ACCEPT_GAME, ANSWER_A, ANSWER_B, ANSWER_C, ASK_AGAIN,
                      ASK_TO_01, ASK_TO_02, DENY_ABORT, DENY_RETRY,
                      GAME_ABORTED, GAME_END, GAME_FOUND, JOIN_QUEUE,
                      JOINED_GAME, PASS, PASS_TO_01, PASS_TO_02, QUIT_GAME,
                      QUIT_QUEUE, SCORE_01, SCORE_02, SET_NAME,
                      SET_NAME_MAX_SIZE)


class NameTooLong(Exception):
    pass


class GameNetworkClient:
    def __init__(self, socket: Socket):
        self.socket = socket
        self.event_listeners: list[Callable[[NetworkEvent], Awaitable[None]]] = []
        self.read_task = None

        # TODO: Use these
        self.reader: asyncio.StreamReader | None = None
        self.writer: asyncio.StreamWriter | None = None

    async def connect(self):
        self.reader, self.writer = \
            await asyncio.open_connection(sock=self.socket)

        self._try_kickoff_periodic_receive()

    async def abort(self):
        self.read_task.cancel()
        self.writer.close()
        await self.writer.wait_closed()

    async def send_deny_abort(self):
        self.writer.write(DENY_ABORT)
        await self.writer.drain()

    async def send_deny_retry(self):
        self.writer.write(DENY_RETRY)
        await self.writer.drain()

    async def send_set_name(self, name: str):
        if self.writer is None:
            return

        length = len(name)

        if length > int.from_bytes(SET_NAME_MAX_SIZE):
            raise NameTooLong()

        len_bytes = length.to_bytes(4, 'little', signed=False)

        b = SET_NAME + len_bytes + name.encode('utf8')
        self.writer.write(b)
        await self.writer.drain()

    async def send_join_queue(self):
        self.writer.write(JOIN_QUEUE)
        await self.writer.drain()

    async def send_quit_queue(self):
        self.writer.write(QUIT_QUEUE)
        await self.writer.drain()

    async def send_accept_game(self):
        self.writer.write(ACCEPT_GAME)
        await self.writer.drain()

    async def send_ask_again(self):
        self.writer.write(ASK_AGAIN)
        await self.writer.drain()

    async def send_answer_a(self):
        self.writer.write(ANSWER_A)
        await self.writer.drain()

    async def send_answer_b(self):
        self.writer.write(ANSWER_B)
        await self.writer.drain()

    async def send_answer_c(self):
        self.writer.write(ANSWER_C)
        await self.writer.drain()

    async def send_pass(self):
        self.writer.write(PASS)
        await self.writer.drain()

    async def send_quit_game(self):
        self.writer.write(QUIT_GAME)
        await self.writer.drain()

    def on_event_received(self, cb: Callable[[NetworkEvent], Awaitable[None]]):
        self.event_listeners.append(cb)
        self._try_kickoff_periodic_receive()

    def _broadcast_event(self, event: NetworkEvent):
        debug('Network', 'Broadcasting event', event)
        for listener in self.event_listeners:
            if iscoroutinefunction(listener):
                asyncio.ensure_future(listener(event))
            else:
                listener(event)

    def _try_kickoff_periodic_receive(self):
        if self.read_task is None and \
           self.reader is not None and \
           len(self.event_listeners) > 0:

            self.read_task = asyncio.ensure_future(
                self._periodic_try_receive())

    async def _periodic_try_receive(self):
        debug('Network', 'Periodicing')
        while True:
            # debug('Network', 'reader', self.reader)
            if self.reader is not None:
                event = await self.try_receive()

                if event is not None:
                    self._broadcast_event(event)

                    if isinstance(event, DisconnectedNetworkEvent):
                        await self.abort()

    async def try_receive(self) -> NetworkEvent | None:
        if self.reader is None:
            return None

        event_tag = await self.reader.read(1)

        if len(event_tag) == 0:
            return DisconnectedNetworkEvent()

        # debug('Network', 'Read', event_tag)
        event_cls = NetworkEventBase.event_clss[event_tag]

        # MyPy is lelé da cuca
        return await event_cls.read_params(self.reader)


async def readall(reader: asyncio.StreamReader, size: int) -> bytes:
    raw = bytes()
    # debug("Network", "Start reading...", size)

    while len(raw) < size:
        partial = await reader.read(size - len(raw))
        raw += partial
        # debug("Network", f"Read {len(partial)}!", f"New length is {len(raw)} / {size}")

    return raw


class NetworkEventBase(ABC):
    tag: bytes

    # Static
    event_clss: dict[bytes, Type[NetworkEvent]] = {}

    # Automatically store a reference to the subclass on definition
    def __init_subclass__(cls):
        NetworkEventBase.event_clss[cls.tag] = cls

    @classmethod
    @abstractmethod
    async def read_params(cls, reader: asyncio.StreamReader) -> NetworkEvent:
        raise NotImplementedError()


@dataclass
class DisconnectedNetworkEvent(NetworkEventBase):
    tag = b'\x00'

    @classmethod
    async def read_params(cls, reader: asyncio.StreamReader) -> NetworkEvent:
        return DisconnectedNetworkEvent()


@dataclass
class GameFoundNetworkEvent(NetworkEventBase):
    tag = GAME_FOUND

    @classmethod
    async def read_params(cls, reader: asyncio.StreamReader) -> NetworkEvent:
        return GameFoundNetworkEvent()


@dataclass
class JoinedGameNetworkEvent(NetworkEventBase):
    game_id: bytes
    player_id: int
    tag = JOINED_GAME

    @classmethod
    async def read_params(cls, reader: asyncio.StreamReader) -> NetworkEvent:
        game_id = await readall(reader, 8)
        player_id_raw = await readall(reader, 1)

        if player_id_raw == b'\x01':
            player_id = 1
        else:
            player_id = 2

        return JoinedGameNetworkEvent(game_id, player_id)


@dataclass
class AskTo1NetworkEvent(NetworkEventBase):
    question: str
    tag = ASK_TO_01

    @classmethod
    async def read_params(cls, reader: asyncio.StreamReader) -> NetworkEvent:
        length = int.from_bytes(await readall(reader, 4), 'little')
        question_raw = await readall(reader, length)

        question = question_raw.decode('utf8')
        return AskTo1NetworkEvent(question)


@dataclass
class AskTo2NetworkEvent(NetworkEventBase):
    question: str
    tag = ASK_TO_02

    @classmethod
    async def read_params(cls, reader: asyncio.StreamReader) -> NetworkEvent:
        length = int.from_bytes(await readall(reader, 4), 'little')
        question_raw = await readall(reader, length)

        question = question_raw.decode('utf8')
        return AskTo2NetworkEvent(question)


@dataclass
class PassTo1NetworkEvent(NetworkEventBase):
    tag = PASS_TO_01

    @classmethod
    async def read_params(cls, reader: asyncio.StreamReader) -> NetworkEvent:
        return PassTo1NetworkEvent()


@dataclass
class PassTo2NetworkEvent(NetworkEventBase):
    tag = PASS_TO_02

    @classmethod
    async def read_params(cls, reader: asyncio.StreamReader) -> NetworkEvent:
        return PassTo2NetworkEvent()


@dataclass
class Score1NetworkEvent(NetworkEventBase):
    score: int
    tag = SCORE_01

    @classmethod
    async def read_params(cls, reader: asyncio.StreamReader) -> NetworkEvent:
        score = int.from_bytes(await readall(reader, 4), 'little', signed=True)
        return Score1NetworkEvent(score)


@dataclass
class Score2NetworkEvent(NetworkEventBase):
    score: int
    tag = SCORE_02

    @classmethod
    async def read_params(cls, reader: asyncio.StreamReader) -> NetworkEvent:
        score = int.from_bytes(await readall(reader, 4), 'little', signed=True)
        return Score2NetworkEvent(score)


@dataclass
class GameEndNetworkEvent(NetworkEventBase):
    tag = GAME_END

    @classmethod
    async def read_params(cls, reader: asyncio.StreamReader) -> NetworkEvent:
        return GameEndNetworkEvent()


@dataclass
class GameAbortedNetworkEvent(NetworkEventBase):
    tag = GAME_ABORTED

    @classmethod
    async def read_params(cls, reader: asyncio.StreamReader) -> NetworkEvent:
        return GameAbortedNetworkEvent()


@dataclass
class DenyAbortNetworkEvent(NetworkEventBase):
    tag = DENY_ABORT

    @classmethod
    async def read_params(cls, reader: asyncio.StreamReader) -> NetworkEvent:
        return DenyAbortNetworkEvent()


# Equivalent type to NetworkEventBase but allows for narrowing
NetworkEvent = DisconnectedNetworkEvent | GameFoundNetworkEvent | \
    JoinedGameNetworkEvent | AskTo1NetworkEvent | AskTo2NetworkEvent | \
    PassTo1NetworkEvent | PassTo2NetworkEvent | Score1NetworkEvent | \
    Score2NetworkEvent | GameEndNetworkEvent | GameAbortedNetworkEvent | \
    DenyAbortNetworkEvent
