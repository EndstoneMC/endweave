"""bStats metrics polyfill for Endstone."""

import platform
from typing import Any, Dict

from endstone.metrics import Metrics


class EndweaveMetrics(Metrics):
    """Polyfill for endstone<=0.11.2 Metrics which has runtime errors and
    incorrect data formatting in append_platform_data and append_service_data.
    """

    def append_platform_data(self, platform_data: Dict[str, Any]) -> None:
        super().append_platform_data(platform_data)
        server = self._plugin.server
        if "minecraftVersion" in platform_data:
            platform_data.pop("minecraftVersion")

        if "bukkitVersion" not in platform_data:
            platform_data["bukkitVersion"] = f"{server.version} (MC: {server.minecraft_version})"
            platform_data["bukkitName"] = server.name

        os_arch = platform.machine().lower()
        if os_arch == "x86_64":
            os_arch = "amd64"
        platform_data["osArch"] = os_arch

    def append_service_data(self, service_data: dict[str, object]) -> None:
        description = self._plugin._description
        if description is not None:
            service_data["pluginVersion"] = description.version
