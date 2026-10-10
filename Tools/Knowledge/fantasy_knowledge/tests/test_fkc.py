import io
import json
from pathlib import Path
import sqlite3
import struct
import sys
import tempfile
import unittest
from unittest.mock import patch

sys.path.insert(0,str(Path(__file__).resolve().parents[2]))
from fantasy_knowledge.otbm.node_reader import NodeReader
from fantasy_knowledge.evidence.importer import import_evidence
from fantasy_knowledge.evidence.database import integrity
from fantasy_knowledge.evidence.fingerprint import fingerprint
from fantasy_knowledge.knowledge.compiler import compile_knowledge
from fantasy_knowledge.cli import inspect_asset
from fantasy_knowledge.mining.regions import RegionMiner


def u16(value):
    return struct.pack('<H',value)


def string(value):
    raw=value.encode('latin-1')
    return u16(len(raw))+raw


def node(kind,properties=b'',children=()):
    escaped=b''.join(bytes((253,b)) if b in (253,254,255) else bytes((b,)) for b in properties)
    return bytes((254,kind))+escaped+b''.join(children)+b'\xff'


def fixture(tiles=None,extra=()):
    if tiles is None:
        # A-X-A along x; corners for A on a second row; nested container.
        tiles=[
            (0,0,0,[10]),(1,0,0,[20]),(2,0,0,[10]),
            (0,1,4,[10,30]),(1,1,0,[10]),(2,1,0,[10])]
    children=[]
    for x,y,house,items in tiles:
        item_nodes=[]
        for sid in items[1:]:
            contents=[node(6,u16(31)+b'\x06'+string('nested'))] if sid==30 else []
            item_nodes.append(node(6,u16(sid)+b'\x04'+u16(254),contents))
        props=bytes((x,y))+(struct.pack('<I',house) if house else b'')+b'\x03'+struct.pack('<I',1)
        if items:
            props+=b'\x09'+u16(items[0])
        children.append(node(14 if house else 5,props,item_nodes))
    town=node(13,struct.pack('<I',3)+string('Town')+u16(101)+u16(201)+b'\x07')
    waypoint=node(16,string('point')+u16(102)+u16(202)+b'\x07')
    area=node(4,u16(100)+u16(200)+b'\x07',children)
    root=node(0,struct.pack('<IHHII',2,64,64,3,57),[
        node(2,b'\x01'+string('Synthetic'),[area,node(12,children=[town]),node(15,children=[waypoint]),*extra])])
    return b'OTBM'+root


class PhysicalReaderTests(unittest.TestCase):
    def test_escaping_across_chunks(self):
        raw=b'\0'*4+node(0,b'\xfd\xfe\xff', [node(6,b'abc')])
        events=list(NodeReader(io.BytesIO(raw),chunk_size=1).events())
        self.assertEqual(events[0][1].properties,b'\xfd\xfe\xff')
        self.assertEqual([event for event,_ in events],['start','start','end','end'])
        self.assertEqual(events[1][1].depth,1)

    def test_truncated_and_trailing(self):
        for raw in (fixture()[:-1],fixture()+b'garbage',b'OTBM\xfe',b'OTBM\xfe\0\xfd'):
            with self.subTest(raw=raw[-10:]),self.assertRaises(ValueError):
                list(NodeReader(io.BytesIO(raw),chunk_size=3).events())

    def test_never_reads_whole_file(self):
        class Bounded(io.BytesIO):
            def read(self,size=-1):
                if size<0 or size>31:
                    raise AssertionError('Unbounded read')
                return super().read(size)
        self.assertGreater(len(list(NodeReader(Bounded(fixture()),chunk_size=31).events())),10)


