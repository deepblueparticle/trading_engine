project(cme_ilink)
add_executable(${PROJECT_NAME} utils.cpp sequence_serialization.cpp audit_trail.cpp fix_trader.cpp fix_engine.cpp)
target_link_libraries(${PROJECT_NAME} quickfix)
