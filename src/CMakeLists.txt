PROJECT(game)
CMAKE_MINIMUM_REQUIRED(VERSION 2.8)

#编译参数
SET(CMAKE_C_FLAGS "-Wall -g -ggdb -Wextra -pipe -Wno-invalid-offsetof -Wunused -D__X86_64")
SET(CMAKE_CXX_FLAGS "-Wall -g -ggdb -Wextra -pipe -Wno-invalid-offsetof -Wunused -D__X86_64")

#预定义
ADD_DEFINITIONS(-D_GNU_SOURCE -D_REENTRANT)

#设置变量
SET(COMMON_DIR ${PROJECT_SOURCE_DIR}/common)
SET(FRAME_DIR ${PROJECT_SOURCE_DIR}/common/frame)
SET(DEP_DIR ${PROJECT_SOURCE_DIR}/common/dep)
SET(PROTOCOL_DIR ${PROJECT_SOURCE_DIR}/common/protocol)

#设置依赖库变量
SET(FRAME_INCLUDE ${FRAME_DIR})
SET(FRAME_LIB ${FRAME_DIR}/libgameframe.a)
SET(PROTOCOL_INCLUDE ${PROTOCOL_DIR}/cpp)
SET(PROTOCOL_LIB ${PROTOCOL_DIR}/cpp/libgameprotocol.a)
SET(GLOG_INCLUDE ${DEP_DIR}/glog/include)
SET(GLOG_LIB ${DEP_DIR}/glog/lib/libglog.a)
SET(PROTOBUF_INCLUDE ${DEP_DIR}/protobuf/include)
SET(PROTOBUF_LIB ${DEP_DIR}/protobuf/lib/libprotobuf.a)
SET(ZMQ_INCLUDE ${DEP_DIR}/zmq/include)
SET(ZMQ_LIB ${DEP_DIR}/zmq/lib/libzmq.a)
SET(LUA_INCLUDE ${DEP_DIR}/lua/include)
SET(LUA_LIB ${DEP_DIR}/lua/lib/liblua.a)
SET(MYSQL_INCLUDE ${DEP_DIR}/mysql/include)
SET(MYSQL_LIB ${DEP_DIR}/mysql/lib/libmysqlclient.a)

#设置工具变量
SET(CPPCHECK ${DEP_DIR}/cppcheck/bin/cppcheck)
SET(CPPCHECK_FLAGS "--enable=all" " --template=gcc")

SET(PROTOC ${DEP_DIR}/protobuf/bin/protoc)

#设置脚本路径
SET(GAME_LUA_SCRIPT_SRC ${PROJECT_SOURCE_DIR}/gamesvr/script)
SET(GAME_LUA_SCRIPT_RUN ${PROJECT_SOURCE_DIR}/../run/script/gamesvr)

#链接库
SET(DYN_LIBS dl m c z rt pthread)
SET(STATIC_LIBS
    ${FRAME_LIB}
    ${PROTOCOL_LIB}
    ${GLOG_LIB}
    ${PROTOBUF_LIB}
    ${ZMQ_LIB}
    ${LUA_LIB}
    ${MYSQL_LIB}
)

#头文件 -I路径设置
INCLUDE_DIRECTORIES(
    ${FRAME_INCLUDE}
    ${PROTOCOL_INCLUDE}
    ${GLOG_INCLUDE}
    ${PROTOBUF_INCLUDE}
    ${ZMQ_INCLUDE}
    ${LUA_INCLUDE}
    ${MYSQL_INCLUDE}
)

#静态库 -L路径设置
#LINK_DIRECTORIES(
    #${COMMON_LIB_DIR}
#)

#CPPCHECK
EXECUTE_PROCESS(COMMAND ${CPPCHECK} ${PROJECT_SOURCE_DIR}/common/frame ${CPPCHECK_FLAGS})
EXECUTE_PROCESS(COMMAND ${CPPCHECK} ${PROJECT_SOURCE_DIR}/connsvr ${CPPCHECK_FLAGS})
EXECUTE_PROCESS(COMMAND ${CPPCHECK} ${PROJECT_SOURCE_DIR}/gamesvr ${CPPCHECK_FLAGS})

#子目录
ADD_SUBDIRECTORY(common)
ADD_SUBDIRECTORY(tcpclient)
ADD_SUBDIRECTORY(connsvr)
ADD_SUBDIRECTORY(datasvr)
ADD_SUBDIRECTORY(gamesvr)

