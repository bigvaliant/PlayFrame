MESSAGE(STATUS "CMAKE_CURRENT_SOURCE_DIR" ${CMAKE_CURRENT_SOURCE_DIR})

#执行命令 加上WORKING_DIRECTORY
#DB
EXECUTE_PROCESS(COMMAND ${PROTOC} -I=${PROTOCOL_DIR}/proto/database
    --cpp_out=${PROTOCOL_DIR}/cpp ${PROTOCOL_DIR}/proto/database/database.proto)

#CS
EXECUTE_PROCESS(COMMAND ${PROTOC} -I=${PROTOCOL_DIR}/proto/cs_proto 
    --cpp_out=${PROTOCOL_DIR}/cpp ${PROTOCOL_DIR}/proto/cs_proto/cs_msg.proto)
EXECUTE_PROCESS(COMMAND ${PROTOC} -I=${PROTOCOL_DIR}/proto/cs_proto 
    --cpp_out=${PROTOCOL_DIR}/cpp ${PROTOCOL_DIR}/proto/cs_proto/cs_role.proto)
EXECUTE_PROCESS(COMMAND ${PROTOC} -I=${PROTOCOL_DIR}/proto/cs_proto 
    --cpp_out=${PROTOCOL_DIR}/cpp ${PROTOCOL_DIR}/proto/cs_proto/cs_reg.proto)
EXECUTE_PROCESS(COMMAND ${PROTOC} -I=${PROTOCOL_DIR}/proto/cs_proto 
    --cpp_out=${PROTOCOL_DIR}/cpp ${PROTOCOL_DIR}/proto/cs_proto/cs_notify.proto)

#SS
EXECUTE_PROCESS(COMMAND ${PROTOC} -I=${PROTOCOL_DIR}/proto/ss_proto 
    --cpp_out=${PROTOCOL_DIR}/cpp ${PROTOCOL_DIR}/proto/ss_proto/ss_msg.proto)
EXECUTE_PROCESS(COMMAND ${PROTOC} -I=${PROTOCOL_DIR}/proto/ss_proto 
    --cpp_out=${PROTOCOL_DIR}/cpp ${PROTOCOL_DIR}/proto/ss_proto/ss_data.proto)

