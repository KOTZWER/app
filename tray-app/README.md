# TrayApp

Минималистичное Windows-приложение, работающее в системном трее. Реализовано на C++ с использованием Win32 API и собирается через CMake.

---

## Требования к окружению

| Инструмент | Версия |
|---|---|
| Windows | 10 / 11 |
| MSVC (Visual Studio Build Tools) | 2019 или 2022 |
| CMake | ≥ 3.20 |
| Python 3 | только для генерации иконки, опционально |

---

## Сборка локально

```bat
:: 1. Открыть «Developer Command Prompt» для VS 2022 (x64)

:: 2. Клонировать репозиторий
git clone <repo-url>
cd tray-app

:: 3. (Опционально) сгенерировать иконку, если её ещё нет
python resources\generate_icon.py

:: 4. Сконфигурировать
cmake -B build -G "NMake Makefiles" -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=dist

:: 5. Собрать
cmake --build build --config Release

:: 6. Установить / собрать артефакты
cmake --install build --config Release
```

Готовый `TrayApp.exe` окажется в папке `dist\bin\`.

---

## Запуск

```bat
:: Обычный запуск (главное окно показывается сразу)
TrayApp.exe

:: Запуск в скрытом режиме (только трей)
TrayApp.exe --hidden
```

---

## Функциональность

| # | Требование | Реализация |
|---|---|---|
| 1 | Иконка в трее при запуске | `Shell_NotifyIcon(NIM_ADD, ...)` в `WinMain` |
| 2 | ЛКМ по иконке → главное окно | `WM_TRAYICON` + `WM_LBUTTONUP` |
| 3 | ПКМ по иконке → контекстное меню | `WM_TRAYICON` + `WM_RBUTTONUP` |
| 4 | Пункт «Открыть» в контекстном меню | `IDM_OPEN` → `ShowMainWindow()` |
| 5 | Пункт «Выход» в контекстном меню | `IDM_EXIT` → `ExitApp()` |
| 6 | Пересоздание панели задач → повторное добавление иконки | `RegisterWindowMessage("TaskbarCreated")` |
| 7 | Запуск без показа главного окна | аргумент командной строки `--hidden` |
| 8 | Закрытие окна → фоновый режим | `WM_CLOSE` → `HideMainWindow()` (без `DestroyWindow`) |
| 9 | Меню «Файл → Выход» в главном окне | `CreateMainMenu()` + `IDM_FILE_EXIT` |
| 10 | Единственный экземпляр на пользователя | `CreateMutex` + `Local\` namespace |
| 11 | Сборка на конвейере | GitHub Actions (`.github/workflows/build.yml`) и GitLab CI (`.gitlab-ci.yml`) |
| 12 | Артефакт — `TrayApp.exe` | `cmake --install` + `upload-artifact` |

---

## CI/CD

### GitHub Actions

Файл: `.github/workflows/build.yml`

- Триггеры: `push` и `pull_request` на ветки `main` / `master` / `develop`.
- Раннер: `windows-latest` (MSVC + CMake предустановлены).
- Артефакт: `TrayApp-x64-Release` → `dist/bin/TrayApp.exe`.

### GitLab CI

Файл: `.gitlab-ci.yml`

- Требует Windows-раннер с тегом `windows` и установленными VS 2022 Build Tools.
- Артефакт: `dist/bin/TrayApp.exe`, хранится 30 дней.

---

## Структура проекта

```
tray-app/
├── .github/
│   └── workflows/
│       └── build.yml          # GitHub Actions pipeline
├── .gitlab-ci.yml             # GitLab CI pipeline
├── CMakeLists.txt
├── README.md
├── resources/
│   ├── app.ico                # Иконка приложения
│   └── generate_icon.py       # Скрипт генерации иконки
└── src/
    ├── main.cpp               # Весь код приложения
    ├── resource.h             # ID ресурсов
    └── app.rc                 # Файл ресурсов
```
