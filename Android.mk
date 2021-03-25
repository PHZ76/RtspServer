LOCAL_PATH := $(call my-dir)
DEUBG = -D_DEBUG

SRC1_FILES := $(wildcard $(LOCAL_PATH)/src/net/*.cpp)
SRC2_FILES := $(wildcard $(LOCAL_PATH)/src/xop/*.cpp)
SRC3_FILES := $(wildcard $(LOCAL_PATH)/example/rtsp_server.cpp)
SRC4_FILES := $(wildcard $(LOCAL_PATH)/example/rtsp_pusher.cpp)
SRC5_FILES := $(wildcard $(LOCAL_PATH)/example/rtsp_h264_file.cpp)

##### Module rtsp_server########

include $(CLEAR_VARS)
LOCAL_MODULE := rtsp_server

LOCAL_C_INCLUDES += \
	$(LOCAL_PATH)/inc \
	$(LOCAL_PATH)/src \
	$(LOCAL_PATH)/src/xop \
	$(LOCAL_PATH)/src/3rdpart 

LOCAL_SRC_FILES :=  \
        $(SRC1_FILES:$(LOCAL_PATH)/%=%) \
		$(SRC2_FILES:$(LOCAL_PATH)/%=%) \
		$(SRC3_FILES:$(LOCAL_PATH)/%=%) 

LOCAL_CPPFLAGS  += -fPIC -Wall -Wno-unused-parameter -lrt -pthread -lpthread -ldl -lm $(DEBUG) -std=c++11

include $(BUILD_EXECUTABLE)

##### Module rtsp_pusher########
include $(CLEAR_VARS)
LOCAL_MODULE := rtsp_pusher

LOCAL_C_INCLUDES += \
	$(LOCAL_PATH)/inc \
	$(LOCAL_PATH)/src \
	$(LOCAL_PATH)/src/xop \
	$(LOCAL_PATH)/src/3rdpart 

LOCAL_SRC_FILES :=  \
        $(SRC1_FILES:$(LOCAL_PATH)/%=%) \
		$(SRC2_FILES:$(LOCAL_PATH)/%=%) \
		$(SRC4_FILES:$(LOCAL_PATH)/%=%) 

LOCAL_CPPFLAGS  += -fPIC -Wall -Wno-unused-parameter -lrt -pthread -lpthread -ldl -lm $(DEBUG) -std=c++11

include $(BUILD_EXECUTABLE)

##### Module rtsp_h264_file########
include $(CLEAR_VARS)
LOCAL_MODULE := rtsp_h264_file

LOCAL_C_INCLUDES += \
	$(LOCAL_PATH)/inc \
	$(LOCAL_PATH)/src \
	$(LOCAL_PATH)/src/xop \
	$(LOCAL_PATH)/src/3rdpart 

LOCAL_SRC_FILES :=  \
        $(SRC1_FILES:$(LOCAL_PATH)/%=%) \
		$(SRC2_FILES:$(LOCAL_PATH)/%=%) \
		$(SRC5_FILES:$(LOCAL_PATH)/%=%) 

LOCAL_CPPFLAGS  += -fPIC -Wall -Wno-unused-parameter -lrt -pthread -lpthread -ldl -lm $(DEBUG) -std=c++11

include $(BUILD_EXECUTABLE)