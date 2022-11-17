#!/usr/bin/env python3
# pip install msgpack, requests
import argparse, unittest, sys
import types
from skvclient import (CollectionMetadata, CollectionCapacity, SKVClient,
                       HashScheme, StorageDriver, Schema, SchemaField, FieldType, TimeDelta,
                       Operation, Value, Expression, TxnOptions, Txn)
import logging
from ast import literal_eval

helpmsg = """
Examples:

python3 ogskvquery.py get-databases # get all database info

python3 ogskvquery.py get-clusters # get all cluster info

python3 ogskvquery.py get-tables # get all tables

python3 ogskvquery.py get-schema  --toid 1233 # get schema by table oid

python3 ogskvquery.py get-schema  --table pg_depend # get schema by table name

python3 ogskvquery.py get-schema --schema 00000000000030008000000000000a30

python3 ogskvquery.py query  --table pg_depend # query by table name

python3 ogskvquery.py query  --toid 1233 # query by table oid

python3 ogskvquery.py query --schema 00000000000030008000000000000a30  # query by schema name

To specify remote sever use --http 'http://remote_server:port' option
"""

def check_status(status):
    if not status.is2xxOK():
        raise Exception(status.message)

class WrappedTxn:
    def __init__(self, skvtxn = None):
        self.txn = skvtxn
        self.do_cleanup = False
    
    def __enter__(self):
        # print(f"With txn {self.txn}")
        if not self.txn:
            status, self.txn = cl.begin_txn(TxnOptions(timeout=TimeDelta(seconds=60)))
            check_status(status)
            self.do_cleanup = True
        return self

    def create_query(self, coll, schema):
        status, query = self.txn.create_query(str.encode(coll), str.encode(schema))
        if not status.is2xxOK():
            raise Exception(status.message)
        return query
        
    def queryAll(self, query):
        status, records = self.txn.queryAll(query)
        check_status(status)
        return records

    def write(self, collection, record, erase=False):
        status = self.txn.write(str.encode(collection), record, erase)
        check_status(status)
     
    def __exit__(self, exc_type, exc_value, traceback):
        # print(f"Cleanup {self.txn} {self.do_cleanup}")
        if self.do_cleanup and self.txn:
            if exc_type:
                # print(f"Aborting txn {self.txn}")
                self.txn.end(False)
            else:
                # print("Commiting txn {txn}")
                self.txn.end(True)
                
def query(coll, schema, txnarg = None):
    with WrappedTxn(txnarg) as wtxn:
        query = wtxn.create_query(coll, schema)
        records = wtxn.queryAll(query)
    return records

def delete_table_data(coll, table, indices):
    with WrappedTxn() as wtxn:
        schema_name = table.data['TableId'].decode()
        records = query(coll, schema_name, wtxn.txn)
        for record in records:
            print(f'deleting {record.data}')
            wtxn.write(coll, record, True)
        for idx in indices:
            idx_recods = query(coll, idx.data['TableId'].decode(), wtxn.txn)
            for idxrec in idx_recods:
                print(f'deleting {idxrec.data}')
                wtxn.write(coll, idxrec, True)

def get_table(tables, table_oid=None, table_name=None):
    table = None
    if table_oid:
        table = [t for t in tables if int(t.data['TableOid']) == int(table_oid)]
    elif table_name:
        table = [t for t in tables if t.data['TableName'].decode() == table_name]

    if not table:
        raise Exception(f"Table with oid {table_oid} not found")
    return next(t for t in table)

    
def get_table_info(tables, table_oid=None, table_name=None):
    table = get_table(tables, table_oid, table_name)
    if table.data['IsIndex']:
        base_table =  next(t for t in tables if t.data['TableId'] == table.data['BaseTableId'])
        return False, table, base_table
    else:
        indices = [t for t in tables if t.data['BaseTableId'] == table.data['TableId']]
        return True, table, indices

def print_table(coll, table):
    print(table.data)
    coll, schema = get_schema(coll, table.data['TableId'].decode())
    print_schema(schema)


