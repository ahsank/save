REVOKE ALL on pg_authid FROM public;

DELETE FROM pg_depend;
DELETE FROM pg_shdepend;
INSERT INTO pg_depend SELECT 0,0,0, tableoid,oid,0, 'p' FROM pg_class;
INSERT INTO pg_depend SELECT 0,0,0, tableoid,oid,0, 'p' FROM pg_type;
INSERT INTO pg_depend SELECT 0,0,0, tableoid,oid,0, 'p' FROM pg_cast;
INSERT INTO pg_depend SELECT 0,0,0, tableoid,oid,0, 'p' FROM pg_constraint;
INSERT INTO pg_depend SELECT 0,0,0, tableoid,oid,0, 'p' FROM pg_attrdef;
INSERT INTO pg_depend SELECT 0,0,0, tableoid,oid,0, 'p' FROM pg_language;
INSERT INTO pg_depend SELECT 0,0,0, tableoid,oid,0, 'p' FROM pg_operator;
INSERT INTO pg_depend SELECT 0,0,0, tableoid,oid,0, 'p' FROM pg_opclass;
INSERT INTO pg_depend SELECT 0,0,0, tableoid,oid,0, 'p' FROM pg_opfamily;
INSERT INTO pg_depend SELECT 0,0,0, tableoid,oid,0, 'p' FROM pg_amop;
INSERT INTO pg_depend SELECT 0,0,0, tableoid,oid,0, 'p' FROM pg_amproc;
INSERT INTO pg_depend SELECT 0,0,0, tableoid,oid,0, 'p' FROM pg_rewrite;
INSERT INTO pg_depend SELECT 0,0,0, tableoid,oid,0, 'p' FROM pg_trigger;
INSERT INTO pg_depend SELECT 0,0,0, tableoid,oid,0, 'p' FROM pg_namespace WHERE nspname LIKE 'pg%' or nspname = 'cstore';
INSERT INTO pg_depend SELECT 0,0,0, tableoid,oid,0, 'p' FROM pg_ts_parser;
INSERT INTO pg_depend SELECT 0,0,0, tableoid,oid,0, 'p' FROM pg_ts_dict;
INSERT INTO pg_depend SELECT 0,0,0, tableoid,oid,0, 'p' FROM pg_ts_template;
INSERT INTO pg_depend SELECT 0,0,0, tableoid,oid,0, 'p' FROM pg_ts_config;
INSERT INTO pg_depend SELECT 0,0,0, tableoid,oid,0, 'p' FROM pg_collation;
INSERT INTO pg_shdepend SELECT 0,0,0,0, tableoid,oid, 'p' FROM pg_authid;
CREATE EXTENSION plpgsql;

