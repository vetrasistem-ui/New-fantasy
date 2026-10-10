"""Small per-tile records; no in-memory global map model."""
from dataclasses import dataclass, field


@dataclass
class Item:
    server_id: int
    parent_index: int | None = None
    nesting_depth: int = 0
    stack_index: int = 0
    attributes: list = field(default_factory=list)
    compact: bool = False


@dataclass
class Tile:
    x: int
    y: int
    z: int
    house_id: int = 0
    flags: int = 0
    items: list = field(default_factory=list)
