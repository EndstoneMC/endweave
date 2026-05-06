"""CraftingDataEntry and Recipe variant codecs."""

import enum
from dataclasses import dataclass, field
from typing import Any

from endstone_endweave.codec.reader import PacketReader
from endstone_endweave.codec.writer import PacketWriter

from .enums import CraftingDataEntryType
from .item import NETWORK_ITEM_INSTANCE_DESCRIPTOR, ItemInstance
from .primitives import Type


class ItemDescriptorKind(enum.IntEnum):
    INVALID = 0
    DEFAULT = 1
    MOLANG = 2
    ITEM_TAG = 3
    DEFERRED = 4
    COMPLEX_ALIAS = 5


@dataclass
class ItemDescriptor:
    kind: ItemDescriptorKind = ItemDescriptorKind.INVALID
    item_id: int = 0
    aux_value: int = 0
    tag_expression: str = ""
    molang_version: int = 0
    item_tag: str = ""
    full_name: str = ""


class _ItemDescriptorType(Type[ItemDescriptor]):
    def read(self, reader: PacketReader) -> ItemDescriptor:
        kind = ItemDescriptorKind(reader.read_byte())
        if kind is ItemDescriptorKind.DEFAULT:
            item_id = reader.read_short_le()
            aux_value = reader.read_short_le() if item_id != 0 else 0
            return ItemDescriptor(kind=kind, item_id=item_id, aux_value=aux_value)
        if kind is ItemDescriptorKind.MOLANG:
            return ItemDescriptor(
                kind=kind,
                tag_expression=reader.read_string(),
                molang_version=reader.read_byte(),
            )
        if kind is ItemDescriptorKind.ITEM_TAG:
            return ItemDescriptor(kind=kind, item_tag=reader.read_string())
        if kind is ItemDescriptorKind.DEFERRED:
            return ItemDescriptor(
                kind=kind,
                full_name=reader.read_string(),
                aux_value=reader.read_short_le(),
            )
        if kind is ItemDescriptorKind.COMPLEX_ALIAS:
            return ItemDescriptor(kind=kind, full_name=reader.read_string())
        return ItemDescriptor(kind=kind)

    def write(self, writer: PacketWriter, value: ItemDescriptor) -> None:
        writer.write_byte(value.kind.value)
        if value.kind is ItemDescriptorKind.DEFAULT:
            writer.write_short_le(value.item_id)
            if value.item_id != 0:
                writer.write_short_le(value.aux_value)
            return
        if value.kind is ItemDescriptorKind.MOLANG:
            writer.write_string(value.tag_expression)
            writer.write_byte(value.molang_version)
            return
        if value.kind is ItemDescriptorKind.ITEM_TAG:
            writer.write_string(value.item_tag)
            return
        if value.kind is ItemDescriptorKind.DEFERRED:
            writer.write_string(value.full_name)
            writer.write_short_le(value.aux_value)
            return
        if value.kind is ItemDescriptorKind.COMPLEX_ALIAS:
            writer.write_string(value.full_name)
            return


ITEM_DESCRIPTOR = _ItemDescriptorType()


@dataclass
class RecipeIngredient:
    descriptor: ItemDescriptor = field(default_factory=ItemDescriptor)
    count: int = 0


class _RecipeIngredientType(Type[RecipeIngredient]):
    def read(self, reader: PacketReader) -> RecipeIngredient:
        descriptor = ITEM_DESCRIPTOR.read(reader)
        count = reader.read_varint()
        return RecipeIngredient(descriptor=descriptor, count=count)

    def write(self, writer: PacketWriter, value: RecipeIngredient) -> None:
        ITEM_DESCRIPTOR.write(writer, value.descriptor)
        writer.write_varint(value.count)


RECIPE_INGREDIENT = _RecipeIngredientType()


class UnlockingContext(enum.IntEnum):
    NONE = 0
    ALWAYS_UNLOCKED = 1
    PLAYER_IN_WATER = 2
    PLAYER_HAS_MANY_ITEMS = 3


@dataclass
class RecipeUnlockingRequirement:
    context: UnlockingContext = UnlockingContext.NONE
    ingredients: list[RecipeIngredient] = field(default_factory=list)


class _RecipeUnlockingRequirementType(Type[RecipeUnlockingRequirement]):
    def read(self, reader: PacketReader) -> RecipeUnlockingRequirement:
        context = UnlockingContext(reader.read_byte())
        ingredients: list[RecipeIngredient] = []
        if context is UnlockingContext.NONE:
            n = reader.read_uvarint()
            ingredients = [RECIPE_INGREDIENT.read(reader) for _ in range(n)]
        return RecipeUnlockingRequirement(context=context, ingredients=ingredients)

    def write(self, writer: PacketWriter, value: RecipeUnlockingRequirement) -> None:
        writer.write_byte(int(value.context))
        if value.context is UnlockingContext.NONE:
            writer.write_uvarint(len(value.ingredients))
            for ingredient in value.ingredients:
                RECIPE_INGREDIENT.write(writer, ingredient)


