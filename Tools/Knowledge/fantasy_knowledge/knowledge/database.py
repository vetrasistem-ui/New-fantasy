SCHEMA = 'fantasy-knowledge-v1'
DDL = '''
CREATE TABLE metadata(key TEXT PRIMARY KEY,value TEXT NOT NULL);
CREATE TABLE source_evidence(evidence_sha256 TEXT PRIMARY KEY,evidence_schema TEXT,generated_at TEXT);
CREATE TABLE asset_usage(server_id INTEGER PRIMARY KEY,occurrence_count INTEGER,tile_count INTEGER,house_occurrences INTEGER,non_house_occurrences INTEGER,floor_min INTEGER,floor_max INTEGER);
CREATE TABLE asset_floor_usage(server_id INTEGER,z INTEGER,count INTEGER,PRIMARY KEY(server_id,z));
CREATE TABLE adjacency(asset_a INTEGER,asset_b INTEGER,dx INTEGER,dy INTEGER,dz INTEGER,count INTEGER,PRIMARY KEY(asset_a,asset_b,dx,dy,dz));
CREATE TABLE cooccurrence(asset_a INTEGER,asset_b INTEGER,count INTEGER,PRIMARY KEY(asset_a,asset_b));
CREATE TABLE ground_transitions(ground_a INTEGER,ground_b INTEGER,dx INTEGER,dy INTEGER,count INTEGER,PRIMARY KEY(ground_a,ground_b,dx,dy));
CREATE TABLE house_affinity(server_id INTEGER PRIMARY KEY,inside_count INTEGER,outside_count INTEGER,ratio REAL);
CREATE TABLE line_patterns(server_id INTEGER,axis TEXT,sequence_count INTEGER,max_sequence INTEGER,mean_sequence REAL,PRIMARY KEY(server_id,axis));
CREATE TABLE corner_patterns(server_id INTEGER,pattern TEXT,count INTEGER,PRIMARY KEY(server_id,pattern));
CREATE TABLE interruption_patterns(interrupter_id INTEGER,neighbor_family_a INTEGER,neighbor_family_b INTEGER,axis TEXT,count INTEGER,PRIMARY KEY(interrupter_id,neighbor_family_a,neighbor_family_b,axis));
CREATE TABLE vertical_relationships(asset_a INTEGER,asset_b INTEGER,dz INTEGER,count INTEGER,PRIMARY KEY(asset_a,asset_b,dz));
CREATE TABLE family_candidates(family_id INTEGER PRIMARY KEY,family_type TEXT,confidence REAL,evidence_count INTEGER,status TEXT);
CREATE TABLE family_members(family_id INTEGER,server_id INTEGER,role TEXT,confidence REAL,PRIMARY KEY(family_id,server_id,role));
CREATE TABLE knowledge_provenance(provenance_id INTEGER PRIMARY KEY,knowledge_type TEXT,knowledge_key TEXT,evidence_type TEXT,evidence_key TEXT,evidence_count INTEGER,confidence REAL);
CREATE TABLE representative_occurrences(id INTEGER PRIMARY KEY,knowledge_type TEXT,knowledge_key TEXT,tile_id INTEGER,occurrence_id INTEGER,reason TEXT);
CREATE INDEX provenance_key ON knowledge_provenance(knowledge_type,knowledge_key);
CREATE INDEX representatives_key ON representative_occurrences(knowledge_type,knowledge_key);
'''
