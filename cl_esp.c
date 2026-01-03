#include "cl_esp.h"
#include "cl_local.h"
#include "common.h"
#include "ref_api.h"
#include "cl_entity.h"

void ESP_Init(void) { }
void ESP_Shutdown(void) { }

void ESP_Render(void) {
    if (!cl.refdef) return;
    for (int i = 0; i < MAX_EDICTS; i++) {
        cl_entity_t *ent = &cl_entities[i];
        if (!ent->model || ent->model->type != mod_brush) continue;
        vec3_t screen;
        if (WorldToScreen(ent->origin, screen)) {
            HUD_DrawString(screen[0], screen[1], "ESP", 255, 255, 0);
        }
    }
}
