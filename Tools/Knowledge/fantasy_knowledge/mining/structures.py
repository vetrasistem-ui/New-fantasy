"""V1 identity families: same server ID, without semantic names."""


class IdentityFamilyResolver:
    def family(self, server_id):
        return server_id


def mine_lines(db, progress, family_resolver=None):
    # Future families must materialize unique family presence before this step.
    if family_resolver is not None and not isinstance(family_resolver,IdentityFamilyResolver):
        raise NotImplementedError('Custom asset families require a family-presence projection')
    for axis, variable, fixed in (('horizontal','x','y'),('vertical','y','x')):
        progress(f'COMPILE lines {axis}')
        db.execute(f'''INSERT INTO line_patterns
        WITH ordered AS (
          SELECT p.server_id,t.map_id,t.z,t.{fixed} AS fixed,t.{variable} AS coordinate,
          t.{variable}-row_number() OVER(PARTITION BY p.server_id,t.map_id,t.z,t.{fixed} ORDER BY t.{variable}) AS run
          FROM presence p JOIN evidence.tiles t ON t.tile_id=p.tile_id
        ), runs AS (
          SELECT server_id,count(*) AS length FROM ordered
          GROUP BY server_id,map_id,z,fixed,run HAVING count(*)>=2
        ) SELECT server_id,?,count(*),max(length),avg(length) FROM runs GROUP BY server_id''',(axis,))


def mine_corners(db, progress):
    for pattern,(dx,dy) in {'NE':(1,-1),'NW':(-1,-1),'SE':(1,1),'SW':(-1,1)}.items():
        progress(f'COMPILE corners {pattern}')
        db.execute('''INSERT INTO corner_patterns
        SELECT p.server_id,?,count(*) FROM presence p JOIN evidence.tiles t ON t.tile_id=p.tile_id
        JOIN evidence.tiles h ON h.map_id=t.map_id AND h.x=t.x+? AND h.y=t.y AND h.z=t.z
        JOIN evidence.tiles v ON v.map_id=t.map_id AND v.x=t.x AND v.y=t.y+? AND v.z=t.z
        JOIN presence ph ON ph.tile_id=h.tile_id AND ph.server_id=p.server_id
        JOIN presence pv ON pv.tile_id=v.tile_id AND pv.server_id=p.server_id
        GROUP BY p.server_id''',(pattern,dx,dy))


def mine_interruptions(db, progress):
    for axis,dx,dy in (('horizontal',1,0),('vertical',0,1)):
        progress(f'COMPILE interruptions {axis}')
        # Exact A-X-A, with A absent at center. Multiple distinct center IDs
        # yield distinct evidence. Family columns are identity IDs in V1.
        db.execute('''INSERT INTO interruption_patterns
        SELECT p.server_id,a.server_id,a.server_id,?,count(*) FROM evidence.tiles t
        JOIN evidence.tiles l ON l.map_id=t.map_id AND l.x=t.x-? AND l.y=t.y-? AND l.z=t.z
        JOIN evidence.tiles r ON r.map_id=t.map_id AND r.x=t.x+? AND r.y=t.y+? AND r.z=t.z
        JOIN presence a ON a.tile_id=l.tile_id
        JOIN presence b ON b.tile_id=r.tile_id AND b.server_id=a.server_id
        JOIN presence p ON p.tile_id=t.tile_id AND p.server_id!=a.server_id
        WHERE NOT EXISTS(SELECT 1 FROM presence middle WHERE middle.tile_id=t.tile_id AND middle.server_id=a.server_id)
        GROUP BY p.server_id,a.server_id''',(axis,dx,dy,dx,dy))
