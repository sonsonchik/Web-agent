# Web Agent

C++ агент, который:
- регистрируется на сервере;
- получает задания;
- выполняет команды или отправляет файлы;
- отправляет результат выполнения обратно на сервер.

## Требования

- CMake >= 3.16
- C++17 compiler (`clang++`, `g++` или MSVC)
- Опционально: `libcurl` (если нужен реальный HTTP/HTTPS; без него работает mock-сетевой режим)

## Структура проекта

- `include/` — заголовки
- `src/` — реализация
- `tests/` — unit-тесты
- `config/agent_config.yaml` — конфигурация агента
- `logs/` — логи
- `tasks/` — рабочая директория для файлов задач
- `results/` — результаты выполнения
- `.github/workflows/release.yml` — CI/CD сборка и публикация релизов

## Сборка

### Linux / macOS

```bash
cmake -S . -B build -DWEB_AGENT_BUILD_TESTS=ON
cmake --build build
```

### Windows (PowerShell)

```powershell
cmake -S . -B build -DWEB_AGENT_BUILD_TESTS=ON
cmake --build build --config Release
```

## Запуск

### Обычный режим (постоянный polling)

```bash
./build/web-agent --config config/agent_config.yaml
```

На macOS бинарник может лежать внутри `.app`:

```bash
./build/web-agent.app/Contents/MacOS/web-agent --config config/agent_config.yaml
```

На Windows:

```powershell
.\build\Release\web-agent.exe --config config\agent_config.yaml
```

### Один цикл (для теста)

```bash
./build/web-agent --config config/agent_config.yaml --once
```

## Тесты

```bash
ctest --test-dir build --output-on-failure
```

Windows:

```powershell
ctest --test-dir build -C Release --output-on-failure
```

## Конфиг агента

Файл: `config/agent_config.yaml`

Поддерживаемые поля:
- `uid` — идентификатор агента
- `server` — базовый URL сервера
- `register_path` — endpoint регистрации агента
- `task_path` — endpoint получения задания
- `result_path` — endpoint отправки результата
- `ping_path` — endpoint health-check
- `access_code` — код доступа агента (если требуется сервером)
- `interval` — интервал опроса (сек)
- `max_interval` — максимальный интервал при backoff (сек)
- `backoff_multiplier` — множитель backoff
- `tasks_dir` — директория входных файлов
- `results_dir` — директория результатов
- `log_file` — путь к лог-файлу

Пример для сервера XDEV:
```yaml
uid: 77777aaaaaa
server: https://xdev.arkcom.ru:9999/app/webagent1/api
register_path: /wa_reg/
task_path: /wa_task/
result_path: /wa_result/
ping_path: /wa_reg/
access_code: efcaf0-33e9-0fff-3ff2-ceaa255f
```

## Протокол заданий

Агент принимает payload в одном из форматов:
- `key=value` по строкам;
- `key: value` по строкам;
- JSON-объект.

Нормализация типов:
- `TASK` / `RUN_PROGRAM` -> `run_program`
- `FILE` / `SEND_FILE` -> `send_file`
- `CONF` / `CONFIG` / `UPDATE_CONFIG` -> `update_config`
- `TIMEOUT` / `UPDATE_TIMEOUT` -> `update_timeout`

### Обязательные/важные поля задания

Общие поля:
- `task_id`
- `session_id` (если сервер использует сессии)
- `type`

Дополнительные поля по типу задания описаны ниже.

## Описание передаваемых опций по заданиям

### 1) TASK (`run_program`)

Назначение: запустить программу/команду и вернуть лог выполнения.

Поддерживаемые поля:
- `command` — исполняемая команда (предпочтительно)
- `args` — аргументы команды (опционально)
- `output_file` — имя файла лога в `results_dir` (опционально)
- `options` / `data` — fallback для команды, если `command` не передан

Правила:
- если `command` пуст, агент возьмет команду из `options`;
- stdout/stderr редиректятся в `output_file`;
- если `output_file` не задан, используется `<task_id>.log` или `result.log`.

Пример (key=value):
```text
task_id=101
session_id=s1
type=TASK
command=python3
args=--version
output_file=python_version.log
```

### 2) FILE (`send_file`)

Назначение: отправить файл на сервер.

Поддерживаемые поля:
- `output_file` — путь/имя файла (предпочтительно)
- `options` / `data` / `file` / `path` — альтернативные ключи пути

Правила поиска файла:
- если путь абсолютный — используется как есть;
- если относительный — агент проверяет:
  - `<relative>`
  - `<tasks_dir>/<relative>`
  - `<results_dir>/<relative>`

Что агент отправляет в `/result`:
- `file_name`
- `file_size`
- `file_content_base64`
- `result_path`

Пример (JSON):
```json
{
  "task_id": "102",
  "session_id": "s1",
  "type": "FILE",
  "options": "note.txt"
}
```

### 3) CONF (`update_config`)

Назначение: изменить runtime-параметр конфигурации агента.

Поддерживаемые поля:
- `options` (или `data`) в формате `key=value`

Поддерживаемые ключи:
- `interval` или `poll_interval_sec`
- `max_interval` или `max_poll_interval_sec`
- `backoff_multiplier`
- `server` или `server_uri`

Пример:
```text
task_id=103
type=CONF
options=interval=15
```

### 4) TIMEOUT (`update_timeout`)

Назначение: изменить интервал опроса `poll_interval_sec`.

Поддерживаемые поля:
- `options` или `args`

Допустимый формат значения:
- `25`
- `timeout=25`

Пример:
```text
task_id=104
type=TIMEOUT
options=25
```

## Формат результата (`/result`)

Агент отправляет JSON с полями:
- `uid`
- `task_id`
- `session_id`
- `task_type`
- `exit_code`
- `result_path`
- `message`
- `file_name`
- `file_size`
- `file_content_base64`
- `error`

Примечания:
- для `TASK` обычно заполнены `exit_code`, `result_path`, `error`;
- для `FILE` дополнительно заполнены `file_*`;
- для `CONF/TIMEOUT` используется `message` с подтверждением применения.

## CI/CD релизы

Workflow: `.github/workflows/release.yml`

Что происходит при каждом `push`:
- сборка на `ubuntu-latest`, `macos-latest`, `windows-latest`;
- прогон тестов;
- упаковка артефактов;
- публикация prerelease в GitHub Releases с тегом `build-<commit_sha>`.

Публикуемые файлы:
- Linux: `web-agent-linux-x86_64.tar.gz`
- macOS: `web-agent-macos.app.zip` (one-click после распаковки `.app`)
- Windows: `web-agent-windows.exe` (one-click)
