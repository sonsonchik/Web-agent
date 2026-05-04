# Релизы для платформ в GitHub

Этот проект публикует платформенные релизы автоматически через GitHub Actions.

## Что публикуется
При push тега `v*` создается GitHub Release с артефактами:
- Linux (`.tar.gz`)
- macOS (`.tar.gz`)
- Windows (`.zip`)

В каждом артефакте:
- бинарник агента;
- `README.md`;
- папка `docs/`.

## Как выпустить релиз
```bash
git tag v1.0.0
git push origin v1.0.0
```

После этого workflow `.github/workflows/release.yml`:
1. Соберет проект на трех ОС.
2. Упакует платформенные артефакты.
3. Создаст/обновит GitHub Release и прикрепит файлы.

## Docker
`Dockerfile` добавлен как опция для воспроизводимой Linux-сборки и запуска.
Docker не заменяет нативные платформенные релизы для macOS/Windows.
