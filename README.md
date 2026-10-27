# OpenGL — Ambiente Base Configurado para A3

Projeto-base em C++ utilizando OpenGL, GLFW e GLAD, com configuração de compilação por meio do CMake.

## 📋 Pré-requisitos

Antes de compilar, instale:

- **Git** (opcional, caso baixe o projeto pelo ZIP).
- **CMake 3.20 ou superior**.
- **MSYS2 com o ambiente UCRT64**, incluindo GCC e G++.
- Drivers gráficos compatíveis com OpenGL.
- **Visual Studio Code** (opcional, recomendado para desenvolvimento).

## 📦 Bibliotecas utilizadas

- **OpenGL** — renderização gráfica.
- **GLFW 3.5.1** — criação de janelas e gerenciamento de entrada.
- **GLAD** — carregamento das funções OpenGL.
- **CMake** — configuração do processo de compilação.

O código-fonte do GLFW e os arquivos necessários do GLAD estão incluídos no repositório.

## 📥 Como baixar o projeto

1. Abra o repositório no GitHub.
2. Clique em **Code → Download ZIP**.
3. Extraia o arquivo ZIP para uma pasta de sua preferência.
4. Abra o PowerShell ou terminal na pasta que contém o arquivo `CMakeLists.txt`.

### ⚠️ Atenção ao extrair o ZIP

Dependendo de como o ZIP for extraído, pode ser criada uma pasta com o nome do projeto dentro da pasta onde você o extraiu.

Por exemplo:

```text
Open-GL-Ambiente-Base-Configurado-Para-A3-/
└── Open-GL-Ambiente-Base-Configurado-Para-A3-/
    ├── CMakeLists.txt
    ├── Libraries/
    ├── src/
    └── README.md
```

Nesse caso, **entre na pasta interna**, pois é nela que está o `CMakeLists.txt`.

Os comandos de compilação devem ser executados na raiz real do projeto, onde está esse arquivo. Se você executar os comandos na pasta externa, o CMake poderá informar que não encontrou o projeto.

## ⚙️ Como configurar o ambiente no Windows

### 1. Instalar o MSYS2

Baixe e instale o MSYS2 pelo site oficial:

https://www.msys2.org/

Após a instalação, abra o terminal **MSYS2 UCRT64**.

### 2. Instalar o compilador C/C++ e as ferramentas

No terminal MSYS2 UCRT64, execute:

```bash
pacman -S --needed base-devel mingw-w64-ucrt-x86_64-toolchain mingw-w64-ucrt-x86_64-cmake
```

Confirme a instalação quando solicitado. Se aparecer uma seleção de pacotes do grupo `toolchain`, pressione Enter para aceitar a seleção padrão e confirme quando solicitado.

Esse comando instala o compilador GCC/G++, o CMake e ferramentas de desenvolvimento necessárias.

### 3. Configurar a variável PATH

Adicione o seguinte diretório à variável de ambiente `Path` do Windows:

```text
C:\msys64\ucrt64\bin
```

Se o CMake tiver sido instalado separadamente, certifique-se também de que seu diretório esteja no `Path`.

Depois de alterar a variável, abra um novo terminal PowerShell.

### 4. Verificar a instalação

Execute:

```powershell
g++ --version
cmake --version
```

Se os comandos exibirem as versões instaladas, as ferramentas estão disponíveis no terminal.

## 🔨 Como compilar

Abra o PowerShell **na raiz do projeto**, na mesma pasta onde está o arquivo `CMakeLists.txt`.

### 1. Configurar o projeto

```powershell
cmake -S . -B build -G "MinGW Makefiles"
```

Esse comando configura o projeto e gera os arquivos de compilação na pasta `build`.

### 2. Compilar

```powershell
cmake --build build
```

O CMake configurará e compilará o GLFW a partir do código-fonte incluído no projeto, além de compilar os arquivos do programa.

## ▶️ Como executar

Após uma compilação bem-sucedida, execute:

```powershell
.\build\OpenGLProjeto.exe
```

O executável será gerado localmente durante a compilação. Ele não precisa estar incluído no repositório.

## 📁 Estrutura do projeto

```text
.
├── .vscode/
│   ├── c_cpp_properties.json
│   └── settings.json
├── Libraries/
│   ├── glad/
│   │   ├── include/
│   │   └── src/
│   ├── include/
│   │   ├── glad/
│   │   └── KHR/
│   └── glfw-3.5.1/
├── src/
│   └── main.cpp
├── CMakeLists.txt
├── README.md
└── .gitignore
```

## 🛠️ Desenvolvimento

O projeto utiliza C++17 e CMake. As configurações em `.vscode/` auxiliam o IntelliSense e a integração com o ambiente de desenvolvimento.

As pastas de compilação são geradas localmente e não precisam ser versionadas.

## 📄 Licença

Consulte o arquivo `LICENSE` incluído neste repositório para conhecer os termos de uso e distribuição.
