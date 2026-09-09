"""The update message decision table and how a message reaches its recipient."""

from __future__ import annotations

import asyncio
import uuid
from unittest.mock import AsyncMock, MagicMock, patch

import aiohttp
import pytest
from endstone import Logger

from endweave import update


@pytest.fixture
def mock_plugin(mock_logger: MagicMock) -> MagicMock:
    """Plugin double whose scheduler records the task instead of running it."""
    plugin = MagicMock()
    plugin.logger = mock_logger
    return plugin


def get_update_message(newest: str, current: str, *, console: bool) -> update._UpdateMessage | None:
    with (
        patch.object(update, "_get_newest_version", AsyncMock(return_value=newest)),
        patch.object(update, "__version__", current),
    ):
        return asyncio.run(update._get_update_message(console=console))


@pytest.mark.parametrize("console", [True, False])
def test_older_than_the_latest_release_is_reported_to_everyone(console: bool) -> None:
    message = get_update_message("v0.5.0", "0.4.3", console=console)
    assert message == (Logger.Level.WARNING, "There is a newer plugin version available: 0.5.0, you're on: 0.4.3")


@pytest.mark.parametrize("console", [True, False])
def test_up_to_date_says_nothing(console: bool) -> None:
    assert get_update_message("v0.4.3", "0.4.3", console=console) is None


def test_leading_v_in_the_release_tag_is_not_a_difference() -> None:
    assert get_update_message("v0.4.3", "0.4.3", console=True) is None


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


def test_pre_release_counts_as_ahead_rather_than_as_a_dev_build() -> None:
    message = get_update_message("v0.4.3", "0.4.4rc1", console=True)
    assert message is not None
    assert "newer version of the plugin than is released" in message.text


def test_unparsable_local_version_asks_the_user_to_update() -> None:
    message = get_update_message("v0.4.3", "custom-build", console=True)
    assert message == (Logger.Level.INFO, "You are using a custom version, consider updating.")


def test_unparsable_release_tag_reads_as_a_failed_check() -> None:
    assert get_update_message("not-a-version", "0.4.3", console=True) == (
        Logger.Level.WARNING,
        "Could not check for updates, check your connection.",
    )
    assert get_update_message("not-a-version", "0.4.3", console=False) is None


def test_failed_fetch_is_only_reported_to_the_console() -> None:
    with (
        patch.object(update, "_get_newest_version", AsyncMock(side_effect=aiohttp.ClientError())),
        patch.object(update, "__version__", "0.4.3"),
    ):
        assert asyncio.run(update._get_update_message(console=True)) == (
            Logger.Level.WARNING,
            "Could not check for updates, check your connection.",
        )
        assert asyncio.run(update._get_update_message(console=False)) is None


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