RECIPE_UNLOCKING_REQUIREMENT = _RecipeUnlockingRequirementType()


@dataclass
class Recipe:
    recipe_id: str = ""
    uuid: bytes = b"\x00" * 16
    width: int = 0
    height: int = 0
    priority: int = 0
    recipe_net_id: int = 0
    ingredients: list[RecipeIngredient] = field(default_factory=list)
    results: list[ItemInstance] = field(default_factory=list)
    tag: str = ""
    requirement: RecipeUnlockingRequirement = field(default_factory=RecipeUnlockingRequirement)


@dataclass
class ShapelessRecipe(Recipe):
    pass


@dataclass
class ShapedRecipe(Recipe):
    assume_symmetry: bool = False


@dataclass
class MultiRecipe(Recipe):
    pass


@dataclass
class UserDataShapelessRecipe(ShapelessRecipe):
    pass


@dataclass
class ShapelessChemistryRecipe(ShapelessRecipe):
    pass


@dataclass
class ShapedChemistryRecipe(ShapedRecipe):
    pass


@dataclass
class SmithingTransformRecipe(ShapelessRecipe):
    """Wire format uses ``ingredients[0..2]`` and ``results[0]``."""


@dataclass
class SmithingTrimRecipe(ShapelessRecipe):
    """Wire format uses ``ingredients[0..2]``; no result on the wire."""


def _read_shapeless_body(reader: PacketReader, recipe: Recipe) -> None:
    recipe.recipe_id = reader.read_string()
    n = reader.read_uvarint()
    recipe.ingredients = [RECIPE_INGREDIENT.read(reader) for _ in range(n)]
    n = reader.read_uvarint()
    recipe.results = [NETWORK_ITEM_INSTANCE_DESCRIPTOR.read(reader) for _ in range(n)]
    recipe.uuid = reader.read_bytes(16)
    recipe.tag = reader.read_string()
    recipe.priority = reader.read_varint()
    recipe.requirement = RECIPE_UNLOCKING_REQUIREMENT.read(reader)
    recipe.recipe_net_id = reader.read_uvarint()


def _write_shapeless_body(writer: PacketWriter, value: Recipe) -> None:
    writer.write_string(value.recipe_id)
    writer.write_uvarint(len(value.ingredients))
    for ingredient in value.ingredients:
        RECIPE_INGREDIENT.write(writer, ingredient)
    writer.write_uvarint(len(value.results))
    for result in value.results:
        NETWORK_ITEM_INSTANCE_DESCRIPTOR.write(writer, result)
    if len(value.uuid) != 16:
        raise ValueError(f"Recipe.uuid must be 16 bytes (got {len(value.uuid)})")
    writer.write_bytes(value.uuid)
    writer.write_string(value.tag)
    writer.write_varint(value.priority)
    RECIPE_UNLOCKING_REQUIREMENT.write(writer, value.requirement)
    writer.write_uvarint(value.recipe_net_id)


def _read_shaped_body(reader: PacketReader, recipe: ShapedRecipe) -> None:
    recipe.recipe_id = reader.read_string()
    recipe.width = reader.read_varint()
    recipe.height = reader.read_varint()
    recipe.ingredients = [RECIPE_INGREDIENT.read(reader) for _ in range(recipe.width * recipe.height)]
    n = reader.read_uvarint()
    recipe.results = [NETWORK_ITEM_INSTANCE_DESCRIPTOR.read(reader) for _ in range(n)]
    recipe.uuid = reader.read_bytes(16)
    recipe.tag = reader.read_string()
    recipe.priority = reader.read_varint()
    recipe.assume_symmetry = reader.read_bool()
    recipe.requirement = RECIPE_UNLOCKING_REQUIREMENT.read(reader)
    recipe.recipe_net_id = reader.read_uvarint()


def _write_shaped_body(writer: PacketWriter, value: ShapedRecipe) -> None:
    writer.write_string(value.recipe_id)
    writer.write_varint(value.width)
    writer.write_varint(value.height)
    expected = value.width * value.height
    if len(value.ingredients) != expected:
        raise ValueError(f"ShapedRecipe ingredient count {len(value.ingredients)} != width*height {expected}")
    for ingredient in value.ingredients:
        RECIPE_INGREDIENT.write(writer, ingredient)
    writer.write_uvarint(len(value.results))
    for result in value.results:
        NETWORK_ITEM_INSTANCE_DESCRIPTOR.write(writer, result)
    if len(value.uuid) != 16:
        raise ValueError(f"Recipe.uuid must be 16 bytes (got {len(value.uuid)})")
    writer.write_bytes(value.uuid)
    writer.write_string(value.tag)
    writer.write_varint(value.priority)
    writer.write_bool(value.assume_symmetry)
    RECIPE_UNLOCKING_REQUIREMENT.write(writer, value.requirement)
    writer.write_uvarint(value.recipe_net_id)


