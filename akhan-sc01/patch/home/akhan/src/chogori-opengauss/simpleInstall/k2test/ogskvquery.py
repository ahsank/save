diff --git a/simpleInstall/k2test/ogskvquery.py b/simpleInstall/k2test/ogskvquery.py
index 33f13f69..c039c08c 100644
--- a/simpleInstall/k2test/ogskvquery.py
+++ b/simpleInstall/k2test/ogskvquery.py
@@ -32,53 +32,117 @@ python3 ogskvquery.py query --schema 00000000000030008000000000000a30  # query b
 To specify remote sever use --http 'http://remote_server:port' option
 """
 
-def query(coll=None, schema=None, table_oid=None, table_name=None, database=None, txnarg = None):
-    if txnarg:
-        txn = Txn(cl, literal_eval(txnarg))
-    else:
-        status, txn = cl.begin_txn()
+def check_status(status):
+    if not status.is2xxOK():
+        raise Exception(status.message)
+
+class WrappedTxn:
+    def __init__(self, skvtxn = None):
+        self.txn = skvtxn
+        self.do_cleanup = False
+    
+    def __enter__(self):
+        if not self.txn:
+            status, self.txn = cl.begin_txn(TxnOptions(timeout=TimeDelta(seconds=60)))
+            check_status(status)
+            self.do_cleanup = True
+        # print(f"With txn {self.txn.timestamp}")
+        return self
+
+    def create_query(self, coll, schema):
+        status, query = self.txn.create_query(str.encode(coll), str.encode(schema))
         if not status.is2xxOK():
             raise Exception(status.message)
-    if database:
-        coll = get_db_coll(database)
-        
-    if database and not schema:
-        tables = get_tables(db_name=database)
-        if table_oid:
-            table = [t for t in tables if int(t.data['TableOid']) == int(table_oid)]
-        elif table_name:
-            table = [t for t in tables if t.data['TableName'].decode() == table_name]
-        if not table:
-            raise Exception(f"Table with oid {table_oid} not found")
-        schema = next(t for t in table).data['TableId'].decode()
+        return query
         
-    status, query = txn.create_query(str.encode(coll), str.encode(schema))
-    
-    if not status.is2xxOK():
-        raise Exception(status.message)
-    status, records = txn.queryAll(query)
-    if not status.is2xxOK():
-        raise Exception(status.message)
+    def queryAll(self, query):
+        status, records = self.txn.queryAll(query)
+        check_status(status)
+        return records
+
+    def write(self, collection, record, erase=False):
+        status = self.txn.write(str.encode(collection), record, erase)
+        check_status(status)
+     
+    def __exit__(self, exc_type, exc_value, traceback):
+        # print(f"Cleanup {self.txn} {self.do_cleanup}")
+        if self.do_cleanup and self.txn:
+            if exc_type:
+                # print(f"Aborting txn {self.txn}")
+                self.txn.end(False)
+            else:
+                # print(f"Commiting txn {self.txn}")
+                self.txn.end(True)
+                
+def query(coll, schema, txnarg = None):
+    with WrappedTxn(txnarg) as wtxn:
+        query = wtxn.create_query(coll, schema)
+        records = wtxn.queryAll(query)
     return records
 
-def get_schema(coll, schema_name,  table_oid=None, table_name=None, database=None):
-    if database:
-        coll = get_db_coll(database)
+def delete_table_data(coll, table, indices):
+    with WrappedTxn() as wtxn:
+        schema_name = table.data['TableId'].decode()
+        records = query(coll, schema_name, wtxn.txn)
+        for record in records:
+            print(f'deleting {record.data}')
+            wtxn.write(coll, record, True)
+        for idx in indices:
+            idx_recods = query(coll, idx.data['TableId'].decode(), wtxn.txn)
+            for idxrec in idx_recods:
+                print(f'deleting {idxrec.data}')
+                wtxn.write(coll, idxrec, True)
+
+def get_table(tables, table_oid=None, table_name=None):
+    table = None
+    if table_oid:
+        table = [t for t in tables if int(t.data['TableOid']) == int(table_oid)]
+    elif table_name:
+        table = [t for t in tables if t.data['TableName'].decode() == table_name]
+
+    if not table:
+        raise Exception(f"Table with oid {table_oid} not found")
+    return next(t for t in table)
 
+    
+def get_table_info(tables, table_oid=None, table_name=None):
+    table = get_table(tables, table_oid, table_name)
+    if table.data['IsIndex']:
+        base_table =  next(t for t in tables if t.data['TableId'] == table.data['BaseTableId'])
+        return False, table, base_table
+    else:
+        indices = [t for t in tables if t.data['BaseTableId'] == table.data['TableId']]
+        return True, table, indices
+
+def print_table(coll, table):
+    print(table.data)
+    coll, schema = get_schema(coll, table.data['TableId'].decode())
+    print_schema(schema)
+
+
+def get_schema_name(coll, schema_name,  table_oid=None, table_name=None, database=None):
+    if not coll and database:
+        coll = get_db_coll(database)
+    
     if database and not schema_name:
-        tables = get_tables(db_name=database)
-        if table_oid:
-            table = [t for t in tables if int(t.data['TableOid']) == int(table_oid)]
-        elif table_name:
-            table = [t for t in tables if t.data['TableName'].decode() == table_name]
-        if not table:
-            raise Exception(f"Table with oid {table_oid} not found")
-        schema_name = next(t for t in table).data['TableId'].decode()
+        tables, _ = get_tables(db_name=database)
+        table = get_table(tables, table_oid, table_name)
+        schema_name = table.data['TableId'].decode()
+
+    if not coll:
+        raise Exception("Cannot determine collection")
+    if not schema_name:
+        raise Exception("Cannot determine schema name")
+    return coll, schema_name
+
+def get_schema(coll, schema_name,  table_oid=None, table_name=None, database=None):
+    coll, schema_name = get_schema_name(coll, schema_name, table_oid, table_name, database)
     status, schema = cl.get_schema(str.encode(coll), str.encode(schema_name))
     if not status.is2xxOK():
         raise Exception(status.message)
-    return schema
+    return coll, schema
 
+    
 def print_schema(schema):
     for (k, v) in schema.__dict__.items():
         if k == 'fields':
@@ -107,12 +171,60 @@ def get_db_coll(db_name):
 def get_tables(coll=None, db_name=None):
     if not coll and db_name:
         coll = get_db_coll(db_name)
-    return query(coll=coll, schema="K2RESVD_SCHEMA_SQL_TABLE_META")
+    return query(coll=coll, schema="K2RESVD_SCHEMA_SQL_TABLE_META"), coll
+
+import ast
+
+def print_records(records, filter):
+    
+    filtered = records
+    if filter:
+        for k, v in filter.items():
+            first = filtered[0] if filtered else None
+            if not first:
+                break
+            isbin = isinstance(first.data[k], (bytes, bytearray))
+            if isbin:
+                filtered = [ele for ele in filtered if ele.data[k] == ast.literal_eval("b'" + v + "'")]
+            else:
+                filtered = [ele for ele in filtered if str(ele.data[k]) == v]
 
-def print_records(records):
-    for r in records:
+    for r in filtered:
         print(r.data)
 
+def get_txn(txnarg):
+    if not txnarg:
+        return None
+    times = txnarg.split(':')
+    if len(times) == 1:
+        return Txn(cl, [int(times[0]), 1000, 100000])
+    else:
+        d = int(times[0])
+        h = int(times[1])
+        min = int(times[2])
+        tmp = times[3].split('.')
+        sec = int(tmp[0])
+        msec = int(tmp[1])
+        usec = int(tmp[2])
+        print(f'{d}:{h}:{min}:{sec}.{msec}.{usec}')
+        val = (((((d*24+h)*60+min)*60)+sec)*1000 + msec)*1000+usec
+        return Txn(cl, [val, 1000, 10000])
+        
+class kvdictAppendAction(argparse.Action):
+    """
+    argparse action to split an argument into KEY=VALUE form
+    on the first = and append to a dictionary.
+    """
+    def __call__(self, parser, args, values, option_string=None):
+        assert(len(values) == 1)
+        try:
+            (k, v) = values[0].split("=", 2)
+        except ValueError as ex:
+            raise argparse.ArgumentError(self, f"could not parse argument \"{values[0]}\" as k=v format")
+        d = getattr(args, self.dest) or {}
+        d[k] = v
+        setattr(args, self.dest, d)    
+
 if __name__ == '__main__':
     parser = argparse.ArgumentParser()
     parser.add_argument("command", help="Command")
@@ -122,17 +234,24 @@ if __name__ == '__main__':
     parser.add_argument("--db", default="template1", help="Database name")
     parser.add_argument("--toid", help="Table oid")
     parser.add_argument("--table", help="Table name")
+    parser.add_argument("--filter",
+                      nargs=1,
+                      action=kvdictAppendAction,
+                      metavar="KEY=VALUE",
+                      help="Add key/value params. May appear multiple times.")
     parser.add_argument("--txn")
     
     args = parser.parse_args()
     cl = SKVClient(args.http)
     if args.command == "query":
-        records = query(coll=args.coll, schema=args.schema,
-                        txnarg=args.txn, table_oid=args.toid, table_name=args.table,
+        coll, schema_name = get_schema_name(coll=args.coll, schema_name=args.schema,
+                        table_oid=args.toid, table_name=args.table,
                         database=args.db)
-        print_records(records)
+        records = query(coll=coll, schema=schema_name,
+                        txnarg=get_txn(args.txn))
+        print_records(records, args.filter)
     elif args.command == "get-schema":
-        schema = get_schema(coll=args.coll, schema_name=args.schema,
+        coll, schema = get_schema(coll=args.coll, schema_name=args.schema,
                             table_oid=args.toid, table_name=args.table,
                             database=args.db)
         print_schema(schema)
@@ -143,8 +262,27 @@ if __name__ == '__main__':
         databases = get_databases()
         print_records(databases)
     elif args.command == "get-tables":
-        tables = get_tables(coll=args.coll, db_name=args.db)
+        tables, _ = get_tables(coll=args.coll, db_name=args.db)
         print_records(tables)
+    elif args.command == "get-table-info":
+        tables, coll = get_tables(coll=args.coll, db_name=args.db)
+        is_table, table, ref = get_table_info(tables=tables, table_oid=args.toid, table_name=args.table)
+        print('Table:' if is_table else 'Index')
+        print_table(coll, table)
+        if is_table:
+            print('Indices:')
+            for i in ref:
+                print('Index:')
+                print_table(coll, i)
+        else:
+            print('Base table:')
+            print_table(coll, ref)
+    elif args.command == "delete":
+        tables, coll = get_tables(coll=args.coll, db_name=args.db)
+        is_table, table, ref = get_table_info(tables=tables, table_oid=args.toid, table_name=args.table)
+        if not is_table:
+            raise Exception("Cannot delete data from index")
+        delete_table_data(coll, table, ref);
     elif args.command == "usage":
         print(helpmsg)
     else:
