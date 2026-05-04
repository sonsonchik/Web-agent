## Web Agent

C++17-агент, который регистрируется на сервере, получает задания и отправляет результаты выполнения.

## Документация
- Пользовательская документация: [docs/user-guide.md](docs/user-guide.md)
- Описание опций заданий: [docs/task-options.md](docs/task-options.md)
- Релизы для платформ: [docs/releases.md](docs/releases.md)

## Структура проекта
```text
.
├── include/               # Публичные заголовки
├── src/                   # Исходники
├── tests/                 # Тесты
├── config/                # Конфигурация
├── docs/                  # Документация пользователя и заданий
├── logs/                  # Логи (gitignored)
├── tasks/                 # Временные данные задач (gitignored)
└── results/               # Результаты (gitignored)
```

## Сборка и запуск

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
- `nlohmann/json` (подтягивается через CMake `FetchContent`, если не установлена в системе)

## Релизы для платформ
Релизы публикуются в GitHub Releases автоматически по тегу `v*`.

Что публикуется:
- Linux-архив с бинарником
- macOS-архив с бинарником
- Windows-архив с бинарником `.exe`

Как выпустить релиз:
```bash
git tag v1.0.0
git push origin v1.0.0
```

Workflow-файлы:
- `.github/workflows/ci.yml` — сборка и тесты на Linux/macOS/Windows
- `.github/workflows/release.yml` — публикация релизных артефактов в GitHub

## Docker (опционально)
Для воспроизводимой Linux-сборки добавлен `Dockerfile`.

```bash
docker build -t web-agent:local .
docker run --rm web-agent:local
```