class _ShapelessRecipeType(Type[ShapelessRecipe]):
    def read(self, reader: PacketReader) -> ShapelessRecipe:
        recipe = ShapelessRecipe()
        _read_shapeless_body(reader, recipe)
        return recipe

    def write(self, writer: PacketWriter, value: ShapelessRecipe) -> None:
        _write_shapeless_body(writer, value)


class _ShapedRecipeType(Type[ShapedRecipe]):
    def read(self, reader: PacketReader) -> ShapedRecipe:
        recipe = ShapedRecipe()
        _read_shaped_body(reader, recipe)
        return recipe

    def write(self, writer: PacketWriter, value: ShapedRecipe) -> None:
        _write_shaped_body(writer, value)


class _MultiRecipeType(Type[MultiRecipe]):
    def read(self, reader: PacketReader) -> MultiRecipe:
        uuid = reader.read_bytes(16)
        return MultiRecipe(uuid=uuid, recipe_net_id=reader.read_uvarint())

    def write(self, writer: PacketWriter, value: MultiRecipe) -> None:
        if len(value.uuid) != 16:
            raise ValueError(f"MultiRecipe.uuid must be 16 bytes (got {len(value.uuid)})")
        writer.write_bytes(value.uuid)
        writer.write_uvarint(value.recipe_net_id)


class _UserDataShapelessRecipeType(Type[UserDataShapelessRecipe]):
    def read(self, reader: PacketReader) -> UserDataShapelessRecipe:
        recipe = UserDataShapelessRecipe()
        _read_shapeless_body(reader, recipe)
        return recipe

    def write(self, writer: PacketWriter, value: UserDataShapelessRecipe) -> None:
        _write_shapeless_body(writer, value)


class _ShapelessChemistryRecipeType(Type[ShapelessChemistryRecipe]):
    def read(self, reader: PacketReader) -> ShapelessChemistryRecipe:
        recipe = ShapelessChemistryRecipe()
        _read_shapeless_body(reader, recipe)
        return recipe

    def write(self, writer: PacketWriter, value: ShapelessChemistryRecipe) -> None:
        _write_shapeless_body(writer, value)


class _ShapedChemistryRecipeType(Type[ShapedChemistryRecipe]):
    def read(self, reader: PacketReader) -> ShapedChemistryRecipe:
        recipe = ShapedChemistryRecipe()
        _read_shaped_body(reader, recipe)
        return recipe

    def write(self, writer: PacketWriter, value: ShapedChemistryRecipe) -> None:
        _write_shaped_body(writer, value)


class _SmithingTransformRecipeType(Type[SmithingTransformRecipe]):
    def read(self, reader: PacketReader) -> SmithingTransformRecipe:
        recipe = SmithingTransformRecipe()
        recipe.recipe_id = reader.read_string()
        recipe.ingredients = [RECIPE_INGREDIENT.read(reader) for _ in range(3)]
        recipe.results = [NETWORK_ITEM_INSTANCE_DESCRIPTOR.read(reader)]
        recipe.tag = reader.read_string()
        recipe.recipe_net_id = reader.read_uvarint()
        return recipe

    def write(self, writer: PacketWriter, value: SmithingTransformRecipe) -> None:
        if len(value.ingredients) != 3:
            raise ValueError(f"SmithingTransformRecipe must have 3 ingredients (got {len(value.ingredients)})")
        if len(value.results) != 1:
            raise ValueError(f"SmithingTransformRecipe must have 1 result (got {len(value.results)})")
        writer.write_string(value.recipe_id)
        for ingredient in value.ingredients:
            RECIPE_INGREDIENT.write(writer, ingredient)
        NETWORK_ITEM_INSTANCE_DESCRIPTOR.write(writer, value.results[0])
        writer.write_string(value.tag)
        writer.write_uvarint(value.recipe_net_id)


