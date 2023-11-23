idf_component_register(SRCS "mp3_player.c"
                        INCLUDE_DIRS "."
                        REQUIRES audio_pipeline audio_stream esp_peripherals)