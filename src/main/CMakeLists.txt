idf_component_register(
    SRCS
        "main.c"
        "mpu6050.c"
        "audio.c"
        "posture_fsm.c"
        "voice/sit_up_wav.c"
    INCLUDE_DIRS "."
)

target_link_libraries(${COMPONENT_LIB} PUBLIC m)
