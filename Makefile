DEUBG = -DXOP_DEBUG

TARGET = test
OBJS_PATH = objs

CXX   = g++
CC    = gcc
STRIP = 

INC  = -I$(shell pwd)/src -I$(shell pwd)/src/xop
LIB  = 
									   
LD_FLAGS  = -lrt -pthread -lpthread -ldl -lm $(DEBUG)
CXX_FLAGS = -std=c++11 

O_FLAG   = -O2 -g 

SRC1  = $(notdir $(wildcard ./example/*.cpp))	
OBJS1 = $(patsubst %.cpp,$(OBJS_PATH)/%.o,$(SRC1))	

SRC2  = $(notdir $(wildcard ./src/*.cpp))		
OBJS2 = $(patsubst %.cpp,$(OBJS_PATH)/%.o,$(SRC2))	

SRC3  = $(notdir $(wildcard ./src/xop/*.cpp))		
OBJS3 = $(patsubst %.cpp,$(OBJS_PATH)/%.o,$(SRC3))	

all: BUILD_DIR $(TARGET)

BUILD_DIR:
	@-mkdir -p $(OBJS_PATH)
	
$(TARGET) : $(OBJS1) $(OBJS2) $(OBJS3)
	$(CXX) $^ -o $@ $(CFLAGS) $(LD_FLAGS) 

$(OBJS_PATH)/%.o : ./example/%.cpp  
	$(CXX) -c  $< -o  $@ $(CXX_FLAGS) $(LD_FLAGS) $(INC)
$(OBJS_PATH)/%.o : ./src/%.cpp  
	$(CXX) -c  $< -o  $@ $(CXX_FLAGS) $(LD_FLAGS) 
$(OBJS_PATH)/%.o : ./src/xop/%.cpp  
	$(CXX) -c  $< -o  $@ $(CXX_FLAGS) $(LD_FLAGS) 	
	
clean: 
	-rm -rf $(OBJS_PATH) $(TARGET)














