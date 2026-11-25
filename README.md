# OpenGL — Ambiente Base Configurado para A3

Engine gráfica desenvolvida em C++17 utilizando OpenGL, GLFW, GLAD e CMake. O projeto possui uma arquitetura modular que permite compartilhar o motor gráfico entre diferentes aplicações.

Atualmente, o projeto reúne três aplicações principais: **Rotor Sandbox**, **Dynamis** e **Rotor Launcher**, além da integração com DOOM.

## 📋 Pré-requisitos

Antes de compilar, instale:

- **Git** (opcional, caso baixe o projeto pelo ZIP).
- **CMake 3.20 ou superior**.
- **MSYS2 com o ambiente UCRT64**, incluindo GCC e G++.
- Drivers gráficos compatíveis com OpenGL.
- **Visual Studio Code** (opcional, recomendado para desenvolvimento).

## 📦 Bibliotecas e tecnologias utilizadas

| Tecnologia | Utilização |
|---|---|
| C++17 | Linguagem principal do projeto |
| OpenGL | Renderização gráfica |
| GLFW 3.5.1 | Criação de janelas e gerenciamento de entrada |
| GLAD | Carregamento das funções OpenGL |
| GLM | Operações matemáticas e transformações 3D |
| nlohmann/json | Persistência e leitura de arquivos JSON |
| STB Image | Carregamento de imagens e texturas |
| STB TrueType | Recursos de fonte |
| CMake | Configuração e compilação |
| DoomGeneric | Integração do DOOM |

Os códigos-fonte e arquivos necessários das bibliotecas estão incluídos no repositório.

## 🏗️ Arquitetura do projeto

O projeto utiliza uma arquitetura compartilhada:

**Rotor Engine**

Biblioteca estática responsável pelos sistemas centrais do motor gráfico.

**Aplicações**

- **Rotor Sandbox:** ambiente de edição, visualização e simulação de cenas 3D.
- **Dynamis:** aplicação independente de experimentação com física.
- **Rotor Launcher:** inicializador que descobre e executa aplicações compatíveis com o Rotor Engine.

As aplicações reutilizam os módulos disponíveis no motor, evitando duplicação de implementações.

## 📥 Como baixar o projeto

1. Abra o repositório no GitHub.
2. Clique em **Code → Download ZIP**.
3. Extraia o arquivo ZIP para uma pasta de sua preferência.
4. Abra o PowerShell ou terminal na pasta que contém o arquivo `CMakeLists.txt`.

### ⚠️ Atenção ao extrair o ZIP

Dependendo de como o ZIP for extraído, pode ser criada uma pasta com o nome do projeto dentro da pasta onde você o extraiu.

Exemplo:

```text
Open-GL-Ambiente-Base-Configurado-Para-A3-/
└── A3-Computacao-Grafica/
    ├── CMakeLists.txt
    ├── CMakePresets.json
    ├── Libraries/
    ├── external/
    ├── assets/
    ├── src/
    └── README.md
```

Nesse caso, entre na pasta interna, pois é nela que está o `CMakeLists.txt`.

Os comandos de compilação devem ser executados na raiz real do projeto.

## ⚙️ Como configurar o ambiente no Windows

### 1. Instalar o MSYS2

Baixe e instale o MSYS2:

https://www.msys2.org/

Após a instalação, abra o terminal **MSYS2 UCRT64**.

### 2. Instalar o compilador C/C++ e as ferramentas

No terminal MSYS2 UCRT64, execute:

```bash
pacman -S --needed base-devel mingw-w64-ucrt-x86_64-toolchain mingw-w64-ucrt-x86_64-cmake
```

Confirme a instalação quando solicitado.

Esse comando instala o compilador GCC/G++, o CMake e as ferramentas de desenvolvimento necessárias.

### 3. Configurar a variável PATH

Adicione o seguinte diretório à variável de ambiente `Path` do Windows:

```text
C:\msys64\ucrt64\bin
```

Depois de alterar a variável, abra um novo terminal PowerShell.

### 4. Verificar a instalação

Execute:

```powershell
g++ --version
cmake --version
```

Se os comandos exibirem as versões instaladas, as ferramentas estão disponíveis.

