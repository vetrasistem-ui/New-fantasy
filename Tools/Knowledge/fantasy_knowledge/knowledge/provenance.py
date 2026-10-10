"""Reproducible query recipes tied to the exact Evidence fingerprint."""
import json

TABLE_KEYS = {
    'asset_usage': ('server_id',), 'asset_floor_usage': ('server_id','z'),
    'adjacency': ('asset_a','asset_b','dx','dy','dz'),
    'cooccurrence': ('asset_a','asset_b'),
    'ground_transitions': ('ground_a','ground_b','dx','dy'),
    'house_affinity': ('server_id',), 'line_patterns': ('server_id','axis'),
    'corner_patterns': ('server_id','pattern'),
    'interruption_patterns': ('interrupter_id','neighbor_family_a','neighbor_family_b','axis'),
    'vertical_relationships': ('asset_a','asset_b','dz'),
}
COUNT_COLUMNS = {
    'asset_usage':'occurrence_count','asset_floor_usage':'count','adjacency':'count',
    'cooccurrence':'count','ground_transitions':'count','house_affinity':'inside_count + outside_count',
    'line_patterns':'sequence_count','corner_patterns':'count',
    'interruption_patterns':'count','vertical_relationships':'count',
}


def record(db, evidence_hash):
    for table,columns in TABLE_KEYS.items():
        for row in db.execute(f"SELECT {','.join(columns)},{COUNT_COLUMNS[table]} FROM {table}"):
            key = json.dumps(list(row[:-1]),separators=(',',':'))
            recipe = json.dumps({'sha256':evidence_hash,'recipe':f'fkc-v1:{table}',
                                 'parameters':dict(zip(columns,row[:-1]))},sort_keys=True)
            db.execute('INSERT INTO knowledge_provenance(knowledge_type,knowledge_key,evidence_type,evidence_key,evidence_count,confidence) VALUES(?,?,?,?,?,?)',
                       (table,key,'evidence_query',recipe,row[-1],1.0))
    # At most three pointers per asset, never copied coordinates or regions.
    for (asset,) in db.execute('SELECT server_id FROM asset_usage'):
        for occurrence,tile in db.execute('SELECT occurrence_id,tile_id FROM evidence.tile_items WHERE server_id=? ORDER BY occurrence_id LIMIT 3',(asset,)):
            db.execute('INSERT INTO representative_occurrences(knowledge_type,knowledge_key,tile_id,occurrence_id,reason) VALUES(?,?,?,?,?)',
                       ('asset_usage',json.dumps([asset],separators=(',',':')),tile,occurrence,'First three raw occurrences by stable source occurrence id'))
    # Bound spatial examples to the 25 most frequent keys per pattern class.
    # Each pointer is verified against the same predicate used by the miner.
    for asset,pattern in db.execute('SELECT server_id,pattern FROM corner_patterns ORDER BY count DESC,server_id,pattern LIMIT 25'):
        dx,dy = {'NE':(1,-1),'NW':(-1,-1),'SE':(1,1),'SW':(-1,1)}[pattern]
        examples=db.execute('''SELECT p.tile_id FROM presence p
        JOIN evidence.tiles t ON t.tile_id=p.tile_id
        JOIN evidence.tiles h ON h.map_id=t.map_id AND h.x=t.x+? AND h.y=t.y AND h.z=t.z
        JOIN evidence.tiles v ON v.map_id=t.map_id AND v.x=t.x AND v.y=t.y+? AND v.z=t.z
        JOIN presence ph ON ph.tile_id=h.tile_id AND ph.server_id=p.server_id
        JOIN presence pv ON pv.tile_id=v.tile_id AND pv.server_id=p.server_id
        WHERE p.server_id=? ORDER BY p.tile_id LIMIT 3''',(dx,dy,asset))
        for (tile,) in examples:
            db.execute('INSERT INTO representative_occurrences(knowledge_type,knowledge_key,tile_id,reason) VALUES(?,?,?,?)',
                       ('corner_patterns',json.dumps([asset,pattern],separators=(',',':')),tile,'Center satisfies same-ID perpendicular-arm predicate'))
    for asset,neighbor_a,neighbor_b,axis in db.execute('''SELECT interrupter_id,neighbor_family_a,neighbor_family_b,axis
    FROM interruption_patterns ORDER BY count DESC,interrupter_id,neighbor_family_a,axis LIMIT 25'''):
        dx,dy=(1,0) if axis=='horizontal' else (0,1)
        examples=db.execute('''SELECT p.tile_id FROM presence p JOIN evidence.tiles t ON t.tile_id=p.tile_id
        JOIN evidence.tiles l ON l.map_id=t.map_id AND l.x=t.x-? AND l.y=t.y-? AND l.z=t.z
        JOIN evidence.tiles r ON r.map_id=t.map_id AND r.x=t.x+? AND r.y=t.y+? AND r.z=t.z
        JOIN presence a ON a.tile_id=l.tile_id AND a.server_id=?
        JOIN presence b ON b.tile_id=r.tile_id AND b.server_id=?
        WHERE p.server_id=? AND NOT EXISTS(SELECT 1 FROM presence m WHERE m.tile_id=t.tile_id AND m.server_id=?)
        ORDER BY p.tile_id LIMIT 3''',(dx,dy,dx,dy,neighbor_a,neighbor_b,asset,neighbor_a))
        for (tile,) in examples:
            db.execute('INSERT INTO representative_occurrences(knowledge_type,knowledge_key,tile_id,reason) VALUES(?,?,?,?)',
                       ('interruption_patterns',json.dumps([asset,neighbor_a,neighbor_b,axis],separators=(',',':')),tile,'Center satisfies exact identity-family A-X-A predicate'))
    # Candidate classifications are statistical, not semantic certainty.
    for (asset,total) in db.execute('SELECT server_id,sum(count) FROM corner_patterns GROUP BY server_id'):
        family = db.execute('INSERT INTO family_candidates(family_type,confidence,evidence_count,status) VALUES(?,?,?,?)',
                            ('corner_candidate',min(0.5,total/(total+100)),total,'candidate')).lastrowid
        confidence = min(0.5,total/(total+100))
        db.execute('INSERT INTO family_members VALUES(?,?,?,?)',(family,asset,'corner_candidate',confidence))
        db.execute('INSERT INTO knowledge_provenance(knowledge_type,knowledge_key,evidence_type,evidence_key,evidence_count,confidence) VALUES(?,?,?,?,?,?)',
                   ('family_candidates',json.dumps([family]),'corner_patterns',json.dumps({'server_id':asset,'evidence_sha256':evidence_hash}),total,confidence))
