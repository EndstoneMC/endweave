"""The cached release check, the update message decision table, and how a message reaches its recipient."""

from __future__ import annotations

import asyncio
import uuid
from unittest.mock import AsyncMock, MagicMock, patch

import aiohttp
import pytest
from endstone import Logger
from packaging.version import Version

from endweave import update


class Py310TimeoutError(Exception):
    """``asyncio.TimeoutError`` as Python 3.10 defines it, unrelated to the builtin ``TimeoutError``."""


class FakeResponse:
    def __init__(self, outcome: object) -> None:
        self._outcome = outcome

    async def __aenter__(self) -> FakeResponse:
        await asyncio.sleep(0)
        if isinstance(self._outcome, BaseException):
            raise self._outcome
        return self

    async def __aexit__(self, *args: object) -> None:
        return None

    def raise_for_status(self) -> None:
        return None

    async def json(self) -> object:
        return self._outcome


class FakeGitHub:
    """``aiohttp.ClientSession`` double replaying one queued outcome, a JSON body or an exception, per request."""

    def __init__(self, *outcomes: object) -> None:
        self._outcomes = list(outcomes)
        self.requests = 0

    def __call__(self, **kwargs: object) -> FakeGitHub:
        return self

    async def __aenter__(self) -> FakeGitHub:
        return self

    async def __aexit__(self, *args: object) -> None:
        return None

    def get(self, url: str, **kwargs: object) -> FakeResponse:
        outcome = self._outcomes[min(self.requests, len(self._outcomes) - 1)]
        self.requests += 1
        return FakeResponse(outcome)


@pytest.fixture(autouse=True)
def fresh_cache() -> None:
    """Every test starts with an empty check cache and a lock bound to no event loop."""
    update._cached_version = None
    update._cache_expiry = 0.0
    update._cache_lock = asyncio.Lock()


@pytest.fixture
def mock_plugin(mock_logger: MagicMock) -> MagicMock:
    """Plugin double whose scheduler records the task instead of running it."""
    plugin = MagicMock()
    plugin.logger = mock_logger
    return plugin


def get_newest_version(github: FakeGitHub, calls: int = 1) -> list[Version | None]:
    async def check() -> list[Version | None]:
        return [await update._get_newest_version() for _ in range(calls)]

    with patch.object(aiohttp, "ClientSession", github):
        return asyncio.run(check())


def get_update_message(newest: str | None, current: str, *, console: bool) -> update._UpdateMessage | None:
    with (
        patch.object(update, "_get_newest_version", AsyncMock(return_value=Version(newest) if newest else None)),
        patch.object(update, "__version__", current),
    ):
        return asyncio.run(update._get_update_message(console=console))


def test_the_leading_v_in_a_release_tag_is_not_part_of_the_version() -> None:
    assert get_newest_version(FakeGitHub({"tag_name": "v0.4.3"})) == [Version("0.4.3")]


def test_an_unparsable_release_tag_reads_as_a_failed_check() -> None:
    assert get_newest_version(FakeGitHub({"tag_name": "not-a-version"})) == [None]


def test_a_release_document_without_a_tag_reads_as_a_failed_check() -> None:
    assert get_newest_version(FakeGitHub({})) == [None]


def test_a_rate_limited_check_reads_as_a_failed_check() -> None:
    refused = aiohttp.ClientResponseError(MagicMock(), (), status=403, message="rate limit exceeded")
    assert get_newest_version(FakeGitHub(refused)) == [None]


def test_a_total_timeout_reads_as_a_failed_check() -> None:
    assert get_newest_version(FakeGitHub(asyncio.TimeoutError())) == [None]


def test_the_timeout_python_3_10_raises_is_caught_as_well() -> None:
    with patch.object(asyncio, "TimeoutError", Py310TimeoutError):
        assert get_newest_version(FakeGitHub(Py310TimeoutError())) == [None]


def test_a_successful_check_is_answered_from_the_cache_next_time() -> None:
    github = FakeGitHub({"tag_name": "v0.5.0"})
    assert get_newest_version(github, calls=2) == [Version("0.5.0"), Version("0.5.0")]
    assert github.requests == 1


def test_checks_that_overlap_share_one_request() -> None:
    github = FakeGitHub({"tag_name": "v0.5.0"})

    async def check_together() -> list[Version | None]:
        return list(await asyncio.gather(update._get_newest_version(), update._get_newest_version()))

    with patch.object(aiohttp, "ClientSession", github):
        assert asyncio.run(check_together()) == [Version("0.5.0"), Version("0.5.0")]
    assert github.requests == 1


def test_a_failed_check_is_not_repeated_until_the_retry_window_is_up() -> None:
    github = FakeGitHub(aiohttp.ClientError(), {"tag_name": "v0.5.0"})
    assert get_newest_version(github, calls=2) == [None, None]
    assert github.requests == 1


def test_a_failed_check_is_retried_rather_than_remembered_until_restart() -> None:
    github = FakeGitHub(aiohttp.ClientError(), {"tag_name": "v0.5.0"})
    with patch.object(update, "_FAILURE_TTL", 0.0):
        assert get_newest_version(github, calls=2) == [None, Version("0.5.0")]
    assert github.requests == 2