No VS Code, abra a pasta do projeto e selecione o preset:

**MinGW (MSYS2 UCRT64)**

## 🔨 Como compilar

Abra o PowerShell na raiz do projeto, na mesma pasta onde está o arquivo `CMakeLists.txt`.

### 1. Configurar o projeto

```powershell
cmake --preset mingw
```

O preset utiliza o gerador **MinGW Makefiles** e configura os arquivos de compilação na pasta `build`.

### 2. Compilar

```powershell
cmake --build --preset mingw --parallel
```

O processo compila o motor compartilhado, suas dependências e as aplicações.

### Principais alvos de compilação

| Alvo | Descrição |
|---|---|
| `rotor_engine` | Biblioteca central do Rotor Engine |
| `OpenGLProjeto` | Aplicação Rotor Sandbox |
| `Dynamis` | Aplicação de experimentação física |
| `RotorLauncher` | Inicializador de aplicações |
| `DoomCore` | Núcleo do DoomGeneric |
| `DoomGeneric` | Executável independente do DoomGeneric |
| `physics_world_tests` | Testes automatizados da física |

## ▶️ Como executar

Após uma compilação bem-sucedida, os executáveis ficam na pasta `build`.

### Rotor Sandbox

```powershell
.\build\OpenGLProjeto.exe
```

Ambiente principal para visualização, edição e simulação de cenas 3D.

### Dynamis

```powershell
.\build\Dynamis.exe
```

Aplicação independente para experimentos de física utilizando o Rotor Engine.

### Rotor Launcher

```powershell
.\build\RotorLauncher.exe
```

Inicializador responsável por localizar e executar aplicações compatíveis com o Rotor Engine.

### DoomGeneric

```powershell
.\build\DoomGeneric.exe
```

Executável independente do port DoomGeneric.

## 🚀 Rotor Launcher

O Rotor Launcher permite descobrir e iniciar aplicações que utilizam o Rotor Engine.

A descoberta é realizada por meio de arquivos de manifesto com o sufixo:

```text
.rotor-app.json
```

Cada manifesto contém informações como:

- Versão do manifesto.
- Identificador único da aplicação.
- Nome da aplicação.
- Descrição.
- Executável correspondente.
- Versão compatível da API do Rotor Engine.

O CMake gera automaticamente os manifestos das aplicações incluídas no projeto:

```text
sandbox.rotor-app.json
dynamis.rotor-app.json
```

O Launcher procura manifestos válidos no diretório do próprio executável e apresenta as aplicações encontradas.

Essa estrutura permite adicionar novas aplicações compatíveis sem precisar registrar manualmente cada uma no código do Launcher.

## 🎮 Rotor Sandbox

O Rotor Sandbox é o ambiente principal de experimentação gráfica do projeto.

Permite carregar modelos 3D, editar objetos, manipular geometrias, configurar iluminação e executar simulações físicas.

### Editor 3D

O Editor possui ferramentas para manipulação de objetos e faces.

Principais ferramentas:

- **Move:** movimentação de objetos nos eixos 3D.
- **Rotate:** rotação de objetos.
- **Scale:** alteração de escala.
- **Extrude:** extrusão de faces da geometria.

O Editor também possui seleção de objetos, seleção de faces, gizmos de transformação e barra de ferramentas.

### Histórico de alterações

O Editor possui um sistema de desfazer utilizando:

```text
Ctrl + Z
```

O histórico contempla operações de transformação, extrusão e alterações da iluminação registradas pelo Editor.

### Gerenciamento de cenas

O Sandbox possui recursos para salvar, carregar e restaurar cenas.

As cenas são armazenadas em arquivos JSON.

O sistema registra:

- Nome dos objetos.
- Índice da instância.
- Posição.
- Rotação.
- Escala.
- Visibilidade.
- Caminho de origem dos modelos.
- Geometrias extrudadas.
- Propriedades da fonte de luz.

O sistema também salva automaticamente o estado da cena ao encerrar a aplicação e tenta recuperá-lo na próxima inicialização.

O menu permite salvar cenas com nomes próprios e selecionar arquivos JSON existentes.

Diretório de cenas:

```text
src/applications/sandbox/data/
```

