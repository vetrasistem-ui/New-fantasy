def mine(db, ground_resolution):
    if ground_resolution != 'resolved':
        db.execute('INSERT INTO metadata VALUES(?,?)',('ground_transitions_status','unresolved: no ground registry; no inferred transitions'))
        return
    for dx,dy in ((1,0),(0,1)):
        db.execute('''INSERT INTO ground_transitions SELECT t.ground_server_id,n.ground_server_id,?,?,count(*)
        FROM evidence.tiles t JOIN evidence.tiles n
        ON n.map_id=t.map_id AND n.x=t.x+? AND n.y=t.y+? AND n.z=t.z
        WHERE t.ground_server_id IS NOT NULL AND n.ground_server_id IS NOT NULL
        AND t.ground_server_id!=n.ground_server_id GROUP BY t.ground_server_id,n.ground_server_id''',(dx,dy,dx,dy))
        db.execute('''INSERT INTO ground_transitions SELECT ground_b,ground_a,-dx,-dy,count
        FROM ground_transitions WHERE dx=? AND dy=?''',(dx,dy))