def get_schema_name(coll, schema_name,  table_oid=None, table_name=None, database=None):
    if not coll and database:
        coll = get_db_coll(database)
    
    if database and not schema_name:
        tables, _ = get_tables(db_name=database)
        table = get_table(tables, table_oid, table_name)
        schema_name = table.data['TableId'].decode()

    if not coll:
        raise Exception("Cannot determine collection")
    if not schema_name:
        raise Exception("Cannot determine schema name")
    return coll, schema_name

def get_schema(coll, schema_name,  table_oid=None, table_name=None, database=None):
    coll, schema_name = get_schema_name(coll, schema_name, table_oid, table_name, database)
    status, schema = cl.get_schema(str.encode(coll), str.encode(schema_name))
    if not status.is2xxOK():
        raise Exception(status.message)
    return coll, schema

    
def print_schema(schema):
    for (k, v) in schema.__dict__.items():
        if k == 'fields':
            print(f'{k}:')
            for item in v:
                print(f' {item.__dict__}')
        else:
            print(f'{k}:{v}')
                
def get_clusters():
    return query(coll="K2RESVD_COLLECTION_SQL_PRIMARY_CLUSTER", schema="K2RESVD_SCHEMA_SQL_CLUSTER_META")

def get_databases():
    return query(coll="K2RESVD_COLLECTION_SQL_PRIMARY_CLUSTER", schema="K2RESVD_SCHEMA_SQL_DATABASE_META")

def get_db_coll(db_name):
    dbs = get_databases()
    if dbs:
        db = next(d for d in dbs if d.data['DatabaseName'] == str.encode(db_name))
    else:
        db = None
    if db is None:
        raise Exception(f"Database {db_name} not found")
    return db.data['DatabaseId'].decode()
    
def get_tables(coll=None, db_name=None):
    if not coll and db_name:
        coll = get_db_coll(db_name)
    return query(coll=coll, schema="K2RESVD_SCHEMA_SQL_TABLE_META"), coll

def print_records(records):
    for r in records:
        print(r.data)

if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument("command", help="Command")
    parser.add_argument("--http", default="http://172.17.0.1:30000", help="HTTP API URL")
    parser.add_argument("--coll", default="", help="Collection name")
    parser.add_argument("--schema", default="", help="Schema name")
    parser.add_argument("--db", default="template1", help="Database name")
    parser.add_argument("--toid", help="Table oid")
    parser.add_argument("--table", help="Table name")
    parser.add_argument("--txn")
    
    args = parser.parse_args()
    cl = SKVClient(args.http)
    if args.command == "query":
        coll, schema_name = get_schema_name(coll=args.coll, schema_name=args.schema,
                        table_oid=args.toid, table_name=args.table,
                        database=args.db)
        records = query(coll=coll, schema=schema_name,
                        txnarg=args.txn)
        print_records(records)
    elif args.command == "get-schema":
        coll, schema = get_schema(coll=args.coll, schema_name=args.schema,
                            table_oid=args.toid, table_name=args.table,
                            database=args.db)
        print_schema(schema)
    elif args.command == "get-clusters":
        clusters = get_clusters()
        print_records(clusters)
    elif args.command == "get-databases":
        databases = get_databases()
        print_records(databases)
    elif args.command == "get-tables":
        tables, _ = get_tables(coll=args.coll, db_name=args.db)
        print_records(tables)
    elif args.command == "get-table-info":
        tables, coll = get_tables(coll=args.coll, db_name=args.db)
        is_table, table, ref = get_table_info(tables=tables, table_oid=args.toid, table_name=args.table)
        print('Table:' if is_table else 'Index')
        print_table(coll, table)
        if is_table:
            print('Indices:')
            for i in ref:
                print('Index:')
                print_table(coll, i)
        else:
            print('Base table:')
            print_table(coll, ref)
    elif args.command == "delete":
        tables, coll = get_tables(coll=args.coll, db_name=args.db)
        is_table, table, ref = get_table_info(tables=tables, table_oid=args.toid, table_name=args.table)
        if not is_table:
            raise Exception("Cannot delete data from index")
        delete_table_data(coll, table, ref);
    elif args.command == "usage":
        print(helpmsg)
    else:
        raise Exception(f"Invalid command {args.command}")

