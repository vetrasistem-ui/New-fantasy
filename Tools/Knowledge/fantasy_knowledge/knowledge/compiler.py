import json
from pathlib import Path
from ..evidence.database import staging, publish, readonly, integrity, SCHEMA as EVIDENCE_SCHEMA
from ..evidence.fingerprint import fingerprint
from ..mining import adjacency, transitions, structures
from .database import DDL, SCHEMA
from .provenance import record, TABLE_KEYS


def compile_knowledge(evidence, knowledge, progress=print):
    source,final = Path(evidence).resolve(),Path(knowledge).resolve()
    if source in (final,final.with_name(final.name+'.building')):
        raise ValueError('Knowledge output must not overwrite Evidence')
    before = fingerprint(source)
    with readonly(source) as check:
        metadata = dict(check.execute('SELECT key,value FROM metadata'))
        if metadata.get('schema_version') != EVIDENCE_SCHEMA:
            raise ValueError('Unsupported Evidence schema')
        integrity(check)
    check.close()
    db,temporary = staging(final,DDL)
    try:
        db.execute('ATTACH DATABASE ? AS evidence',(source.as_uri()+'?mode=ro',))
        db.executemany('INSERT INTO metadata VALUES(?,?)',[
            ('schema_version',SCHEMA),('ground_resolution',metadata.get('ground_resolution','unresolved')),
            ('spatial_scope','Distinct top-level presence; nested items count only in usage/floor/house statistics'),
            ('line_definition','Maximal contiguous same-ID runs of length >=2 in one map/floor/row or column'),
            ('corner_definition','Same-ID presence at center and both cardinal arms; evidence, not exclusivity or semantics'),
            ('interruption_definition','Identity-family A-X-A; A absent at center; X!=A'),
            ('house_ratio_definition','inside_occurrences / total_occurrences'),
            ('provenance_recipes','fkc-v1 recipes are defined in Tools/Knowledge/fantasy_knowledge/mining and compiler.py'),
            ('representative_limit','3 per asset; 3 per key for top 25 corner/interruption keys each; Evidence pointers only')])
        db.execute("INSERT INTO source_evidence VALUES(?,?,strftime('%Y-%m-%dT%H:%M:%SZ','now'))",(before['sha256'],EVIDENCE_SCHEMA))
        progress('COMPILE asset and floor usage')
        db.execute('''CREATE TEMP VIEW facts AS
        SELECT tile_id,server_id FROM evidence.tile_items
        UNION ALL SELECT tile_id,ground_server_id FROM evidence.tiles WHERE ground_server_id IS NOT NULL''')
        db.execute('''INSERT INTO asset_usage
        SELECT i.server_id,count(*),count(DISTINCT i.tile_id),
        sum(t.house_id!=0),sum(t.house_id=0),min(t.z),max(t.z)
        FROM facts i JOIN evidence.tiles t ON t.tile_id=i.tile_id GROUP BY i.server_id''')
        db.execute('''INSERT INTO asset_floor_usage SELECT i.server_id,t.z,count(*)
        FROM facts i JOIN evidence.tiles t ON t.tile_id=i.tile_id GROUP BY i.server_id,t.z''')
        db.execute('''INSERT INTO house_affinity SELECT server_id,house_occurrences,non_house_occurrences,
        CAST(house_occurrences AS REAL)/occurrence_count FROM asset_usage''')
        progress('COMPILE top-level presence projection')
        adjacency.prepare(db)
        adjacency.mine(db,progress)
        transitions.mine(db,metadata.get('ground_resolution','unresolved'))
        structures.mine_lines(db,progress)
        structures.mine_corners(db,progress)
        structures.mine_interruptions(db,progress)
        progress('COMPILE provenance and representative pointers')
        record(db,before['sha256'])
        counts = {table:db.execute(f'SELECT count(*) FROM {table}').fetchone()[0] for table in TABLE_KEYS}
        db.execute('INSERT INTO metadata VALUES(?,?)',('counts',json.dumps(counts,sort_keys=True)))
        def validate():
            if fingerprint(source) != before:
                raise ValueError('Evidence changed during compilation')
        progress('COMPILE integrity and fingerprint validation')
        publish(db,temporary,final,validate)
        progress(f'COMPILE PASS {counts}')
        return counts
    except BaseException:
        db.close()
        raise