@pytest.mark.parametrize("console", [True, False])
def test_older_than_the_latest_release_is_reported_to_everyone(console: bool) -> None:
    message = get_update_message("v0.5.0", "0.4.3", console=console)
    assert message == (Logger.Level.WARNING, "There is a newer plugin version available: 0.5.0, you're on: 0.4.3")


@pytest.mark.parametrize("console", [True, False])
def test_up_to_date_says_nothing(console: bool) -> None:
    assert get_update_message("v0.4.3", "0.4.3", console=console) is None


def test_dev_build_ahead_of_the_latest_release_is_only_told_to_the_console() -> None:
    message = get_update_message("v0.4.3", "0.4.4.dev1", console=True)
    assert message is not None
    assert message.level is Logger.Level.INFO
    assert "development version" in message.text
    assert get_update_message("v0.4.3", "0.4.4.dev1", console=False) is None


def test_release_ahead_of_the_latest_release_warns_the_console() -> None:
    message = get_update_message("v0.4.3", "0.5.0", console=True)
    assert message is not None
    assert message.level is Logger.Level.WARNING
    assert "newer version of the plugin than is released" in message.text


def test_pre_release_ahead_of_the_latest_release_is_reported_as_a_development_build() -> None:
    message = get_update_message("v0.4.3", "0.5.0rc1", console=True)
    assert message is not None
    assert message.level is Logger.Level.INFO
    assert "development version" in message.text
    assert get_update_message("v0.4.3", "0.5.0rc1", console=False) is None


@pytest.mark.parametrize("console", [True, False])
def test_pre_release_older_than_the_final_release_is_told_to_update(console: bool) -> None:
    message = get_update_message("v0.5.0", "0.5.0rc1", console=console)
    assert message == (Logger.Level.WARNING, "There is a newer plugin version available: 0.5.0, you're on: 0.5.0rc1")


@pytest.mark.parametrize("console", [True, False])
def test_a_newer_pre_release_is_offered_to_someone_already_on_one(console: bool) -> None:
    message = get_update_message("v0.5.0rc2", "0.5.0rc1", console=console)
    assert message == (Logger.Level.WARNING, "There is a newer plugin version available: 0.5.0rc2, you're on: 0.5.0rc1")


@pytest.mark.parametrize("console", [True, False])
def test_whatever_github_calls_the_latest_release_is_offered(console: bool) -> None:
    message = get_update_message("v0.5.0rc1", "0.4.3", console=console)
    assert message == (Logger.Level.WARNING, "There is a newer plugin version available: 0.5.0rc1, you're on: 0.4.3")


def test_unparsable_local_version_asks_the_user_to_update() -> None:
    message = get_update_message("v0.4.3", "custom-build", console=True)
    assert message == (Logger.Level.INFO, "You are using a custom version, consider updating.")


def test_failed_check_is_only_reported_to_the_console() -> None:
    assert get_update_message(None, "0.4.3", console=True) == (
        Logger.Level.WARNING,
        "Could not check for updates, check your connection.",
    )
    assert get_update_message(None, "0.4.3", console=False) is None


def run_delivery(mock_plugin: MagicMock, unique_id: uuid.UUID | None, message: update._UpdateMessage | None) -> None:
    with patch.object(update, "_get_update_message", AsyncMock(return_value=message)):
        asyncio.run(update._send_update_message(mock_plugin, unique_id))
    if mock_plugin.server.scheduler.run_task.called:
        mock_plugin.server.scheduler.run_task.call_args.args[1]()


def test_console_delivery_logs_at_the_message_level(mock_plugin: MagicMock) -> None:
    run_delivery(mock_plugin, None, update._UpdateMessage(Logger.Level.WARNING, "out of date"))
    mock_plugin.logger.warning.assert_called_once_with("out of date")

    mock_plugin.server.scheduler.run_task.reset_mock()
    run_delivery(mock_plugin, None, update._UpdateMessage(Logger.Level.INFO, "dev build"))
    mock_plugin.logger.info.assert_called_once_with("dev build")


def test_player_delivery_prefixes_the_message(mock_plugin: MagicMock) -> None:
    unique_id = uuid.uuid4()
    player = mock_plugin.server.get_player.return_value
    run_delivery(mock_plugin, unique_id, update._UpdateMessage(Logger.Level.WARNING, "out of date"))
    mock_plugin.server.get_player.assert_called_once_with(unique_id)
    player.send_message.assert_called_once_with(update.PREFIX + "out of date")


def test_player_who_left_before_the_check_returned_is_skipped(mock_plugin: MagicMock) -> None:
    mock_plugin.server.get_player.return_value = None
    run_delivery(mock_plugin, uuid.uuid4(), update._UpdateMessage(Logger.Level.WARNING, "out of date"))
    mock_plugin.logger.warning.assert_not_called()


def test_nothing_is_scheduled_when_there_is_nothing_to_say(mock_plugin: MagicMock) -> None:
    run_delivery(mock_plugin, None, None)
    mock_plugin.server.scheduler.run_task.assert_not_called()
