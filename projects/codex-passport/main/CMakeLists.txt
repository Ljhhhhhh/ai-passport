idf_component_register(
    SRCS "main.c"
         "passport_protocol.c"
         "passport_storage.c"
         "passport_ble.c"
         "passport_ui.c"
         "passport_idle.c"
         "passport_alert.c"
         "passport_adpcm.c"
         "passport_clips.c"
         "font_passport_16.c"
    INCLUDE_DIRS "."
    REQUIRES bsp nvs_flash bt
    EMBED_FILES "assets/voice/voice_done.ima"
                "assets/voice/voice_wait.ima"
                "assets/voice/voice_error.ima"
                "assets/voice/voice_new_msg.ima"
)
