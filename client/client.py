from __future__ import annotations

import asyncio
import functools
from dataclasses import dataclass
from enum import Enum
from inspect import iscoroutinefunction
from socket import socket as Socket
from typing import Callable, assert_never

from network import (AskTo1NetworkEvent, AskTo2NetworkEvent,
                     DenyAbortNetworkEvent, DisconnectedNetworkEvent,
                     GameAbortedNetworkEvent, GameEndNetworkEvent,
                     GameFoundNetworkEvent, GameNetworkClient,
                     JoinedGameNetworkEvent, NetworkEvent, PassTo1NetworkEvent,
                     PassTo2NetworkEvent, Score1NetworkEvent,
                     Score2NetworkEvent)


class GameClientState(Enum):
    LATENT = 0
    HOME = 1
    QUEUE = 2
    AFK_CHECK = 3
    GAME_ANSWERING = 4
    GAME_WAITING_OPPONENT = 5
    WAITING_SERVER = 6


class UnavailableOperation(Exception):
    pass


class GameClient:
    def __init__(self, socket: Socket):
        self.socket = socket
        self.networking = GameNetworkClient(socket)

        self.state = GameClientState.LATENT
        self.player_id = 0
        self.player_you_score = 0
        self.player_opponent_score = 0

        # Only one _pass_ and one _repass_ allowed. We need to keep
        # track.
        self.passes = 0

        self.event_listeners: list[Callable[[GameEvent], None]] = []

        self.networking.on_event_received(self._on_network_event)

    def _reset(self):
        self.player_id = 0
        self.player_you_score = 0
        self.player_opponent_score = 0
        self.passes = 0

    def _broadcast_event(self, event: GameEvent):
        for listener in self.event_listeners:
            if iscoroutinefunction(listener):
                asyncio.ensure_future(listener(event))
            else:
                listener(event)

    async def start(self):
        await self.networking.connect()
        self.state = GameClientState.HOME
        self._broadcast_event(ConnectedToGame())

    async def abort(self):
        await self.networking.abort()
        self.state = GameClientState.LATENT
        self._broadcast_event(GameEndEvent())

    async def _on_network_event(self, event: NetworkEvent):
        match event:
            case GameFoundNetworkEvent():
                self.state = GameClientState.AFK_CHECK
                self._broadcast_event(GameFoundEvent())

            case JoinedGameNetworkEvent(match_id, player_id):
                self.player_id = player_id

                if player_id == 1:
                    self.state = GameClientState.GAME_ANSWERING
                else:
                    self.state = GameClientState.GAME_WAITING_OPPONENT

                self._broadcast_event(JoinedGameEvent(match_id))

            case AskTo1NetworkEvent(question):
                self.passes = 0
                if self.player_id == 1:
                    self.state = GameClientState.GAME_ANSWERING
                    self._broadcast_event(AskYouEvent(question))
                else:
                    self.state = GameClientState.GAME_WAITING_OPPONENT
                    self._broadcast_event(AskOpponentEvent(question))

            case AskTo2NetworkEvent(question):
                self.passes = 0
                if self.player_id == 2:
                    self.state = GameClientState.GAME_ANSWERING
                    self._broadcast_event(AskYouEvent(question))
                else:
                    self.state = GameClientState.GAME_WAITING_OPPONENT
                    self._broadcast_event(AskOpponentEvent(question))

            case PassTo1NetworkEvent():
                self.passes += 1
                if self.player_id == 1:
                    self.state = GameClientState.GAME_ANSWERING
                    self._broadcast_event(PassEvent())
                else:
                    self.state = GameClientState.GAME_WAITING_OPPONENT

            case PassTo2NetworkEvent():
                self.passes += 1
                if self.player_id == 2:
                    self.state = GameClientState.GAME_ANSWERING
                    self._broadcast_event(PassEvent())
                else:
                    self.state = GameClientState.GAME_WAITING_OPPONENT

            case Score1NetworkEvent(score):
                if self.player_id == 1:
                    self.player_you_score = score
                    self._broadcast_event(YourScoreEvent(score))
                else:
                    self.player_opponent_score = score
                    self._broadcast_event(OpponentScoreEvent(score))

            case Score2NetworkEvent(score):
                if self.player_id == 2:
                    self.player_you_score = score
                    self._broadcast_event(YourScoreEvent(score))
                else:
                    self.player_opponent_score = score
                    self._broadcast_event(OpponentScoreEvent(score))

            case GameEndNetworkEvent():
                self.state = GameClientState.HOME
                your_score = self.player_you_score
                opponents_score = self.player_opponent_score

                self._broadcast_event(GameEndEvent(your_score, opponents_score))
                self._reset()

            case GameAbortedNetworkEvent():
                if self.state == GameClientState.AFK_CHECK:
                    self.state = GameClientState.HOME
                    self._broadcast_event(AfkCheckAbortedEvent())
                else:
                    self.state = GameClientState.HOME
                    your_score = self.player_you_score
                    opponents_score = self.player_opponent_score

                    self._broadcast_event(GameEndEvent(your_score, opponents_score))
                    self._reset()

            case DisconnectedNetworkEvent():
                # TODO: Not sure what to do in this case
                self.state = GameClientState.LATENT
                your_score = self.player_you_score
                opponents_score = self.player_opponent_score

                self._broadcast_event(GameEndEvent(your_score, opponents_score))

            case DenyAbortNetworkEvent():
                await self.abort()

            case _ as unreachable:
                assert_never(unreachable)

    def on_event_received(self, cb: Callable[[GameEvent], None]):
        self.event_listeners.append(cb)

    # MyPy can't comprehend
    def in_state(state: GameClientState):  # type: ignore
        def decorator_in_state(f):
            @functools.wraps(f)
            async def wrapper(self, *args, **kwargs):
                if self.state != state:
                    raise UnavailableOperation()

                await f(self, *args, **kwargs)

            return wrapper
        return decorator_in_state

    @in_state(GameClientState.HOME)
    async def join_queue(self):
        self.state = GameClientState.QUEUE
        await self.networking.send_join_queue()

    @in_state(GameClientState.QUEUE)
    async def quit_queue(self):
        self.state = GameClientState.HOME
        await self.networking.send_quit_queue()

    @in_state(GameClientState.AFK_CHECK)
    async def accept_game(self):
        self.state = GameClientState.WAITING_SERVER
        await self.networking.send_accept_game()

    @in_state(GameClientState.AFK_CHECK)
    async def deny_game(self):
        self.state = GameClientState.HOME
        await self.networking.send_deny_abort()

    @in_state(GameClientState.GAME_ANSWERING)
    async def answer_a(self):
        self.state = GameClientState.GAME_WAITING_OPPONENT
        await self.networking.send_answer_a()

    @in_state(GameClientState.GAME_ANSWERING)
    async def answer_b(self):
        self.state = GameClientState.GAME_WAITING_OPPONENT
        await self.networking.send_answer_b()

    @in_state(GameClientState.GAME_ANSWERING)
    async def answer_c(self):
        self.state = GameClientState.GAME_WAITING_OPPONENT
        await self.networking.send_answer_c()

    @in_state(GameClientState.GAME_ANSWERING)
    async def pazz(self):
        if self.passes >= 2:
            raise UnavailableOperation()

        # We received a PassToXNetworkEvent whenever either player
        # passes, so we update self.passes while handling that
        self.state = GameClientState.GAME_WAITING_OPPONENT
        await self.networking.send_pass()

    def get_potential_score_win(self):
        return [20, 10, 10][self.passes]

    def get_potential_score_loss(self):
        return [10, 5, 5][self.passes]


@dataclass
class ConnectedToGame:
    pass


@dataclass
class GameFoundEvent:
    pass


@dataclass
class JoinedGameEvent:
    game_id: bytes


@dataclass
class AskOpponentEvent:
    question: str


@dataclass
class AskYouEvent:
    question: str


@dataclass
class PassEvent:
    pass


@dataclass
class YourScoreEvent:
    score: int


@dataclass
class OpponentScoreEvent:
    score: int


@dataclass
class GameEndEvent:
    your_score: int
    opponents_score: int


@dataclass
class AfkCheckAbortedEvent:
    pass


GameEvent = ConnectedToGame | GameFoundEvent | JoinedGameEvent | \
    AskYouEvent | AskOpponentEvent | PassEvent | YourScoreEvent | \
    OpponentScoreEvent | GameEndEvent | AfkCheckAbortedEvent
