"""Streaming import: bounded tile/item batches plus full source fingerprints."""
import json
import os
from pathlib import Path
from ..otbm.node_reader import NodeReader
from ..otbm.attributes import Cursor, item_attributes
from ..otbm.model import Item, Tile
from ..sidecars.houses import import_houses
from ..sidecars.spawns import import_spawns
from .fingerprint import fingerprint, verify
from .database import DDL, INDICES, SCHEMA, staging, publish


def import_evidence(otbm, evidence, house=None, spawn=None, batch_size=20000, progress=print):
    if not 100 <= batch_size <= 50000:
        raise ValueError('Batch size must be between 100 and 50000')
    paths = {'otbm': Path(otbm).resolve()}
    if house:
        paths['house'] = Path(house).resolve()
    if spawn:
        paths['spawn'] = Path(spawn).resolve()
    final = Path(evidence).resolve()
    if final in paths.values() or final.with_name(final.name+'.building') in paths.values():
        raise ValueError('Output must not overwrite an input')
    before = {kind: fingerprint(path) for kind,path in paths.items()}
    db, temporary = staging(final, DDL)
    tile_rows, item_rows, attr_rows, compact_rows = [], [], [], []
    tile_id = occurrence_id = 0
    nodes = []
    item_stack = []
    item_sibling_counts = {}
    tile = area = None
    map_seen = False
    ignored_depth = None

    def diagnostic(node, code, raw=None):
        db.execute('INSERT INTO diagnostics(context,code,raw_hex) VALUES(?,?,?)',
                   (f'byte:{node.offset};node:{node.kind};depth:{node.depth}',code,
                    (node.properties if raw is None else raw).hex()))

    def flush():
        db.executemany('INSERT INTO tiles VALUES(?,1,?,?,?,?,?,NULL)', tile_rows)
        db.executemany('INSERT INTO tile_items VALUES(?,?,?,?,?,?)', item_rows)
        db.executemany('INSERT INTO item_attributes(occurrence_id,ordinal,name,value_json) VALUES(?,?,?,?)', attr_rows)
        db.executemany('INSERT INTO compact_items VALUES(?)', compact_rows)
        db.commit()
        tile_rows.clear(); item_rows.clear(); attr_rows.clear(); compact_rows.clear()

    try:
        root = Path(__file__).resolve().parents[4]
        for kind,path in paths.items():
            db.execute('INSERT INTO sources(kind,path,sha256,byte_size) VALUES(?,?,?,?)',
                       (kind,os.path.relpath(path,root).replace('\\','/'),before[kind]['sha256'],before[kind]['byte_size']))
        db.executemany('INSERT INTO metadata VALUES(?,?)', [
            ('schema_version',SCHEMA),('ground_resolution','unresolved'),
            ('ground_resolution_reason','No asset registry supplied; all compact and child items retained in raw stack order.'),
            ('reader_provenance','Independent Python implementation of Fantasy OtbmStreamReader/OtbmReader/LegacyAuxXmlReader behavior'),
            ('string_encoding','latin-1 reversible source bytes'),('batch_size',str(batch_size))])
        with paths['otbm'].open('rb') as stream:
            for event,node in NodeReader(stream).events():
                parent = nodes[-1].kind if nodes else None
                if event == 'start':
                    nodes.append(node)
                    if ignored_depth is not None:
                        diagnostic(node,'unknown_subtree_node')
                        continue
                    c = Cursor(node.properties)
                    if node.depth == 0:
                        if node.kind not in (0,1):
                            raise ValueError('Unexpected OTBM root type')
                        header = (c.number('I'),c.number('H'),c.number('H'),c.number('I'),c.number('I'))
                        c.finish()
                        db.execute('INSERT INTO maps VALUES(1,1,?,?,?,?,?)',header)
                    elif node.kind == 2 and parent in (0,1) and node.depth == 1:
                        if map_seen:
                            raise ValueError('Duplicate MAP_DATA node')
                        map_seen = True
                        while c.pos < len(c.data):
                            start = c.pos
                            code = c.number('B')
                            key = {1:'description',11:'spawn_file',13:'house_file'}.get(code)
                            if not key:
                                diagnostic(node,f'unknown_map_attribute:{code}',c.data[start:]); break
                            db.execute('INSERT OR REPLACE INTO metadata VALUES(?,?)',(key,c.string()))
                    elif node.kind == 4 and parent == 2:
                        area = (c.number('H'),c.number('H'),c.number('B'))
                        c.finish()
                    elif node.kind in (5,14) and parent == 4 and area is not None:
                        if tile is not None:
                            raise ValueError('Nested tile')
                        tile = Tile(area[0]+c.number('B'),area[1]+c.number('B'),area[2],c.number('I') if node.kind==14 else 0)
                        if not 0 <= tile.z <= 15 or tile.x > 65535 or tile.y > 65535:
                            raise ValueError(f'Invalid tile coordinates: {tile}')
                        item_stack.clear(); item_sibling_counts.clear()
                        while c.pos < len(c.data):
                            start = c.pos
                            code = c.number('B')
                            if code == 3:
                                tile.flags = c.number('I')
                            elif code == 9:
                                tile.items.append(Item(c.number('H'),stack_index=len(tile.items),compact=True))
                            else:
                                diagnostic(node,f'unknown_tile_attribute:{code}',c.data[start:]); break
                    elif node.kind == 6 and tile is not None and parent in (5,14,6):
                        parent_index = item_stack[-1] if parent==6 and item_stack else None
                        stack_index = (item_sibling_counts.get(parent_index,0) if parent_index is not None
                                       else sum(item.nesting_depth==0 for item in tile.items))
                        item_sibling_counts[parent_index] = stack_index+1
                        item = Item(c.number('H'),parent_index,len(item_stack),stack_index)
                        item.attributes, unknown = item_attributes(c)
                        if unknown:
                            diagnostic(node,f'unknown_item_attribute:{unknown[0]};server:{item.server_id}',unknown[1])
                        item_stack.append(len(tile.items)); tile.items.append(item)
                    elif node.kind == 13 and parent == 12:
                        town = (c.number('I'),c.string(),*c.position()); c.finish()
                        db.execute('INSERT INTO towns(map_id,town_id,name,temple_x,temple_y,temple_z) VALUES(1,?,?,?,?,?)',town)
                    elif node.kind == 16 and parent == 15:
                        waypoint = (c.string(),*c.position()); c.finish()
                        db.execute('INSERT INTO waypoints(map_id,name,x,y,z) VALUES(1,?,?,?,?)',waypoint)
                    elif node.kind in (12,15) and parent == 2:
                        c.finish()
                    else:
                        diagnostic(node,'unknown_or_misplaced_node')
                        ignored_depth = node.depth
                else:
                    if ignored_depth is not None:
                        if node.depth == ignored_depth:
                            ignored_depth = None
                        nodes.pop()
                        continue
                    if node.kind == 6 and len(nodes)>1 and nodes[-2].kind in (5,14,6) and tile is not None:
                        item_stack.pop()
                    elif node.kind in (5,14) and tile is not None and len(nodes)>1 and nodes[-2].kind == 4:
                        tile_id += 1
                        tile_rows.append((tile_id,tile.x,tile.y,tile.z,tile.house_id,tile.flags))
                        local_ids = []
                        for item in tile.items:
                            occurrence_id += 1
                            parent_id = local_ids[item.parent_index] if item.parent_index is not None else None
                            local_ids.append(occurrence_id)
                            item_rows.append((occurrence_id,tile_id,item.stack_index,item.server_id,parent_id,item.nesting_depth))
                            if item.compact:
                                compact_rows.append((occurrence_id,))
                            for ordinal,(name,value) in enumerate(item.attributes):
                                attr_rows.append((occurrence_id,ordinal,name,json.dumps(value,ensure_ascii=True)))
                        tile = None
                        if len(tile_rows) >= batch_size or len(item_rows) >= batch_size:
                            flush()
                        if tile_id % 250000 == 0:
                            progress(f'IMPORT tiles={tile_id} items={occurrence_id}')
                    elif node.kind == 4:
                        area = None
                    nodes.pop()
        if not map_seen:
            raise ValueError('OTBM MAP_DATA node is missing')
        flush()
        if house:
            import_houses(db,paths['house'])
        if spawn:
            import_spawns(db,paths['spawn'])
        progress('IMPORT creating indices and checking integrity')
        db.executescript(INDICES)
        counts = {name:db.execute(f'SELECT count(*) FROM {name}').fetchone()[0]
                  for name in ('tiles','tile_items','towns','houses','spawn_areas','spawn_entries','diagnostics')}
        db.execute('INSERT INTO metadata VALUES(?,?)',('counts',json.dumps(counts,sort_keys=True)))
        db.execute('INSERT INTO metadata VALUES(?,?)',('inputs_byte_identical','yes'))
        publish(db,temporary,final,lambda:verify(paths,before))
        progress(f'IMPORT PASS {counts}')
        return counts
    except BaseException:
        db.close()
        # Keep staging for diagnosis; previous published database is untouched.
        raise
