## Project Structure
.
├── include/ # Public headers

│ ├── core/ # Core agent classes

│ ├── config/ # Configuration management

│ ├── network/ # HTTP/HTTPS client

│ └── ...

├── src/ # Implementation files

│ ├── core/

│ ├── config/

│ └── ...

├── tests/ # Unit tests

├── config/ # Configuration files

├── logs/ # Log files (gitignored)

├── tasks/ # Task storage (gitignored)

└── results/ # Results storage (gitignored)

## Настройка VS Code (для разработчиков)

Чтобы убрать красные волнистые линии в VS Code:

1. Установи расширение "C/C++" (ms-vscode.cpptools)
2. Открой проект и нажми `Ctrl+Shift+P`
3. Выбери "C/C++: Edit Configurations (UI)"
4. В поле "Include path" добавь:
   - `${workspaceFolder}/include`
   - `${workspaceFolder}/src`
5. Или просто пересобери проект:
   ```bash
   cd build && cmake .. && make

## Кроссплатформенная сборка и запуск

Проект собирается через CMake и использует C++17.

### macOS / Linux

```bash
cmake -S . -B build
cmake --build build -j
./build/web_agent
ctest --test-dir build --output-on-failure
```

### Windows (PowerShell)

```powershell
cmake -S . -B build
cmake --build build --config Release
.\build\Release\web_agent.exe
ctest --test-dir build -C Release --output-on-failure
```

Зависимости:
- `libcurl`
- `nlohmann/json` (если не установлена в системе, подтягивается автоматически через CMake `FetchContent`)