### Renderização e iluminação

O Renderer é responsável por desenhar a cena e gerenciar os recursos gráficos.

Recursos disponíveis:

- Shaders de renderização.
- Materiais e texturas.
- Iluminação.
- Reflexões ambientais.
- Skybox.
- Marcador visual da fonte de luz.
- Renderização de modelos e malhas.
- Efeitos de chuva.

### Configurações do Sandbox

O menu de configurações possui categorias:

| Categoria | Recursos |
|---|---|
| Câmera | Campo de visão, velocidade de caminhada, velocidade com Shift e sensibilidade |
| Ambiente | Skybox, chuva e velocidade da física |
| Iluminação | Luz ambiente difusa e reflexão ambiental |
| Janela | Janela normal, maximizada, sem bordas e tela cheia |
| DOOM | Inicialização do jogo integrado |

### Modelos 3D

Os modelos utilizados pelo Sandbox estão organizados em:

```text
assets/sandbox/models/
```

O projeto trabalha com modelos no formato GLB, incluindo objetos como mesa, tablet, mouse, teclado, monitor, celular, caderno, caneca, lápis, lâmpada e outros elementos de composição da cena.

As texturas utilizadas estão em:

```text
assets/sandbox/textures/
```

### Atalhos do Sandbox

| Tecla | Ação |
|---|---|
| `ESC` | Abrir ou fechar o menu |
| `TAB` | Alternar entre navegação e edição |
| `CTRL + Z` | Desfazer a última alteração do Editor |
| `1` a `7` | Visualizar objetos específicos da cena |
| `F8` | Visualizar a foto |
| `F9` | Visualizar o porta-caneta |
| `F10` | Visualizar a suculenta |
| `0` | Exibir todos os objetos |
| `F4` | Ativar ou desativar o profiling da física |

Durante a execução do DOOM, `F10` retorna ao Sandbox.

## ⚙️ Sistema de Física — Rotor Engine

O motor possui um sistema próprio de física desenvolvido em C++.

A implementação é organizada em módulos de corpos rígidos, colisores, geometria de colisão e resolução de contatos.

### Recursos físicos

- Gravidade.
- Corpos dinâmicos.
- Esferas e caixas orientadas (OBB).
- Colisores baseados em malhas trianguladas.
- Colisores estáticos.
- Plano de chão.
- Superfície de bacia.
- Detecção e resolução de colisões.
- Restituição.
- Atrito estático e dinâmico.
- Aplicação de forças e torques.
- Forças contínuas.
- Impulsos lineares e angulares.
- Amortecimento linear e angular.
- Escala de tempo da simulação.
- Estabilização inicial de sobreposições.
- Captura e restauração da pose inicial.

O sistema permite integrar os corpos físicos às transformações dos objetos da cena.

### Profiling da física

O profiling pode ser ativado ou desativado pelo atalho:

```text
F4
```

Quando ativado, gera informações de desempenho no console e no arquivo:

```text
physics_profile.log
```

## 🧪 Dynamis

O Dynamis é uma aplicação independente desenvolvida para demonstrar e experimentar os recursos físicos do Rotor Engine.

O cenário atual possui uma bacia procedural e duas esferas com massas diferentes.

### Experimento de rolamento

A aplicação demonstra:

- Movimento de corpos esféricos.
- Gravidade.
- Atrito com a superfície.
- Rolamento.
- Colisão entre corpos.
- Transferência de movimento.
- Influência da massa no comportamento da simulação.

As esferas utilizam padrões visuais diferentes para facilitar a observação de sua rotação.

### Controles

| Tecla | Ação |
|---|---|
| `TAB` | Alternar entre simulação e edição |
| `W`, `A`, `S`, `D` | Aplicar força ao corpo dinâmico selecionado |
| `R` | Reiniciar o experimento |

Quando um corpo dinâmico está selecionado durante a simulação, a câmera pode acompanhar e orbitar esse objeto.

No modo de edição, a física fica pausada e o Editor disponibiliza suas ferramentas de transformação.

## 🎮 Integração com DOOM

O projeto possui integração com o DoomGeneric, permitindo iniciar DOOM diretamente dentro da aplicação Rotor Sandbox.