class _SmithingTrimRecipeType(Type[SmithingTrimRecipe]):
    def read(self, reader: PacketReader) -> SmithingTrimRecipe:
        recipe = SmithingTrimRecipe()
        recipe.recipe_id = reader.read_string()
        recipe.ingredients = [RECIPE_INGREDIENT.read(reader) for _ in range(3)]
        recipe.tag = reader.read_string()
        recipe.recipe_net_id = reader.read_uvarint()
        return recipe

    def write(self, writer: PacketWriter, value: SmithingTrimRecipe) -> None:
        if len(value.ingredients) != 3:
            raise ValueError(f"SmithingTrimRecipe must have 3 ingredients (got {len(value.ingredients)})")
        writer.write_string(value.recipe_id)
        for ingredient in value.ingredients:
            RECIPE_INGREDIENT.write(writer, ingredient)
        writer.write_string(value.tag)
        writer.write_uvarint(value.recipe_net_id)


SHAPELESS_RECIPE = _ShapelessRecipeType()
SHAPED_RECIPE = _ShapedRecipeType()
MULTI_RECIPE = _MultiRecipeType()
USER_DATA_SHAPELESS_RECIPE = _UserDataShapelessRecipeType()
SHAPELESS_CHEMISTRY_RECIPE = _ShapelessChemistryRecipeType()
SHAPED_CHEMISTRY_RECIPE = _ShapedChemistryRecipeType()
SMITHING_TRANSFORM_RECIPE = _SmithingTransformRecipeType()
SMITHING_TRIM_RECIPE = _SmithingTrimRecipeType()


RECIPE_CODECS: dict[CraftingDataEntryType, Type[Any]] = {
    CraftingDataEntryType.SHAPELESS_RECIPE: SHAPELESS_RECIPE,
    CraftingDataEntryType.SHAPED_RECIPE: SHAPED_RECIPE,
    CraftingDataEntryType.MULTI_RECIPE: MULTI_RECIPE,
    CraftingDataEntryType.USER_DATA_SHAPELESS_RECIPE: USER_DATA_SHAPELESS_RECIPE,
    CraftingDataEntryType.SHAPELESS_CHEMISTRY_RECIPE: SHAPELESS_CHEMISTRY_RECIPE,
    CraftingDataEntryType.SHAPED_CHEMISTRY_RECIPE: SHAPED_CHEMISTRY_RECIPE,
    CraftingDataEntryType.SMITHING_TRANSFORM_RECIPE: SMITHING_TRANSFORM_RECIPE,
    CraftingDataEntryType.SMITHING_TRIM_RECIPE: SMITHING_TRIM_RECIPE,
}


@dataclass
class CraftingDataEntry:
    recipe: Recipe | None
    item_data: int
    item_aux: int
    tag: str
    item_result: ItemInstance
    type: CraftingDataEntryType


class _CraftingDataEntryType(Type[CraftingDataEntry]):
    def read(self, reader: PacketReader) -> CraftingDataEntry:
        t = CraftingDataEntryType(reader.read_varint())
        if t is CraftingDataEntryType.FURNACE_RECIPE:
            item_data = reader.read_varint()
            item_result = NETWORK_ITEM_INSTANCE_DESCRIPTOR.read(reader)
            tag = reader.read_string()
            return CraftingDataEntry(
                recipe=None,
                item_data=item_data,
                item_aux=0,
                tag=tag,
                item_result=item_result,
                type=t,
            )
        if t is CraftingDataEntryType.FURNACE_AUX_RECIPE:
            item_data = reader.read_varint()
            item_aux = reader.read_varint()
            item_result = NETWORK_ITEM_INSTANCE_DESCRIPTOR.read(reader)
            tag = reader.read_string()
            return CraftingDataEntry(
                recipe=None,
                item_data=item_data,
                item_aux=item_aux,
                tag=tag,
                item_result=item_result,
                type=t,
            )
        recipe = RECIPE_CODECS[t].read(reader)
        return CraftingDataEntry(
            recipe=recipe,
            item_data=0,
            item_aux=0,
            tag="",
            item_result=ItemInstance(),
            type=t,
        )

    def write(self, writer: PacketWriter, value: CraftingDataEntry) -> None:
        writer.write_varint(value.type.value)
        t = value.type
        if t is CraftingDataEntryType.FURNACE_RECIPE:
            writer.write_varint(value.item_data)
            NETWORK_ITEM_INSTANCE_DESCRIPTOR.write(writer, value.item_result)
            writer.write_string(value.tag)
            return
        if t is CraftingDataEntryType.FURNACE_AUX_RECIPE:
            writer.write_varint(value.item_data)
            writer.write_varint(value.item_aux)
            NETWORK_ITEM_INSTANCE_DESCRIPTOR.write(writer, value.item_result)
            writer.write_string(value.tag)
            return

        assert value.recipe is not None
        RECIPE_CODECS[t].write(writer, value.recipe)


CRAFTING_DATA_ENTRY = _CraftingDataEntryType()
