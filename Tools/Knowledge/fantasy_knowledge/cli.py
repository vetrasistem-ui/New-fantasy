import argparse
import json
import sys
from .evidence.importer import import_evidence
from .evidence.database import readonly
from .knowledge.compiler import compile_knowledge
from .knowledge.database import SCHEMA


def inspect_asset(path, asset):
    db = readonly(path)
    try:
        if db.execute("SELECT value FROM metadata WHERE key='schema_version'").fetchone() != (SCHEMA,):
            raise ValueError('Unsupported Knowledge schema')
        queries = {
            'usage':('SELECT * FROM asset_usage WHERE server_id=?',(asset,)),
            'floors':('SELECT * FROM asset_floor_usage WHERE server_id=? ORDER BY z',(asset,)),
            'house_affinity':('SELECT * FROM house_affinity WHERE server_id=?',(asset,)),
            'top_adjacent':('SELECT * FROM adjacency WHERE asset_a=? ORDER BY count DESC,asset_b,dx,dy LIMIT 20',(asset,)),
            'top_cooccurring':('SELECT * FROM cooccurrence WHERE asset_a=? OR asset_b=? ORDER BY count DESC,asset_a,asset_b LIMIT 20',(asset,asset)),
            'line_evidence':('SELECT * FROM line_patterns WHERE server_id=?',(asset,)),
            'corner_evidence':('SELECT * FROM corner_patterns WHERE server_id=?',(asset,)),
            'interruption_evidence':('SELECT * FROM interruption_patterns WHERE interrupter_id=? ORDER BY count DESC LIMIT 20',(asset,)),
            'representative_occurrences':('SELECT * FROM representative_occurrences WHERE knowledge_type=? AND knowledge_key=?',('asset_usage',json.dumps([asset],separators=(',',':'))))}
        result = {}
        for name,(sql,params) in queries.items():
            cursor = db.execute(sql,params)
            columns = [column[0] for column in cursor.description]
            result[name] = [dict(zip(columns,row)) for row in cursor]
        return result
    finally:
        db.close()


def main(argv=None):
    parser = argparse.ArgumentParser(description='Fantasy Knowledge Compiler V1')
    commands = parser.add_subparsers(dest='command',required=True)
    imp = commands.add_parser('import',help='Read-only OTBM/sidecars to Evidence')
    imp.add_argument('--otbm',required=True)
    imp.add_argument('--house')
    imp.add_argument('--spawn')
    imp.add_argument('--evidence',required=True)
    imp.add_argument('--batch-size',type=int,default=20000)
    comp = commands.add_parser('compile',help='Evidence to derived Knowledge')
    comp.add_argument('--evidence',required=True)
    comp.add_argument('--knowledge',required=True)
    ins = commands.add_parser('inspect')
    ins.add_argument('--knowledge',required=True)
    ins.add_argument('--asset',required=True,type=int)
    args = parser.parse_args(argv)
    try:
        if args.command == 'import':
            result = import_evidence(args.otbm,args.evidence,args.house,args.spawn,args.batch_size,
                                     progress=lambda line:print(line,flush=True))
        elif args.command == 'compile':
            result = compile_knowledge(args.evidence,args.knowledge,progress=lambda line:print(line,flush=True))
        else:
            result = inspect_asset(args.knowledge,args.asset)
        print(json.dumps(result,indent=2,ensure_ascii=True))
        return 0
    except Exception as error:
        print(f'FKC FAIL: {type(error).__name__}: {error}',file=sys.stderr)
        return 1
