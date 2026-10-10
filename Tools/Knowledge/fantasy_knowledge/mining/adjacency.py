"""Distinct top-level asset presence per tile. E/S physical pairs, N/W derived."""


def prepare(db):
    db.executescript('''
    CREATE TEMP TABLE presence(tile_id INTEGER,server_id INTEGER,PRIMARY KEY(tile_id,server_id)) WITHOUT ROWID;
    INSERT INTO presence SELECT tile_id,server_id FROM evidence.tile_items WHERE nesting_depth=0 GROUP BY tile_id,server_id;
    INSERT OR IGNORE INTO presence SELECT tile_id,ground_server_id FROM evidence.tiles WHERE ground_server_id IS NOT NULL;
    CREATE INDEX presence_asset ON presence(server_id,tile_id);
    ANALYZE temp.presence;
    ''')


def mine(db, progress):
    progress('COMPILE same-tile cooccurrence')
    db.execute('''INSERT INTO cooccurrence
    SELECT a.server_id,b.server_id,count(*) FROM presence a
    JOIN presence b ON b.tile_id=a.tile_id AND b.server_id>a.server_id
    GROUP BY a.server_id,b.server_id''')
    for dx,dy in ((1,0),(0,1)):
        progress(f'COMPILE adjacency dx={dx} dy={dy}')
        db.execute('''INSERT INTO adjacency
        SELECT a.server_id,b.server_id,?,?,0,count(*)
        FROM evidence.tiles t JOIN evidence.tiles n
        ON n.map_id=t.map_id AND n.x=t.x+? AND n.y=t.y+? AND n.z=t.z
        JOIN presence a ON a.tile_id=t.tile_id JOIN presence b ON b.tile_id=n.tile_id
        GROUP BY a.server_id,b.server_id''',(dx,dy,dx,dy))
        db.execute('''INSERT INTO adjacency SELECT asset_b,asset_a,-dx,-dy,0,count
        FROM adjacency WHERE dx=? AND dy=?''',(dx,dy))
    progress('COMPILE vertical relationships')
    db.execute('''INSERT INTO vertical_relationships
    SELECT a.server_id,b.server_id,1,count(*) FROM evidence.tiles t
    JOIN evidence.tiles n ON n.map_id=t.map_id AND n.x=t.x AND n.y=t.y AND n.z=t.z+1
    JOIN presence a ON a.tile_id=t.tile_id JOIN presence b ON b.tile_id=n.tile_id
    GROUP BY a.server_id,b.server_id''')
    db.execute('INSERT INTO vertical_relationships SELECT asset_b,asset_a,-1,count FROM vertical_relationships WHERE dz=1')