class DatabaseTests(unittest.TestCase):
    def setUp(self):
        test_root=Path(__file__).resolve().parents[4]/'build'/'knowledge-unit-tests'
        test_root.mkdir(parents=True,exist_ok=True)
        self.temporary=tempfile.TemporaryDirectory(dir=test_root)
        self.root=Path(self.temporary.name)
        self.otbm=self.root/'map.otbm'; self.otbm.write_bytes(fixture())
        self.house=self.root/'house.xml'
        self.house.write_text('<houses><house houseid="4" name="Home" entryx="100" entryy="201" entryz="7" townid="3" rent="8" guildhall="1"/></houses>')
        self.spawn=self.root/'spawn.xml'
        self.spawn.write_text('<spawns><spawn centerx="101" centery="201" centerz="7" radius="2"><monster name="Rat" x="-1" y="1" z="99" spawntime="60"/><monsters x="0" y="0"><monster name="A" chance="40"/><monster name="B" chance="60"/></monsters></spawn></spawns>')
        self.evidence=self.root/'FantasyEvidence.sqlite'
        self.knowledge=self.root/'FantasyKnowledge.sqlite'

    def tearDown(self):
        self.temporary.cleanup()

    def build(self):
        return import_evidence(self.otbm,self.evidence,self.house,self.spawn,100,lambda _:None)

    def test_coordinates_normal_house_stack_nesting_town(self):
        before=fingerprint(self.otbm)
        counts=self.build()
        with sqlite3.connect(self.evidence) as db:
            self.assertEqual(db.execute('SELECT x,y,z,house_id,flags FROM tiles WHERE house_id=4').fetchone(),(100,201,7,4,1))
            self.assertEqual(db.execute('SELECT server_id,stack_index,nesting_depth FROM tile_items WHERE tile_id=4 ORDER BY occurrence_id').fetchall(),[(10,0,0),(30,1,0),(31,0,1)])
            parent=db.execute('SELECT parent_occurrence_id FROM tile_items WHERE server_id=31').fetchone()[0]
            self.assertEqual(db.execute('SELECT server_id FROM tile_items WHERE occurrence_id=?',(parent,)).fetchone()[0],30)
            self.assertEqual(db.execute('SELECT town_id,name,temple_x,temple_y,temple_z FROM towns').fetchone(),(3,'Town',101,201,7))
            self.assertEqual(db.execute('SELECT name FROM waypoints').fetchone()[0],'point')
            self.assertEqual(db.execute("SELECT value FROM metadata WHERE key='ground_resolution'").fetchone()[0],'unresolved')
            self.assertEqual(db.execute('SELECT count(*) FROM tiles WHERE ground_server_id IS NOT NULL').fetchone()[0],0)
            self.assertEqual(integrity(db),'ok')
        db.close()
        self.assertEqual(counts['tiles'],6)
        self.assertEqual(counts['tile_items'],8)
        self.assertEqual(fingerprint(self.otbm),before)

    def test_house_spawn_sidecar_details(self):
        self.build()
        with sqlite3.connect(self.evidence) as db:
            self.assertEqual(db.execute('SELECT house_id,rent,town_id FROM houses').fetchone(),(4,8,3))
            self.assertEqual(db.execute('SELECT x,y,z,interval FROM spawn_entries WHERE kind="monster"').fetchone(),(100,202,7,60))
            self.assertEqual(db.execute('SELECT name,chance FROM spawn_options ORDER BY ordinal').fetchall(),[('A',40),('B',60)])
            self.assertIn('guildhall',db.execute('SELECT attributes_json FROM sidecar_attributes WHERE entity_type="house"').fetchone()[0])
        db.close()

    def test_schema_indices_constraints(self):
        self.build()
        with sqlite3.connect(self.evidence) as db:
            self.assertGreaterEqual(len(db.execute("SELECT name FROM sqlite_master WHERE type='index'").fetchall()),8)
            self.assertEqual(db.execute('PRAGMA foreign_key_check').fetchall(),[])
            with self.assertRaises(sqlite3.IntegrityError):
                db.execute('INSERT INTO tiles(map_id,x,y,z) VALUES(1,100,200,7)')
        db.close()

    def test_atomic_publication_preserves_previous(self):
        self.build(); before=fingerprint(self.evidence)
        with patch('fantasy_knowledge.evidence.importer.verify',side_effect=ValueError('changed input')):
            with self.assertRaisesRegex(ValueError,'changed input'):
                self.build()
        self.assertEqual(fingerprint(self.evidence),before)
        self.assertTrue(self.evidence.with_name(self.evidence.name+'.building').exists())
        with self.assertRaises(FileExistsError):
            self.build()

    def test_invalid_inputs_do_not_publish(self):
        self.otbm.write_bytes(fixture()[:-1])
        with self.assertRaises(ValueError):
            self.build()
        self.assertFalse(self.evidence.exists())

    def test_unknown_nodes_and_attributes_are_preserved(self):
        self.otbm.write_bytes(fixture(extra=[node(99,b'\xfe\xff')]))
        self.build()
        with sqlite3.connect(self.evidence) as db:
            self.assertEqual(db.execute('SELECT raw_hex FROM diagnostics').fetchone()[0],'feff')
            self.assertEqual(db.execute("SELECT value_json FROM item_attributes WHERE name='actionId'").fetchone()[0],'254')
        db.close()

    def test_unknown_subtree_is_not_interpreted_as_items(self):
        extra=node(99,b'opaque',[node(2,children=[node(4,u16(0)+u16(0)+b'\x07',[node(5,b'\0\0\x09'+u16(999))])])])
        self.otbm.write_bytes(fixture(extra=[extra]))
        counts=self.build()
        self.assertEqual(counts['tiles'],6)
        self.assertEqual(counts['diagnostics'],4)

    def test_integrity_check_rejects_invalid_foreign_keys(self):
        self.build()
        with sqlite3.connect(self.evidence) as db:
            db.execute('INSERT INTO tile_items VALUES(999,999,0,55,NULL,0)')
            with self.assertRaisesRegex(ValueError,'foreign key'):
                integrity(db)
        db.close()

    def test_resolved_ground_transitions(self):
        self.build()
        with sqlite3.connect(self.evidence) as db:
            db.execute("UPDATE metadata SET value='resolved' WHERE key='ground_resolution'")
            db.execute('UPDATE tiles SET ground_server_id=CASE WHEN x=100 THEN 500 ELSE 501 END')
        db.close()
        compile_knowledge(self.evidence,self.knowledge,lambda _:None)
        with sqlite3.connect(self.knowledge) as db:
            self.assertEqual(db.execute('SELECT count FROM ground_transitions WHERE ground_a=500 AND ground_b=501 AND dx=1').fetchone()[0],2)
            self.assertEqual(db.execute('SELECT count FROM ground_transitions WHERE ground_a=501 AND ground_b=500 AND dx=-1').fetchone()[0],2)
        db.close()

    def test_usage_adjacency_cooccurrence_lines_corners_interruptions_provenance(self):
        self.build()
        before=fingerprint(self.evidence)
        compile_knowledge(self.evidence,self.knowledge,lambda _:None)
        with sqlite3.connect(self.knowledge) as db:
            self.assertEqual(db.execute('SELECT occurrence_count,tile_count,house_occurrences FROM asset_usage WHERE server_id=10').fetchone(),(5,5,1))
            self.assertEqual(db.execute('SELECT count FROM adjacency WHERE asset_a=10 AND asset_b=20 AND dx=1 AND dy=0').fetchone()[0],1)
            self.assertEqual(db.execute('SELECT count FROM adjacency WHERE asset_a=20 AND asset_b=10 AND dx=-1 AND dy=0').fetchone()[0],1)
            self.assertEqual(db.execute('SELECT count FROM cooccurrence WHERE asset_a=10 AND asset_b=30').fetchone()[0],1)
            self.assertIsNone(db.execute('SELECT count FROM cooccurrence WHERE asset_b=31').fetchone())
            self.assertEqual(db.execute('SELECT sequence_count,max_sequence,mean_sequence FROM line_patterns WHERE server_id=10 AND axis="horizontal"').fetchone(),(1,3,3.0))
            self.assertEqual(db.execute('SELECT count FROM corner_patterns WHERE server_id=10 AND pattern="NE"').fetchone()[0],1)
            self.assertEqual(db.execute('SELECT count FROM interruption_patterns WHERE interrupter_id=20 AND neighbor_family_a=10 AND axis="horizontal"').fetchone()[0],1)
            self.assertEqual(db.execute('SELECT count(*) FROM ground_transitions').fetchone()[0],0)
            self.assertGreater(db.execute('SELECT count(*) FROM knowledge_provenance').fetchone()[0],0)
            self.assertGreater(db.execute("SELECT count(*) FROM representative_occurrences WHERE knowledge_type='interruption_patterns'").fetchone()[0],0)
            self.assertLessEqual(db.execute('SELECT count(*) FROM representative_occurrences WHERE knowledge_key="[10]"').fetchone()[0],3)
            self.assertEqual(integrity(db),'ok')
        db.close()
        self.assertEqual(fingerprint(self.evidence),before)
        self.assertEqual(inspect_asset(self.knowledge,10)['usage'][0]['occurrence_count'],5)

    def test_vertical_relationships(self):
        # Separate tile areas at the same x/y on consecutive floors.
        root=node(0,struct.pack('<IHHII',2,64,64,3,57),[node(2,children=[
            node(4,u16(100)+u16(200)+bytes((z,)),[node(5,b'\0\0\x09'+u16(sid))])
            for z,sid in ((7,10),(8,20))])])
        self.otbm.write_bytes(b'OTBM'+root)
        self.build(); compile_knowledge(self.evidence,self.knowledge,lambda _:None)
        with sqlite3.connect(self.knowledge) as db:
            self.assertEqual(db.execute('SELECT * FROM vertical_relationships ORDER BY dz').fetchall(),[(20,10,-1,1),(10,20,1,1)])
        db.close()

    def test_knowledge_publication_preserves_previous(self):
        self.build(); compile_knowledge(self.evidence,self.knowledge,lambda _:None)
        before=fingerprint(self.knowledge)
        with patch('fantasy_knowledge.knowledge.compiler.fingerprint',side_effect=[fingerprint(self.evidence),{'changed':True}]):
            with self.assertRaisesRegex(ValueError,'Evidence changed'):
                compile_knowledge(self.evidence,self.knowledge,lambda _:None)
        self.assertEqual(fingerprint(self.knowledge),before)

    def test_output_cannot_overwrite_source(self):
        with self.assertRaises(ValueError):
            import_evidence(self.otbm,self.otbm)
        self.build()
        with self.assertRaises(ValueError):
            compile_knowledge(self.evidence,self.evidence)

    def test_duplicate_house_is_rejected(self):
        self.house.write_text('<houses><house houseid="1"/><house houseid="1"/></houses>')
        with self.assertRaisesRegex(ValueError,'duplicate'):
            self.build()

    def test_unknown_spawn_xml_is_reported(self):
        self.spawn.write_text('<spawns><future attr="preserve"><child/></future></spawns>')
        self.build()
        with sqlite3.connect(self.evidence) as db:
            context,code,raw=db.execute('SELECT context,code,raw_hex FROM diagnostics').fetchone()
            self.assertEqual((context,code),('spawns','unknown_xml_element'))
            self.assertIn(b'preserve',bytes.fromhex(raw))
        db.close()

    def test_region_interface_has_eight_neighbors(self):
        self.assertEqual(len(RegionMiner.neighbors),8)
        with self.assertRaises(NotImplementedError):
            RegionMiner().mine(None,None)


if __name__=='__main__':
    unittest.main()