997         ErrorData* edata = &t_thrd.log_cxt.errordata[t_thrd.log_cxt.errordata_stack_depth];                                                                |
(gdb) p *edata                                                                                                                                                 |
$5 = {elevel = 20, output_to_server = true, output_to_client = false, handle_in_client = false, show_funcname = false, hide_stmt = false, hide_prefix = false,\|
 filename = 0x41a011b "parse_func.cpp", lineno = 414, funcname = 0x41a0dc0 <ParseFuncOrColumn(ParseState*, List*, List*, FuncCall*, int, bool)::__func__> "Par\|
seFuncOrColumn", domain = 0x41a010f "plpgsql-9.2", sqlerrcode = 151027844, mod_id = MOD_MAX, message = 0x30ecc770 "function gs_session_memory_detail_tp() does\|
 not exist", detail = 0x0, detail_log = 0x0, hint = 0x30ecc7c0 "No function matches the given name and argument types. You might need to add explicit type casts.", context = 0x0, cursorpos = 17583, internalpos = 0, internalquery = 0x0, saved_errno = 2, backtrace_log = 0x0, internalerrcode = 0, verbose = false, igno\|
re_interrupt = false, cause = 0x0, action = 0x0}                                                                                                              
(g
b parse_func.cpp:408
1247 -> pg_type
2703 -> idx pg_type
2608 -> pg_depend
2674 -> idx pg_depend
1249 -> pg_attribute

* 1259 -> pg_class
* 2662 -> index pg class
2663 -> index pg_class unique
2610 -> pg_index -> idx 2678
#define IndexRelationId  2610
Anum_pg_index_indrelid = 2
Relation rd_id
PgGate_DmlFetch

b RelationGetIndexList if relation.rd_id == 1259
b relcache.cpp:5601 if relation.rd_id == 1259
b PgGate_ExecInsert if table_oid == 2678

also 5603

CatalogIndexInsert
CatalogUpdateIndexes
systable_beginscan
PgGate_ExecSelect
PgGate_DmlFetch

PgGate_ExecInsert


pg_index

create view testvew29 as select 1 as out;

CREATE DATABASE template0;

UPDATE pg_database SET datistemplate = 't', datallowconn = 'f'WHERE datname = 'template0';
        
"UPDATE pg_database SET datlastsysoid = 
(gdb) b PgGate_ExecUpdate if table_oid==1259
(gdb) b PgGate_ExecUpdate if table_oid==2662
b PgGate_ExecInsert if table_oid==2662

CREATE VIEW dbe_perf.session_memory_detail AS SELECT * FROM gs_session_memory_detail_tp();

CREATE OR REPLACE FUNCTION to_number(TEXT) RETURNS NUMERIC AS $$ SELECT numeric_in(textout($1), 0::Oid, -1) $$ LANGUAGE SQL  STRICT IMMUTABLE NOT FENCED;

CREATE TABLE client (c_id INT PRIMARY KEY, c_name VARCHAR(100) NOT NULL, c_mail CHAR(30) UNIQUE);

INSERT INTO client(c_id,c_name,c_mail) VALUES (1,'张一','zhangyi@huawei.com');

CREATE OR REPLACE FUNCTION to_number(TEXT) RETURNS NUMERIC AS $$ SELECT numeric_in(textout($1), 0::Oid, -1) $$ LANGUAGE SQL  STRICT IMMUTABLE NOT FENCED;

backend> statement: INSERT INTO pg_collation (collname, collnamespace, collowner, collencoding, collcollate, collctype)  SELECT DISTINCT ON (collname, encoding)   collname,    (SELECT oid FROM pg_namespace WHERE nspname = 'pg_catalog') AS collnamespace,    (SELECT relowner FROM pg_class WHERE relname = 'pg_collation') AS collowner,    encoding, locale, locale   FROM tmp_pg_collation  WHERE NOT EXISTS (SELECT 1 FROM pg_collation WHERE collname = tmp_pg_collation.collname)  ORDER BY collname, encoding, (collname = locale) DESC, locale;                                                                                                                                                                                                                                                                                                                                                                                                                                  Error status: transaction does not exist with stacktrace: [#1 0x1eb3367 HandleK2PgStatus(k2pg::Status const&)], [#2 0x1ed5ecb /opt/opengauss/bin/gaussdb() [0x1ed5ecb]], [#3 0x1ed8db9 cam_getnext_heaptuple(CamScanDescData*, bool, bool*)], [#4 0x1ed9114 cam_systable_getnext(SysScanDescData*)], [#5 0x20b6e1d systable_getnext(SysScanDescData*)], [#6 0x12dbd75 /opt/opengauss/bin/gaussdb() [0x12dbd75]], [#7 0x12dabfb /opt/opengauss/bin/gaussdb() [0x12dabfb]], [#8 0x12da839 SearchCatCache(CatCache*, unsigned long, unsigned long, unsigned long, unsigned long, int)], [#9 0x1308cf1 SearchSysCache(int, unsigned long, unsigned long, unsigned long, unsigned long, int)], [#10 0x1308ef9 SearchSysCacheExists(int, unsigned long, unsigned long, unsigned long, unsigned long)], [#11 0x130b2dc op_in_opfamily(unsigned int, unsigned int)], [#12 0x160ebd2 /opt/opengauss/bin/gaussdb() [0x160ebd2]], [#13 0x160ea5e /opt/opengauss/bin/gaussdb() [0x160ea5e]], [#14 0x160e6e1 /opt/opengauss/bin/gaussdb() [0x160e6e1]], [#15 0x160e49c /opt/opengauss/bin/gaussdb() [0x160e49c]], [#16 0x160a155 create_index_paths(PlannerInfo*, RelOptInfo*)], [#17 0x15ea5dc /opt/opengauss/bin/gaussdb() [0x15ea5dc]], [#18 0x15e9966 /opt/opengauss/bin/gaussdb() [0x15e9966]], [#19 0x15e9a40 /opt/opengauss/bin/gaussdb() [0x15e9a40]], [#20 0x15e939d /opt/opengauss/bin/gaussdb() [0x15e939d]], [#21 0x15e8e89 make_one_rel(PlannerInfo*, List*)], [#22 0x164ba0d query_planner(PlannerInfo*, List*, void (*)(PlannerInfo*, void*), void*)], [#23 0x165447c /opt/opengauss/bin/gaussdb() [0x165447c]], [#24 0x165187a subquery_planner(PlannerGlobal*, Query*, PlannerInfo*, bool, double, PlannerInfo**, int, ItstDisKey*, List*)]                                                                                                                                                                                                                                           2022-11-30 22:43:19.035 [unknown] [unknown] localhost 139847048281408 0[0:0#0]  [BACKEND] ERROR:  Status: transaction does not exist:                                                                                                                                                                                          2022-11-30 22:43:19.035 [unknown] [unknown] localhost 139847048281408 0[0:0#0]  [BACKEND] STATEMENT:  INSERT INTO pg_collation (collname, collnamespace, collowner, collencoding, collcollate, collctype)  SELECT DISTINCT ON (collname, encoding)   collname,    (SELECT oid FROM pg_namespace WHERE nspname = 'pg_catalog') AS collnamespace,    (SELECT relowner FROM pg_class WHERE relname = 'pg_collation') AS collowner,    encoding, locale, locale   FROM tmp_pg_collation  WHERE NOT EXISTS (SELECT 1 FROM pg_collation WHERE collname = tmp_pg_collation.collname)  ORDER BY collname, encoding, (collname = locale) DESC, locale;

CREATE DOMAIN cardinal_number AS integer CONSTRAINT cardinal_number_domain_check CHECK (value >= 0);
CREATE DOMAIN character_data AS character varying;

UPDATE sql_sizing SET supported_value = (SELECT typlen-1 FROM pg_catalog.pg_type WHERE typname = 'name'), comments = 'Might be less, depending on character set.' WHERE supported_value = 63;

break pg_session.cpp:55 if object_oid == 14722
break PgSession::LoadTable if object_oid == 14722
b  PgGate_NewSelect if table_oid == 14722

SET search_path TO information_schema; select supported_value from sql_sizing;

p *((SeqScanState*)portal->queryDesc->planstate)->ps.plan                                                                                                |
UPDATE information_schema.sql_implementation_info SET character_value = '%s' WHERE implementation_info_name = 'DBMS VERSION';

set search_path to public;

INSERT INTO oorder (o_id, o_d_id, o_w_id, o_c_id, o_entry_d, o_ol_cnt, o_all_local) VALUES (5 , 2, 2, 2, '1999-01-08 04:05:06', 1, 1);

INSERT INTO oorder (o_id, o_d_id, o_w_id, o_c_id, o_ol_cnt, o_all_local) VALUES (5 , 1, 1, 1, 1, 1);

INSERT INTO oorder (o_id, o_d_id, o_w_id, o_c_id, o_entry_d, o_ol_cnt, o_all_local) VALUES (5 , 1, 1, 1, '1999-01-08 04:05:06', 1, 1);

CREATE TABLE test1 (o_w_id int NOT NULL, o_d_id int NOT NULL, o_ol_cnt decimal(2,0) NOT NULL, o_all_local decimal(1,0) NOT NULL, o_entry_d timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP, PRIMARY KEY (o_w_id,o_d_id));

CREATE TABLE test2 (o_w_id int NOT NULL, o_c_id int NOT NULL, o_id int NOT NULL, o_ol_cnt decimal(2,0) NOT NULL, o_all_local decimal(1,0) NOT NULL, o_entry_d timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP, PRIMARY KEY (o_w_id,o_id), UNIQUE (o_w_id,o_c_id, o_id));

INSERT INTO test2 (o_c_id, o_id, o_w_id,  o_entry_d, o_ol_cnt, o_all_local) VALUES (5 , 1,  1,  '1999-01-08 04:05:06', 1, 1);

create table client1 (c_id INT NOT NULL PRIMARY KEY, c_name VARCHAR(100) NOT NULL, c_phone CHAR(20) UNIQUE NOT NULL);

create index ix_name on client1 (c_name);

INSERT INTO client1(c_id,c_name,c_phone) VALUES (1,'zhangyi','18815650001');
INSERT INTO client1(c_id,c_name,c_phone)  VALUES (2, 'zhanger','18815650002');
INSERT INTO client1(c_id,c_name,c_phone)  VALUES (3, 'zhangsan','18815650003');
INSERT INTO client1(c_id,c_name,c_phone)  VALUES (4, 'zhangsi','18815650004');
INSERT INTO client1(c_id,c_name,c_phone)  VALUES (5, 'zhangwu','18815650005');

create table client2 (c_id INT NOT NULL PRIMARY KEY, c_name VARCHAR(100) NOT NULL, c_phone CHAR(20) NOT NULL);

INSERT INTO client2(c_id,c_name,c_phone) VALUES (1,'zhangyi','18815650001');
INSERT INTO client2(c_id,c_name,c_phone)  VALUES (2, 'zhanger','18815650002');

INSERT INTO client3(c_id,c_name,c_phone) VALUES (1,'zhangyi9','18015750001');

select c_name from client2 where c_id=1;

create table client4 (c_id INT NOT NULL, c_name VARCHAR(100) NOT NULL, c_phone CHAR(20) NOT NULL);

create index ix_name on client4 (c_name);

INSERT INTO client4(c_id,c_name,c_phone) VALUES (1,'zhangyi','18815650001');
INSERT INTO client4(c_id,c_name,c_phone)  VALUES (2, 'zhanger','18815650002');

select c_phone from client4 where c_name = 'zhanger';

b PgGate_NewSelect if table_oid == 20760
explain analyze select c_phone from client4 where c_name = 'zhanger';
explain (verbose) select c_phone from client4 where c_name = 'zhanger';

b set_plain_rel_pathlist
b create_index_paths
b get_ndex_paths
b build_index_paths
b match_clause_to_index
b match_clause_to_indexcol
op_in_opfamily
b indexpath.cpp:2378

create table client7 (c_id INT NOT NULL PRIMARY KEY, c_name VARCHAR(100) NOT NULL, c_phone CHAR(20) UNIQUE NOT NULL);

INSERT INTO client7(c_id,c_name,c_phone) VALUES (1,'zhangyi','18815650001');
INSERT INTO client7(c_id,c_name,c_phone)  VALUES (2, 'zhanger','18815650002');

select * from client7 where c_id=1;
