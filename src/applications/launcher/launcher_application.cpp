#include "launcher_application.h"

#include <algorithm>
#include <cmath>

#include <glad/glad.h>
#include <glm/glm.hpp>

#include "engine/ui/theme.h"
#include "process_launcher.h"

namespace
{
    constexpr float outerMargin = 48.0f;
    constexpr float cardGap = 18.0f;
    constexpr float headerHeight = 126.0f;
    constexpr float footerHeight = 58.0f;
    constexpr float cardHeight = 245.0f;
    constexpr float minimumCardWidth = 280.0f;
    constexpr std::size_t maximumColumns = 3;

    const glm::vec3 &textColor() { return theme::text; }
    const glm::vec3 &mutedColor() { return theme::textMuted; }

    std::string fitText(UI &ui, const std::string &text, float scale, float maxWidth)
    {
        if (ui.measureText(text, scale) <= maxWidth)
            return text;

        std::string shortened = text;
        while (!shortened.empty() && ui.measureText(shortened + "...", scale) > maxWidth)
            shortened.pop_back();

        if (shortened.empty())
            return {};
        return shortened + "...";
    }
}

namespace
{
    WindowConfig createWindowConfig()
    {
        WindowConfig config;
        config.title = "Rotor | Launcher";
        config.mode = WindowMode::Windowed;
        config.width = 1060;
        config.height = 700;
        config.minWidth = 900;
        config.minHeight = 620;
        return config;
    }
}

LauncherApplication::LauncherApplication()
    : rotor::Application(createWindowConfig())
{
}

bool LauncherApplication::onInitialize()
{
    if (!ui.create())
        return false;

    const std::filesystem::path executableDirectory = launcher::executableDirectory();
    applications = launcher::discoverCompatibleApplications(executableDirectory);
    return true;
}