Para utilizar essa funcionalidade, é necessário disponibilizar um arquivo `doom1.wad` compatível.

O executável procura esse arquivo no mesmo diretório de `OpenGLProjeto.exe`.

### 1. Configurar o arquivo WAD

Após compilar, copie o arquivo para a pasta `build`:

```powershell
Copy-Item ".\external\doom_licença\doom1.wad" ".\build\doom1.wad"
```

Utilize somente arquivos de jogo que você tenha autorização para utilizar.

### 2. Executar o Sandbox

```powershell
.\build\OpenGLProjeto.exe
```

### 3. Iniciar o jogo

No Sandbox:

1. Pressione `ESC`.
2. Entre em **Configurações**.
3. Acesse a categoria **DOOM**.
4. Selecione **Iniciar DOOM**.

O jogo é executado dentro da mesma janela da aplicação.

Para retornar ao Sandbox, pressione:

```text
F10
```

## 🧪 Testes automatizados

O projeto possui testes automatizados para o sistema de física.

O arquivo de testes está localizado em:

```text
tests/physics_world_tests.cpp
```

Para executar os testes após a compilação:

```powershell
ctest --test-dir build --output-on-failure
```

O CTest apresenta o resultado de cada teste e informa eventuais falhas.

## 📁 Estrutura atual do projeto

A estrutura abaixo representa a organização modular encontrada no projeto.

```text
.
├── .vscode/
│
├── assets/
│   ├── editor/
│   │   └── icons/
│   │
│   ├── engine/
│   │   ├── fonts/
│   │   ├── shaders/
│   │   ├── skybox/
│   │   └── textures/
│   │
│   └── sandbox/
│       ├── models/
│       └── textures/
│
├── Libraries/
│   ├── glad/
│   ├── glfw-3.5.1/
│   └── include/
│       ├── glm/
│       ├── glad/
│       ├── KHR/
│       ├── json/
│       ├── stb_image.h
│       └── stb_truetype.h
│
├── external/
│   ├── doom_licença/
│   ├── doomgeneric/
│   │   └── doomgeneric/
│   └── nlohmann/
│
├── src/
│   ├── applications/
│   │   ├── dynamis/
│   │   ├── launcher/
│   │   └── sandbox/
│   │       ├── data/
│   │       ├── doom/
│   │       └── ui/
│   │
│   └── engine/
│       ├── application/
│       ├── editor/
│       ├── graphics/
│       │   ├── buffers/
│       │   ├── camera/
│       │   ├── environment/
│       │   ├── geometry/
│       │   ├── mesh/
│       │   ├── model/
│       │   └── resources/
│       │
│       ├── physics/
│       │   ├── collision/
│       │   └── solver/
│       │
│       ├── platform/
│       ├── renderer/
│       ├── scene/
│       │   └── scenarios/
│       └── ui/
│
├── tests/
│   └── physics_world_tests.cpp
│
├── CMakeLists.txt
├── CMakePresets.json
├── LICENSE
├── README.md
└── .gitignore
```

## 🛠️ Desenvolvimento

O projeto utiliza C++17 e CMake.

O motor gráfico está separado das aplicações para facilitar a reutilização de funcionalidades e a criação de novos projetos compatíveis com o Rotor Engine.

As principais responsabilidades estão organizadas em módulos:

- **Application:** ciclo de vida das aplicações.
- **Platform:** gerenciamento de janelas e entradas.
- **Graphics:** geometria, modelos, malhas, buffers, câmera e recursos gráficos.
- **Renderer:** renderização da cena e dos efeitos visuais.
- **Physics:** corpos rígidos, colisões e resolução de contatos.
- **Scene:** objetos, transformações, iluminação e persistência.
- **Editor:** seleção, manipulação e edição de objetos.
- **UI:** interface e componentes visuais.

As configurações de desenvolvimento do VS Code ficam em `.vscode/`.

Os arquivos gerados durante a compilação ficam na pasta `build/` e não precisam ser versionados.

## 📄 Licença

Consulte os arquivos `LICENSE` e os documentos de licença das dependências incluídas no projeto para conhecer os respectivos termos de uso e distribuição.