#pragma once

// Configurações de renderização compartilhadas pelo app e pela interface.
// Os dados da câmera ficam na Camera e os da iluminação, na Light.
struct Settings
{
    bool showSkybox = true; // mostra o céu ao redor da cena
};