void LauncherApplication::onRender()
{
    glViewport(0, 0, window.getWidth(), window.getHeight());
    glClearColor(0.035f, 0.050f, 0.075f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    draw();
}

void LauncherApplication::draw()
{
    const float screenWidth = static_cast<float>(window.getWidth());
    const float screenHeight = static_cast<float>(window.getHeight());

    ui.begin(window);

    ui.drawText("ROTOR", outerMargin, 62.0f, 1.05f,
                textColor().r, textColor().g, textColor().b);
    ui.drawText("APPLICATION LAUNCHER", outerMargin + 2.0f, 91.0f, 0.40f,
                mutedColor().r, mutedColor().g, mutedColor().b);

    const std::string foundLabel = std::to_string(applications.size()) +
                                   (applications.size() == 1 ? " APPLICATION FOUND" : " APPLICATIONS FOUND");
    const float foundWidth = ui.measureText(foundLabel, 0.42f);
    ui.drawText(foundLabel, screenWidth - outerMargin - foundWidth, 68.0f, 0.42f,
                theme::accent.r, theme::accent.g, theme::accent.b);
    ui.drawLine(outerMargin, headerHeight, screenWidth - outerMargin, headerHeight,
                1.0f, theme::panelBorder.r, theme::panelBorder.g, theme::panelBorder.b);

    const float availableWidth = std::max(1.0f, screenWidth - outerMargin * 2.0f);
    const float columnEstimate = std::floor((availableWidth + cardGap) /
                                            (minimumCardWidth + cardGap));
    const std::size_t columns = std::max<std::size_t>(
        1, std::min(maximumColumns, static_cast<std::size_t>(std::max(1.0f, columnEstimate))));
    const float cardWidth = (availableWidth - cardGap * static_cast<float>(columns - 1)) /
                            static_cast<float>(columns);

    const float cardTop = headerHeight + 38.0f;
    const float listBottom = screenHeight - footerHeight - 30.0f;
    const float listHeight = std::max(1.0f, listBottom - cardTop);
    const std::size_t visibleRows = std::max<std::size_t>(
        1, static_cast<std::size_t>(std::floor((listHeight + cardGap) / (cardHeight + cardGap))));
    const std::size_t totalRows = applications.empty()
                                     ? 0
                                     : (applications.size() + columns - 1) / columns;
    const std::size_t maximumFirstRow = totalRows > visibleRows ? totalRows - visibleRows : 0;

    // A roda do mouse percorre as linhas quando há mais aplicações do que cabem na janela.
    const double scroll = window.getScrollY();
    if (scroll > 0.0 && firstVisibleRow > 0)
        --firstVisibleRow;
    else if (scroll < 0.0 && firstVisibleRow < maximumFirstRow)
        ++firstVisibleRow;
    firstVisibleRow = std::min(firstVisibleRow, maximumFirstRow);

    if (applications.empty())
    {
        const std::string emptyTitle = "NENHUMA APLICACAO COMPATIVEL ENCONTRADA";
        const float titleWidth = ui.measureText(emptyTitle, 0.58f);
        ui.drawText(emptyTitle, (screenWidth - titleWidth) * 0.5f,
                    cardTop + 145.0f, 0.58f,
                    textColor().r, textColor().g, textColor().b);

        const std::string emptyDescription = "Aplicacoes Rotor com manifesto valido aparecerao aqui.";
        const float descriptionWidth = ui.measureText(emptyDescription, 0.42f);
        ui.drawText(emptyDescription, (screenWidth - descriptionWidth) * 0.5f,
                    cardTop + 181.0f, 0.42f,
                    mutedColor().r, mutedColor().g, mutedColor().b);
    }
    else
    {
        for (std::size_t i = 0; i < applications.size(); ++i)
        {
            const std::size_t row = i / columns;
            if (row < firstVisibleRow || row >= firstVisibleRow + visibleRows)
                continue;

            const std::size_t column = i % columns;
            const float x = outerMargin + static_cast<float>(column) * (cardWidth + cardGap);
            const float y = cardTop + static_cast<float>(row - firstVisibleRow) * (cardHeight + cardGap);
            drawApplicationCard(applications[i], x, y, cardWidth, cardHeight);
        }
    }

    if (!errorMessage.empty())
    {
        const float messageY = screenHeight - footerHeight - 8.0f;
        const std::string visibleError = fitText(ui, errorMessage, 0.40f,
                                                 availableWidth - 130.0f);
        ui.drawText(visibleError, outerMargin, messageY, 0.40f,
                    theme::buttonDanger.r, 0.55f, 0.58f);
    }

    std::string footerLabel = "ROTOR ENGINE  /  DEVELOPMENT BUILD";
    if (!applications.empty())
    {
        footerLabel += "  /  " + std::to_string(firstVisibleRow + 1) + "-" +
                       std::to_string(std::min(totalRows, firstVisibleRow + visibleRows)) +
                       " OF " + std::to_string(totalRows) + " ROWS";
    }
    ui.drawText(footerLabel, outerMargin, screenHeight - 32.0f,
                0.36f, mutedColor().r, mutedColor().g, mutedColor().b);

    const float exitWidth = 110.0f;
    if (ui.button("SAIR", screenWidth - outerMargin - exitWidth,
                  screenHeight - 58.0f, exitWidth, 36.0f,
                  theme::buttonNeutral.r, theme::buttonNeutral.g,
                  theme::buttonNeutral.b))
    {
        window.close();
    }

    ui.end();
}

void LauncherApplication::drawApplicationCard(const DiscoveredApplication &app,
                                               float x, float y,
                                               float width, float height)
{
    const glm::vec3 &background = theme::panelBackground;
    ui.drawRect(x, y, width, height, background.r, background.g, background.b, 0.98f);
    ui.drawRect(x, y, width, 2.0f,
                theme::accent.r, theme::accent.g, theme::accent.b);

    const float contentWidth = std::max(1.0f, width - 44.0f);
    const std::string visibleName = fitText(ui, app.name, 0.68f, contentWidth);
    ui.drawText(visibleName, x + 22.0f, y + 72.0f, 0.68f,
                textColor().r, textColor().g, textColor().b);

    const std::string visibleDescription = fitText(ui, app.description, 0.43f, contentWidth);
    ui.drawText(visibleDescription, x + 22.0f, y + 112.0f, 0.43f,
                mutedColor().r, mutedColor().g, mutedColor().b);

    ui.drawRect(x + 22.0f, y + 146.0f, 5.0f, 5.0f,
                theme::accent.r, theme::accent.g, theme::accent.b);
    ui.drawText("COMPATIVEL", x + 35.0f, y + 151.0f, 0.37f,
                theme::accent.r, theme::accent.g, theme::accent.b);

    const float buttonHeight = 42.0f;
    const float buttonY = y + height - buttonHeight - 22.0f;
    const float buttonWidth = width - 44.0f;
    if (ui.button("INICIAR", x + 22.0f, buttonY, buttonWidth, buttonHeight,
                  theme::buttonPrimary.r, theme::buttonPrimary.g,
                  theme::buttonPrimary.b))
    {
        launch(app);
    }
}

bool LauncherApplication::launch(const DiscoveredApplication &app)
{
    if (!launcher::startProcess(app.executablePath, app.executablePath.parent_path(), errorMessage))
        return false;

    // O Launcher encerra após iniciar uma aplicação; cada app tem seu próprio processo.
    window.close();
    return true;
}
