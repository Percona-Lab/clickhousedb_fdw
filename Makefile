# contrib/clickhousedb_fdw/Makefile

MODULE_big = clickhousedb_fdw
OBJS = clickhousedb_fdw.o clickhousedb_option.o clickhousedb_deparse.o clickhousedb_connection.o clickhousedb_shipable.o
PGFILEDESC = "clickhousedb_fdw - foreign data wrapper for ClickHouse"

PG_CPPFLAGS = -Wno-unused-function -Ilib 
SHLIB_LINK_INTERNAL = -L. -lclickhouse-1.0 -lodbc

EXTENSION = clickhousedb_fdw
DATA = clickhousedb_fdw--1.0.sql

REGRESS = clickhousedb_fdw

ifdef USE_PGXS
PG_CONFIG = pg_config
PGXS := $(shell $(PG_CONFIG) --pgxs)
LIBDIR := $(shell $(PG_CONFIG) --libdir)
include $(PGXS)
else
SHLIB_PREREQS = libclickhouse
subdir = contrib/clickhousedb_fdw
top_builddir = ../..
include $(top_builddir)/src/Makefile.global
include $(top_srcdir)/contrib/contrib-global.mk
endif

all:	libclean libcompile libinstall

libclean:
	rm -rf libclickhouse-1.0.so lib/*.o 

libcompile:
	$(MAKE) -f lib/Makefile

libinstall:
	$(MAKE) -f lib/Makefile
	/usr/bin/install -c -m 755 libclickhouse-1.0.so $(LIBDIR)/libclickhouse-1.0.so
