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
4. Abra o PowerShell ou terminal na pasta extraída.

## ⚙️ Como configurar o ambiente no Windows

Instale o CMake e o MSYS2 com o compilador GCC/G++ do ambiente UCRT64.

Certifique-se de que o executável `g++.exe` esteja disponível em:

`C:\msys64\ucrt64\bin\g++.exe`

Adicione o diretório do CMake e, se necessário, o diretório `C:\msys64\ucrt64\bin` à variável de ambiente `PATH`.

Para verificar as instalações, execute:

```powershell
cmake --version
g++ --version
```

## 🔨 Como compilar

Na raiz do projeto, execute:

```powershell
cmake -S . -B build -G "MinGW Makefiles"
```

Depois, compile:

```powershell
cmake --build build
```

O CMake configurará o GLFW a partir do código-fonte incluído no projeto.

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
