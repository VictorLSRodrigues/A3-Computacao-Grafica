#include "light_panel.h"

#include "engine/ui/panel.h"

namespace
{
    const char *lightTypeLabel(LightType type)
    {
        switch (type)
        {
        case LightType::Directional:
            return "Tipo: Direcional";
        case LightType::Spot:
            return "Tipo: Holofote";
        case LightType::Point:
        default:
            return "Tipo: Pontual";
        }
    }

    void cycleLightType(LightType &type)
    {
        switch (type)
        {
        case LightType::Point:
            type = LightType::Directional;
            break;
        case LightType::Directional:
            type = LightType::Spot;
            break;
        case LightType::Spot:
        default:
            type = LightType::Point;
            break;
        }
    }
}

void LightPanel::draw(const Window &window, UI &ui, Light &light)
{
    PanelStyle style;
    style.width = 370.0f;
    style.margin = 18.0f;
    style.titleHeight = 48.0f;
    style.rowHeight = 34.0f;
    style.gap = 8.0f;
    style.dimBackground = false;

    drawRightPanel(window, ui, "Propriedades da luz", style, [&](Layout &layout)
                   {
        if (layout.button(lightTypeLabel(light.type)))
            cycleLightType(light.type);

        const float step = 0.25f;
        const float limit = 10.0f;

        layout.stepper("Posição X", light.position.x, step, -limit, limit, 2);
        layout.stepper("Posição Y", light.position.y, step, -limit, limit, 2);
        layout.stepper("Posição Z", light.position.z, step, -limit, limit, 2);

        layout.stepper("Alvo X", light.target.x, step, -limit, limit, 2);
        layout.stepper("Alvo Y", light.target.y, step, -limit, limit, 2);
        layout.stepper("Alvo Z", light.target.z, step, -limit, limit, 2);

        layout.colorPicker("Cor", light.color);
        layout.stepper("Intensidade", light.intensity, 0.25f, 0.0f, 10.0f, 2);

        if (layout.button("Restaurar luz"))
        {
            // Restaura posição, alvo e cor; o tipo de luz escolhido é mantido.
            const LightType type = light.type;
            light.resetProperties();
            light.type = type;
        } });
}
