file(GLOB CARD_I4_FILES "${CMAKE_CURRENT_LIST_DIR}/assets/cards/*.i4")
file(GLOB CARD_IMA_FILES "${CMAKE_CURRENT_LIST_DIR}/assets/cards/*.ima")
list(SORT CARD_I4_FILES)
list(SORT CARD_IMA_FILES)

idf_component_register(
    SRCS "cards_main.c"
         "cards_app.c"
         "cards_model.c"
         "cards_ui.c"
         "cards_audio.c"
         "cards_player.c"
         "cards_adpcm.c"
         "cards_catalog.c"
         "font_cards_88.c"
         "font_cards_22.c"
    INCLUDE_DIRS "."
    REQUIRES bsp
    EMBED_FILES ${CARD_I4_FILES} ${CARD_IMA_FILES}
)
