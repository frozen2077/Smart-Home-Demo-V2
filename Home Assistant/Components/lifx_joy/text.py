import logging
import time
import json

from homeassistant.components.text import TextEntity

from homeassistant.config_entries import ConfigEntry
from homeassistant.const import EntityCategory
from homeassistant.core import HomeAssistant, callback
from homeassistant.helpers.entity_platform import AddEntitiesCallback

from .const import (
    ATTR_THEME,
    DOMAIN,
)
from .coordinator import LIFXUpdateCoordinator
from .entity import LIFXEntity
from .util import lifx_features

_LOGGER = logging.getLogger(__name__)



async def async_setup_entry(
    hass: HomeAssistant, entry: ConfigEntry, async_add_entities: AddEntitiesCallback
) -> None:
    coordinator: LIFXUpdateCoordinator = hass.data[DOMAIN][entry.entry_id]
    entities: list[LIFXEntity] = []
    # entities.append(JoyThemeSettingEntity(coordinator))    
    async_add_entities(entities, update_before_add=True)


class JoyThemeSettingEntity(LIFXEntity, TextEntity):

    _attr_icon = "mdi:playlist-play"
    _attr_translation_key = "preset"

    def __init__(self, coordinator: LIFXUpdateCoordinator) -> None:
        """Initialize WLED ."""
        super().__init__(coordinator=coordinator)
        self._attr_native_max = 20000
        self._attr_native_value = '{"default10": [[50,100,100,1500]], "default20": [[120,100,100,1500]]}'
        self.coordinator.theme_list = json.loads(self._attr_native_value)
        self._attr_unique_id = f"{coordinator.serial_number}_theme_setting" 

    async def async_set_value(self, value: str):
        """Change the value."""
        try:
            theme_dict = json.loads(value)
        except:
            theme_dict = None
            _LOGGER.error(f"Invalid theme setting {value}")
        if isinstance(theme_dict, dict):
            self.coordinator.theme_list = theme_dict
            _LOGGER.debug(f"theme setting {self.coordinator.theme_list}")
            self._attr_native_value = value
            super()._handle_coordinator_update()

