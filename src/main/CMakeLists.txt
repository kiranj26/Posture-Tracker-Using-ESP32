idf_component_register(
    SRCS
        "main.c"
        "mpu6050.c"
        "audio.c"
        "posture_fsm.c"
    INCLUDE_DIRS "."
    EMBED_FILES
        "voice/sit_up.wav"
)

target_link_libraries(${COMPONENT_LIB} PUBLIC m)
