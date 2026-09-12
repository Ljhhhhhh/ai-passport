#include "passport_clips.h"

#if defined(ESP_PLATFORM) && !defined(PASSPORT_AUDIO_HOST_TEST)

#define DECLARE_CLIP(sym)     extern const uint8_t s_##sym##_start[] asm("_binary_" #sym "_ima_start");     extern const uint8_t s_##sym##_end[] asm("_binary_" #sym "_ima_end");

#define CLIP_ENTRY(id, sym)     case (id):         blob->data = s_##sym##_start;         blob->size = (size_t)(s_##sym##_end - s_##sym##_start);         return blob->size > 0U;

DECLARE_CLIP(voice_done)
DECLARE_CLIP(voice_wait)
DECLARE_CLIP(voice_error)
DECLARE_CLIP(voice_new_msg)

int passport_clip_blob(passport_clip_id_t id, passport_clip_blob_t *blob)
{
    if (!blob) return 0;
    blob->data = NULL;
    blob->size = 0;
    switch (id) {
    CLIP_ENTRY(PASSPORT_CLIP_DONE, voice_done)
    CLIP_ENTRY(PASSPORT_CLIP_WAIT, voice_wait)
    CLIP_ENTRY(PASSPORT_CLIP_ERROR, voice_error)
    CLIP_ENTRY(PASSPORT_CLIP_NEW_MSG, voice_new_msg)
    default:
        return 0;
    }
}

#else

int passport_clip_blob(passport_clip_id_t id, passport_clip_blob_t *blob)
{
    (void)id;
    if (blob) {
        blob->data = NULL;
        blob->size = 0;
    }
    return 0;
}

#endif
