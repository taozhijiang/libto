DEBUG ?= 1
CXX = g++
CXXFLAGS = -g -O0 -std=c++11
PACKAGE = libto_test
PACKAGE_NAME = $(PACKAGE)
PACKAGE_STRING = $(PACKAGE_NAME)1.0
PACKAGE_VERSION = 1.0
SHELL = /bin/bash
VERSION = 1.0
SRC_DIRS = ../source
TEST_SRC_DIRS = ./
EXTRAFLAGS =  -I../include/ 
EXTRAFLAGS += -lboost_system -lboost_context -lboost_thread -lboost_date_time -lboost_log -lboost_log_setup 
EXTRAFLAGS += -Wall -Wextra -march=native
TESTFLAGS = -lboost_unit_test_framework

OBJDIR = ../obj
TEST_OBJDIR = ./obj

vpath %.cpp $(SRC_DIRS)
vpath %.cpp $(TEST_SRC_DIRS)

srcs = $(filter-out main.cpp, $(notdir $(wildcard $(SRC_DIRS)/*.cpp)))
objs = $(srcs:%.cpp=$(OBJDIR)/%.o)

test_srcs = $(filter-out test_main.cpp, $(notdir $(wildcard $(TEST_SRC_DIRS)/*.cpp)))
test_objs = $(test_srcs:%.cpp=$(TEST_OBJDIR)/%.o)

all : $(PACKAGE)
.PHONY : all

ifeq ($(DEBUG),1)
TARGET_DIR=Debug
else
TARGET_DIR=Release
endif

$(PACKAGE) : $(objs) $(test_objs)
	@test -d $(OBJDIR) || mkdir $(OBJDIR)
	$(CXX) -c $(CXXFLAGS) $(EXTRAFLAGS) $(TEST_SRC_DIRS)/test_main.cpp -o $(TEST_OBJDIR)/test_main.o
	$(CXX) $(TEST_OBJDIR)/test_main.o $^ $(CXXFLAGS) $(EXTRAFLAGS) $(TESTFLAGS) -o $(PACKAGE)

$(objs) : $(OBJDIR)/%.o: %.cpp
	@test -d $(OBJDIR) || mkdir $(OBJDIR)
	$(CXX) -MMD -c $(CXXFLAGS) $< -o $@  $(EXTRAFLAGS) 

$(test_objs): $(TEST_OBJDIR)/%.o: %.cpp
	- @mkdir -p $(TEST_OBJDIR)
	$(CXX) -MMD -c $(CXXFLAGS) $(EXTRAFLAGS) $< -o $@ 

#check header for obj reconstruction
-include $(OBJDIR)/*.d
-include $(TEST_OBJDIR)/*.d

.PHONY : clean 
clean :	
	-rm -fr $(OBJDIR)
	-rm -fr $(PACKAGE)
	-rm -fr $(TEST_OBJDIR)/*
