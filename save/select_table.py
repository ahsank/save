# pip install msgpack, requests
import argparse, unittest, sys
import types
from skvclient import (CollectionMetadata, CollectionCapacity, SKVClient,
                       HashScheme, StorageDriver, Schema, SchemaField, FieldType, TimeDelta,
                       Operation, Value, Expression, TxnOptions, Txn)
import logging
from ast import literal_eval

def query(coll, schema, txnarg = None):
    if txnarg:
        txn = Txn(cl, literal_eval(txnarg))
    else:
        status, txn = cl.begin_txn()
        if not status.is2xxOK():
            raise Exception(status.message)
    print(txn.timestamp)
    status, query = txn.create_query(str.encode(coll), str.encode(schema))
    if not status.is2xxOK():
        raise Exception(status.message)
    status, records = txn.queryAll(query)
    if not status.is2xxOK():
        raise Exception(status.message)
    return records

def get_schema(coll, schema_name):
    status, schema = cl.get_schema(str.encode(coll), str.encode(schema_name))
    if not status.is2xxOK():
        raise Exception(status.message)
    return schema

def print_schema(schema):
    for (k, v) in schema.__dict__.items():
        if k == 'fields':
            print(f'{k}:')
            for item in v:
                print(f' {item.__dict__}')
        else:
            print(f'{k}:{v}')
                
def get_clusters():
    return query("K2RESVD_COLLECTION_SQL_PRIMARY_CLUSTER", "K2RESVD_SCHEMA_SQL_CLUSTER_META")

def get_databases():
    return query("K2RESVD_COLLECTION_SQL_PRIMARY_CLUSTER", "K2RESVD_SCHEMA_SQL_DATABASE_META")

def get_tables(coll, db_name):
    if not coll and db_name:
        dbs = get_databases()
        if dbs:
            db = next(d for d in dbs if d.data['DatabaseName'] == str.encode(db_name))
        else:
            db = None
        if db is None:
            raise Exception(f"Database {db_name} not found")
        coll = db.data['DatabaseId'].decode()
    return query(coll, "K2RESVD_SCHEMA_SQL_TABLE_META")

def print_records(records):
    for r in records:
        print(r.data)
            
if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument("command", help="Command")
    parser.add_argument("--http", default="http://localhost:30000", help="HTTP API URL")
    parser.add_argument("--coll", default="", help="Collection name")
    parser.add_argument("--schema", default="", help="Schema name")
    parser.add_argument("--db", default="template1", help="Database name")
    parser.add_argument("--txn")
    
    args = parser.parse_args()
    cl = SKVClient(args.http)
    if args.command == "query":
        records = query(args.coll, args.schema, args.txn)
        print_records(records)
    elif args.command == "get-schema":
        schema = get_schema(args.coll, args.schema)
        print_schema(schema)
    elif args.command == "get-clusters":
        clusters = get_clusters()
        print_records(cluters)
    elif args.command == "get-databases":
        databases = get_databases()
        print_records(databases)
    elif args.command == "get-tables":
        tables = get_tables(coll=args.coll, db_name=args.db)
        print_records(tables)
    else:
        raise Exception(f"Invalid command {args.command}")

