import json
import xml.etree.ElementTree as ET


def import_houses(db, path):
    seen = set()
    root = None
    for event, node in ET.iterparse(path, events=('start', 'end')):
        if root is None:
            root = node
            if root.tag != 'houses':
                raise ValueError('House XML root <houses> is missing')
        if event != 'end' or node is root:
            continue
        if node.tag.lower() != 'house':
            db.execute('INSERT INTO diagnostics(context,code,raw_hex) VALUES(?,?,?)',
                       ('houses', 'unknown_xml_element', ET.tostring(node).hex()))
        else:
            a = node.attrib
            house_id = int(a.get('houseid', 0))
            if house_id <= 0 or house_id in seen:
                raise ValueError(f'Invalid or duplicate house id: {house_id}')
            seen.add(house_id)
            row = db.execute('INSERT INTO houses(map_id,house_id,name,entry_x,entry_y,entry_z,town_id,rent) VALUES(1,?,?,?,?,?,?,?)',
                (house_id, a.get('name', f'House #{house_id}'), *(int(a.get(k, 0)) for k in ('entryx','entryy','entryz','townid','rent')))).lastrowid
            db.execute('INSERT INTO sidecar_attributes(entity_type,entity_id,attributes_json) VALUES(?,?,?)',
                       ('house', row, json.dumps(a, sort_keys=True)))
        node.clear()
        root.clear()
