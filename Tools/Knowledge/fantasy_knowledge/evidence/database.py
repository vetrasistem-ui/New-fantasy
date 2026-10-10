import sqlite3
from pathlib import Path

SCHEMA = 'fantasy-evidence-v1'
DDL = '''
CREATE TABLE metadata(key TEXT PRIMARY KEY,value TEXT NOT NULL);
CREATE TABLE sources(source_id INTEGER PRIMARY KEY,kind TEXT NOT NULL,path TEXT NOT NULL,sha256 TEXT NOT NULL,byte_size INTEGER NOT NULL);
CREATE TABLE maps(map_id INTEGER PRIMARY KEY,source_id INTEGER NOT NULL REFERENCES sources,format_version INTEGER,width INTEGER,height INTEGER,items_major INTEGER,items_minor INTEGER);
CREATE TABLE tiles(tile_id INTEGER PRIMARY KEY,map_id INTEGER NOT NULL REFERENCES maps,x INTEGER NOT NULL,y INTEGER NOT NULL,z INTEGER NOT NULL,house_id INTEGER NOT NULL DEFAULT 0,flags INTEGER NOT NULL DEFAULT 0,ground_server_id INTEGER,UNIQUE(map_id,x,y,z));
CREATE TABLE tile_items(occurrence_id INTEGER PRIMARY KEY,tile_id INTEGER NOT NULL REFERENCES tiles,stack_index INTEGER NOT NULL,server_id INTEGER NOT NULL,parent_occurrence_id INTEGER REFERENCES tile_items,nesting_depth INTEGER NOT NULL DEFAULT 0);
CREATE TABLE towns(town_row_id INTEGER PRIMARY KEY,map_id INTEGER NOT NULL REFERENCES maps,town_id INTEGER NOT NULL,name TEXT,temple_x INTEGER,temple_y INTEGER,temple_z INTEGER);
CREATE TABLE houses(house_row_id INTEGER PRIMARY KEY,map_id INTEGER NOT NULL REFERENCES maps,house_id INTEGER NOT NULL,name TEXT,entry_x INTEGER,entry_y INTEGER,entry_z INTEGER,town_id INTEGER,rent INTEGER);
CREATE TABLE spawn_areas(spawn_area_id INTEGER PRIMARY KEY,map_id INTEGER NOT NULL REFERENCES maps,center_x INTEGER,center_y INTEGER,center_z INTEGER,radius INTEGER);
CREATE TABLE spawn_entries(spawn_entry_id INTEGER PRIMARY KEY,spawn_area_id INTEGER NOT NULL REFERENCES spawn_areas,kind TEXT,name TEXT,x INTEGER,y INTEGER,z INTEGER,interval INTEGER);
CREATE TABLE waypoints(waypoint_id INTEGER PRIMARY KEY,map_id INTEGER NOT NULL REFERENCES maps,name TEXT,x INTEGER,y INTEGER,z INTEGER);
CREATE TABLE item_attributes(attribute_id INTEGER PRIMARY KEY,occurrence_id INTEGER NOT NULL REFERENCES tile_items,ordinal INTEGER,name TEXT,value_json TEXT);
CREATE TABLE compact_items(occurrence_id INTEGER PRIMARY KEY REFERENCES tile_items);
CREATE TABLE diagnostics(diagnostic_id INTEGER PRIMARY KEY,context TEXT NOT NULL,code TEXT NOT NULL,raw_hex TEXT NOT NULL);
CREATE TABLE sidecar_attributes(id INTEGER PRIMARY KEY,entity_type TEXT,entity_id INTEGER,attributes_json TEXT);
CREATE TABLE spawn_options(id INTEGER PRIMARY KEY,spawn_entry_id INTEGER REFERENCES spawn_entries,ordinal INTEGER,name TEXT,chance INTEGER,attributes_json TEXT);
'''
INDICES = '''
CREATE INDEX tiles_house ON tiles(house_id);
CREATE INDEX tiles_floor ON tiles(map_id,z,y,x);
CREATE INDEX items_tile ON tile_items(tile_id);
CREATE INDEX items_server ON tile_items(server_id);
CREATE INDEX towns_map ON towns(map_id,town_id);
CREATE INDEX houses_map ON houses(map_id,house_id);
CREATE INDEX spawn_position ON spawn_entries(x,y,z);
'''


def readonly(path):
    return sqlite3.connect(Path(path).resolve().as_uri() + '?mode=ro', uri=True)


def staging(path, ddl):
    path = Path(path)
    path.parent.mkdir(parents=True, exist_ok=True)
    temporary = path.with_name(path.name + '.building')
    if temporary.exists():
        raise FileExistsError(f"Unfinished staging exists; preserved: {temporary}")
    db = sqlite3.connect(temporary.resolve().as_uri(), uri=True)
    db.execute('PRAGMA journal_mode=DELETE')
    db.execute('PRAGMA synchronous=NORMAL')
    db.execute('PRAGMA temp_store=FILE')
    db.execute('PRAGMA cache_size=-65536')
    db.execute('PRAGMA foreign_keys=ON')
    db.executescript(ddl)
    return db, temporary


def integrity(db):
    result = db.execute('PRAGMA integrity_check').fetchall()
    if result != [('ok',)]:
        raise ValueError(f"SQLite integrity failed: {result}")
    if db.execute('PRAGMA foreign_key_check').fetchone():
        raise ValueError('SQLite foreign key check failed')
    return 'ok'


def publish(db, temporary, final, validate):
    db.commit()
    integrity(db)
    validate()
    db.close()
    Path(temporary).replace(final)
