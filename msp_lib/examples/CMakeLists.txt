foreach(EXAMPLE read_status pid_tuner motor_test imu_stream)
    add_executable(${EXAMPLE} ${EXAMPLE}.cpp)
    target_link_libraries(${EXAMPLE} PRIVATE msp_lib)
endforeach()
