"""Version ranges, ordering and registry lookups for known Bedrock protocols."""

from collections.abc import Iterator

import pytest

from endweave.protocol import version as version_module
from endweave.protocol.version import (
    UNKNOWN,
    ProtocolVersion,
    SubVersionRange,
    get_by_name,
    get_protocol,
    get_protocols,
    is_registered,
    register,
    v1_21_120,
    v1_21_124,
    v26_0,
    v26_10,
    v26_20,
    v26_30,
)

UNREGISTERED_VERSION = max(get_protocols()).version + 1


@pytest.fixture
def isolated_registry() -> Iterator[None]:
    """Restore the module registry so tests may register versions of their own."""
    versions = dict(version_module._VERSIONS)
    version_list = list(version_module._VERSION_LIST)
    yield
    version_module._VERSIONS.clear()
    version_module._VERSIONS.update(versions)
    version_module._VERSION_LIST[:] = version_list


def test_sub_version_range_expands_every_patch() -> None:
    assert list(SubVersionRange("26", 0, 3)) == ["26.0", "26.1", "26.2", "26.3"]


def test_sub_version_range_keeps_the_third_component() -> None:
    """Bedrock version strings always carry a patch, so no bare "26" alias."""
    assert "26" not in list(SubVersionRange("26", 0, 3))


@pytest.mark.parametrize(("range_from", "range_to"), [(-1, 3), (3, 3), (3, 1)])
def test_sub_version_range_rejects_an_empty_range(range_from: int, range_to: int) -> None:
    with pytest.raises(ValueError):
        SubVersionRange("26", range_from, range_to)


def test_single_version_includes_only_its_own_name() -> None:
    assert v26_20.included_versions == frozenset({"26.20"})
    assert not v26_20.is_range


def test_range_includes_every_covered_version() -> None:
    assert v26_0.included_versions == frozenset({"26.0", "26.1", "26.2", "26.3"})
    assert v26_0.is_range


def test_range_is_derived_from_a_hyphenated_name() -> None:
    assert ProtocolVersion(924, "26.0-26.3").included_versions == v26_0.included_versions


def test_derived_range_keeps_preview_style_names() -> None:
    derived = ProtocolVersion(2192, "26.50.26-26.50.28")
    assert derived.included_versions == frozenset({"26.50.26", "26.50.27", "26.50.28"})


def test_an_explicit_range_overrides_the_name() -> None:
    explicit = ProtocolVersion(924, "26.0-26.3", SubVersionRange("26", 0, 1))
    assert explicit.included_versions == frozenset({"26.0", "26.1"})


@pytest.mark.parametrize("name", ["1.21.124-26.0", "26.0-26.x", "1.26-26.3", "26.beta-x", "x"])
def test_underivable_range_names_are_rejected(name: str) -> None:
    with pytest.raises(ValueError):
        ProtocolVersion(924, name)


def test_wildcard_covers_a_whole_hotfix_line() -> None:
    """Bedrock spends the last digit of the patch on hotfixes, so 26.2x is 26.20 to 26.29."""
    wildcard = ProtocolVersion(975, "26.2x")
    assert wildcard.included_versions == frozenset(f"26.2{hotfix}" for hotfix in range(10))
    assert wildcard.is_version_wildcard


def test_wildcard_reads_a_multi_digit_line() -> None:
    assert ProtocolVersion(859, "1.21.12x").included_versions == frozenset(f"1.21.12{hotfix}" for hotfix in range(10))


def test_java_style_wildcard_reads_the_same_way() -> None:
    """The Java form is the same rule with nothing standing before the "x"."""
    assert ProtocolVersion(47, "1.8.x").included_versions == frozenset(f"1.8.{hotfix}" for hotfix in range(10))


def test_a_plain_name_is_not_a_wildcard() -> None:
    assert not v26_0.is_version_wildcard
    assert not v26_20.is_version_wildcard


def test_str_names_the_version_and_id() -> None:
    assert str(v26_0) == "26.0-26.3 (924)"


def test_ordering_follows_the_protocol_id() -> None:
    assert v26_20 > v26_10
    assert v26_10 < v26_20
    assert v26_10 <= v26_10
    assert v1_21_120 <= v26_0 <= v26_30


def test_sorting_puts_the_oldest_first() -> None:
    assert sorted([v26_30, v1_21_120, v26_10]) == [v1_21_120, v26_10, v26_30]


def test_equal_ids_compare_and_hash_alike() -> None:
    assert ProtocolVersion(924, "whatever") == v26_0
    assert len({ProtocolVersion(924, "whatever"), v26_0}) == 1


def test_comparing_with_a_plain_int_is_a_type_error() -> None:
    assert v26_0 != 924
    with pytest.raises(TypeError):
        _ = v26_0 < 924  # type: ignore[operator]


def test_registered_ids_are_reported() -> None:
    assert is_registered(975)
    assert not is_registered(UNREGISTERED_VERSION)


def test_get_protocol_returns_the_registered_instance() -> None:
    assert get_protocol(924) is v26_0


def test_get_protocol_makes_a_placeholder_for_unregistered_ids() -> None:
    unregistered = get_protocol(UNREGISTERED_VERSION)
    assert unregistered.name == f"Unknown ({UNREGISTERED_VERSION})"
    assert not unregistered.known
    assert not is_registered(UNREGISTERED_VERSION)


@pytest.mark.parametrize("version", [-2168, -1, 0])
def test_get_protocol_has_a_placeholder_for_every_id(version: int) -> None:
    """A client may put any int on the wire, so no id may raise."""
    placeholder = get_protocol(version)
    assert placeholder.name == f"Unknown ({version})"
    assert not placeholder.known


def test_a_placeholder_name_is_never_read_as_a_range() -> None:
    placeholder = ProtocolVersion(-1, "Unknown (-1)", known=False)
    assert placeholder.included_versions == frozenset({"Unknown (-1)"})


def test_registered_versions_are_known() -> None:
    assert v26_0.known


def test_unknown_constant_is_not_registered() -> None:
    assert not UNKNOWN.known
    assert not is_registered(UNKNOWN.version)


def test_get_protocols_is_sorted_and_detached() -> None:
    protocols = get_protocols()
    assert protocols == sorted(protocols)
    protocols.clear()
    assert get_protocols()


def test_get_by_name_resolves_a_covered_version() -> None:
    assert get_by_name("26.2") is v26_0


def test_get_by_name_resolves_the_registered_name() -> None:
    assert get_by_name("26.30-26.32") is v26_30
    assert get_by_name("26.20") is v26_20


@pytest.mark.parametrize("name", ["1.26.2", "1.26.0-1.26.3", "1.26.5x", "1.26.55"])
def test_get_by_name_reads_the_legacy_form(name: str) -> None:
    """Clients and older Endstone releases still report 26 and later as 1.26."""
    assert get_by_name(name) is get_by_name(name[2:])
    assert get_by_name(name) is not None


def test_get_by_name_leaves_versions_before_26_alone() -> None:
    assert get_by_name("1.21.124") is v1_21_124
    assert get_by_name("21.124") is None


def test_get_by_name_returns_none_for_an_unknown_version() -> None:
    assert get_by_name("26.99") is None


def test_register_inserts_in_protocol_order(isolated_registry: None) -> None:
    added = register(UNREGISTERED_VERSION, "99.0")

    assert get_protocol(UNREGISTERED_VERSION) is added
    assert get_by_name("99.0") is added
    assert get_protocols() == sorted(get_protocols())
    assert get_protocols()[-1] is added
