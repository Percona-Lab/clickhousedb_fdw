## What is clickhousedb_fdw?
The clickhousedb_fdw is open-source. It is a Foreign Data Wrapper (FDW) for one of the fastest column store databases; "Clickhouse". This FDW allows you to SELECT from, and INSERT into, a ClickHouse database from within a PostgreSQL server. The FDW supports advanced features like aggregate pushdown and joins pushdown. These significantly improve performance by utilizing the remote server’s resources for these resource intensive operations.


## Documentation
1. [Supported PostgreSQL Versions](#supported-postgresql-versions)
2. [Installation](#installation)
3. [Setup](#setup) 
4. [User Guide](https://github.com/Percona-Lab/clickhousedb_fdw/blob/master/docs/USER_GUIDE.md)
6. [Release Notes](https://github.com/Percona-Lab/clickhousedb_fdw/blob/master/docs/RELEASE_NOTES.md)
7. [License](https://github.com/Percona-Lab/clickhousedb_fdw/blob/master/LICENSE)
8. [Submitting Bug Reports](#submitting-bug-reports)
9. [Copyright Notice](#copyright-notice)

## Supported PostgreSQL Versions
The ``clickhousedb_fdw`` should work on the latest version of PostgreSQL but is only tested with these PostgreSQL versions:

| Distribution            |  Version       | Supported          |
| ------------------------|----------------|--------------------|
| PostgreSQL              | Version < 11   | :x:                |
| PostgreSQL              | Version 11     | :heavy_check_mark: |
| PostgreSQL              | Version 12     | :heavy_check_mark: |
| PostgreSQL              | Version 13     | :heavy_check_mark: |
| Percona Distribution    | Version < 11   | :x:                |
| Percona Distribution    | Version 11     | :heavy_check_mark: |
| Percona Distribution    | Version 12     | :heavy_check_mark: |
| Percona Distribution    | Version 13     | :heavy_check_mark: |

## Installation

### Installing from source code

You can download the source code of the latest release of ``clickhousedb_fdw``  from [this GitHub page](https://github.com/Percona-Lab/clickhousedb_fdw/releases) or using git:
```sh
git clone git://github.com/Percona-Lab/clickhousedb_fdw.git
```

Compile and install the extension
```sh
cd clickhousedb_fdw
make USE_PGXS=1
make USE_PGXS=1 install
```

Create the extension using the ``CREATE EXTENSION`` command.
```sql
CREATE EXTENSION clickhousedb_fdw;
CREATE EXTENSION
```

###### Install using deb / rpm packages.
```bash
sudo yum install clickhousedb_fdw
sudo apt-get install clickhousedb_fdw
```

```sql
CREATE SERVER clickhouse_svr
FOREIGN DATA WRAPPER clickhousedb_fdw 
OPTIONS(dbname 'test_database', driver '/home/vagrant/percona/clickhousedb_fdw/lib/clickhouse-odbc/driver/libclickhouseodbc.so', host '127.0.0.1');
   
CREATE USER MAPPING FOR CURRENT_USER SERVER clickhouse_svr;

CREATE FOREIGN TABLE tax_bills_nyc 
    (
        bbl int8,
        owner_name text,
        address text,
        tax_class text,
        tax_rate text,
        emv Float,
        tbea Float,
        bav Float,
        tba text,
        property_tax text,
        condonumber text,
        condo text,
        insertion_date Time 
    ) SERVER clickhouse_svr;

SELECT bbl,tbea,bav,insertion_date FROM tax_bills_nyc LIMIT 5;
        bbl     | tbea  |  bav   | insertion_date 
    ------------+-------+--------+----------------
    4001940057 | 18755 | 145899 | 15:25:42
    1016830130 |  2216 |  17238 | 15:25:42
    4012850059 | 69562 | 541125 | 15:25:42
    1006130061 | 55883 | 434719 | 15:25:42
    3033540009 | 33100 | 257490 | 15:25:42
    (5 rows)
    
CREATE TABLE tax_bills ( bbl bigint, owner_name text) ENGINE = MergeTree PARTITION BY bbl ORDER BY (bbl)
    
INSERT INTO tax_bills SELECT bbl, tbea from tax_bills_nyc LIMIT 100;
    
EXPLAIN VERBOSE SELECT bbl,tbea,bav,insertion_date FROM tax_bills_nyc LIMIT 5;
                                         QUERY PLAN                                         
    --------------------------------------------------------------------------------------------
    Limit  (cost=0.00..0.00 rows=1 width=32)
    Output: bbl, tbea, bav, insertion_date
     ->  Foreign Scan on public.tax_bills_nyc  (cost=0.00..0.00 rows=0 width=32)
             Output: bbl, tbea, bav, insertion_date
             Remote SQL: SELECT bbl, tbea, bav, insertion_date FROM test_database.tax_bills_nyc
    (5 rows)
 ```


To learn more about ``Percona-Lab/clickhousedb_fdw`` configuration and usage, see [User Guide](https://github.com/Percona-Lab/clickhousedb_fdw/blob/master/docs/USER_GUIDE.md).

## Submitting Bug Reports

If you found a bug in ``clickhousedb_fdw``, please submit the report to the [Jira issue tracker](https://jira.percona.com/projects/PG/issues)

Start by searching the open tickets for a similar report. If you find that someone else has already reported your issue, then you can upvote that report to increase its visibility.

If there is no existing report, submit your report following these steps:

Sign in to [Jira issue tracker](https://jira.percona.com/projects/PG/issues). You will need to create an account if you do not have one.

In the *Summary*, *Description*, *Steps To Reproduce*, *Affects Version* fields describe the problem you have detected. 

As a general rule of thumb, try to create bug reports that are:

- Reproducible: describe the steps to reproduce the problem.

- Specific: include the version of Percona Backup for MongoDB, your environment, and so on.

- Unique: check if there already exists a JIRA ticket to describe the problem.

- Scoped to a Single Bug: only report one bug in one JIRA ticket.


## Copyright Notice
Copyright (c) 2006 - 2020, Percona LLC.
