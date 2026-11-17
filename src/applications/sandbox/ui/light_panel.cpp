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

void LightPanel::draw(const Window &window, UI &ui, Settings &settings)
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
        if (layout.button(lightTypeLabel(settings.lightType)))
            cycleLightType(settings.lightType);

        const float step = 0.25f;
        const float limit = 10.0f;

        layout.stepper("Posição X", settings.lightPosition.x, step, -limit, limit, 2);
        layout.stepper("Posição Y", settings.lightPosition.y, step, -limit, limit, 2);
        layout.stepper("Posição Z", settings.lightPosition.z, step, -limit, limit, 2);

        layout.stepper("Alvo X", settings.lightTarget.x, step, -limit, limit, 2);
        layout.stepper("Alvo Y", settings.lightTarget.y, step, -limit, limit, 2);
        layout.stepper("Alvo Z", settings.lightTarget.z, step, -limit, limit, 2);

        layout.colorPicker("Cor", settings.lightColor);

        if (layout.button("Restaurar luz"))
        {
            const Settings defaults;
            settings.lightPosition = defaults.lightPosition;
            settings.lightTarget = defaults.lightTarget;
            settings.lightColor = defaults.lightColor;
        } });
}
