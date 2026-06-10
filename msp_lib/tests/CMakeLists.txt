add_executable(test_codec test_codec.cpp)
target_link_libraries(test_codec PRIVATE msp_lib)
add_test(NAME codec COMMAND test_codec)

add_executable(test_crc test_crc.cpp)
target_link_libraries(test_crc PRIVATE msp_lib)
add_test(NAME crc COMMAND test_crc)
