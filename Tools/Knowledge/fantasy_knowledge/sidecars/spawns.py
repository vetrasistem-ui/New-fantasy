import json
import xml.etree.ElementTree as ET


def import_spawns(db, path):
    root = None
    depth = 0
    for event, node in ET.iterparse(path, events=('start','end')):
        if event == 'start':
            depth += 1
        if root is None:
            root = node
            if root.tag != 'spawns':
                raise ValueError('Spawn XML root <spawns> is missing')
        if event != 'end':
            continue
        depth -= 1
        if depth != 1:
            continue
        if node.tag.lower() != 'spawn':
            db.execute('INSERT INTO diagnostics(context,code,raw_hex) VALUES(?,?,?)',
                       ('spawns','unknown_xml_element',ET.tostring(node).hex()))
            node.clear(); root.clear()
            continue
        a = node.attrib
        x,y,z = (int(a.get(k,0)) for k in ('centerx','centery','centerz'))
        if x == 0 or y == 0:
            raise ValueError('Spawn has invalid center position')
        area = db.execute('INSERT INTO spawn_areas(map_id,center_x,center_y,center_z,radius) VALUES(1,?,?,?,?)',
                          (x,y,z,int(a.get('radius',-1)))).lastrowid
        db.execute('INSERT INTO sidecar_attributes(entity_type,entity_id,attributes_json) VALUES(?,?,?)',
                   ('spawn',area,json.dumps(a,sort_keys=True)))
        for entry in node:
            kind = entry.tag.lower()
            if kind not in ('monster','npc','monsters'):
                db.execute('INSERT INTO diagnostics(context,code,raw_hex) VALUES(?,?,?)',
                           (f'spawn:{area}','unknown_xml_element',ET.tostring(entry).hex()))
                continue
            a = entry.attrib
            row = db.execute('INSERT INTO spawn_entries(spawn_area_id,kind,name,x,y,z,interval) VALUES(?,?,?,?,?,?,?)',
                             (area,kind,a.get('name',''),x+int(a.get('x',0)),y+int(a.get('y',0)),z,int(a.get('spawntime',0)))).lastrowid
            db.execute('INSERT INTO sidecar_attributes(entity_type,entity_id,attributes_json) VALUES(?,?,?)',
                       ('spawn_entry',row,json.dumps(a,sort_keys=True)))
            for ordinal, option in enumerate(entry):
                attributes = option.attrib
                db.execute('INSERT INTO spawn_options(spawn_entry_id,ordinal,name,chance,attributes_json) VALUES(?,?,?,?,?)',
                           (row,ordinal,attributes.get('name',''),int(attributes.get('chance',100//len(entry))),json.dumps(attributes,sort_keys=True)))
        node.clear()
        root.clear()
