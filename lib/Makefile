.PHONY : clean

CC=g++
CPPFLAGS:= -O0 -g3 -Wno-unused-variable -fPIC -Wall -I. -Ilib 
LDFLAGS:= $(LDFLAGS) -fPIC -shared -ldl -lstdc++ -L -lodbc 

TEST_CPPFLAGS:= -O0 -g3 -Wno-unused-variable -Iinclude 
TEST_LDFLAGS:= -ldl -L. -lclickhouse-1.0 -lodbc 
PG_CONFIG=pg_config
INSTALL_DIR=$(shell $(PG_CONFIG) --libdir)

SOURCES = lib/clickhouse-client.cpp
HEADERS = 
OBJECTS = $(SOURCES:.cpp=.o)

TEST_SOURCES = lib/example/odbc_test.cpp
TEST_OBJECTS = $(TEST_SOURCES:.cpp=.o)

TARGET=libclickhouse-1.0.so
TEST_TARGET=test

all: $(TARGET) $(TEST_TARGET)

clean:
	rm -rf $(OBJECTS) $(TARGET) $(TEST_OBJECTS) $(TEST_TARGET) target

install:
	cp -rf $(TARGET) $(INSTALL_DIR)

test: $(TEST_TARGET)

$(TEST_TARGET): $(TARGET) $(TEST_OBJECTS)
	$(CC) $(TEST_CPPFLAGS) $(TEST_OBJECTS) -o $@ $(TEST_LDFLAGS) 
	
$(TARGET) : $(OBJECTS)
	$(CC) $(CFLAGS) $(OBJECTS) -o $@ $(LDFLAGS)

