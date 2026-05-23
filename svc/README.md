# TrayService

Windows-служба, управляющая запуском `TrayApp.exe` в пользовательских терминальных сессиях. Взаимодействует с графическим приложением через Windows RPC (транспорт ALPC).

---

## Архитектура

```
TrayService.exe (SYSTEM, Session 0)
  │
  ├─ WTSEnumerateSessions → запускает TrayApp.exe в каждой активной сессии
  ├─ WTSRegisterSessionNotification → при новом логине запускает TrayApp.exe
  └─ RPC-сервер (ncalrpc, endpoint: TrayServiceEndpoint)
       └─ StopService() → останавливает все TrayApp + саму службу

TrayApp.exe (пользователь, Session N)
  ├─ При старте: проверяет состояние службы
  │    └─ Если остановлена → StartService() → ждёт Running → выходит
  ├─ Проверяет родительский процесс (должен быть TrayService.exe)
  └─ "Выход" (меню/трей) → RPC-вызов StopService()
```

---

## Сборка

```bat
:: Developer Command Prompt for VS 2022 (x64), из корня репо
python tray-app/resources/generate_icon.py
cmake -B build -G "NMake Makefiles" -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=dist
cmake --build build
cmake --install build
```

Результат: `dist\bin\TrayApp.exe` и `dist\bin\TrayService.exe`.

---

## Установка и запуск службы

```bat
:: Установить службу (от имени администратора)
sc create TrayService binPath= "C:\path\to\TrayService.exe" start= auto

:: Запустить
sc start TrayService

:: Остановить (только через RPC — команда sc stop намеренно игнорируется)
:: Используйте пункт «Выход» в TrayApp
```

---

## Соответствие требованиям

| # | Требование | Реализация |
|---|---|---|
| Служба 1 | Запуск TrayApp во всех сессиях (кроме 0) | `LaunchAppInAllSessions()` через `WTSEnumerateSessions` + `CreateProcessAsUser` |
| Служба 2 | Отслеживание новых логинов | `WTSRegisterSessionNotification` + `WM_WTSSESSION_CHANGE` |
| Служба 3 | Игнорирование Stop/Shutdown | `ServiceCtrlHandler` не обрабатывает `SERVICE_CONTROL_STOP` |
| Служба 4 | RPC-сервер на ALPC | `RpcServerUseProtseqEp("ncalrpc", ...)` |
| Служба 5 | RPC-интерфейс для остановки | `ITrayService` IDL + `StopService()` |
| Служба 6 | Завершение TrayApp при остановке | `TerminateAllApps()` |
| Приложение 1 | Проверка службы при старте | `QueryServiceState()` + `StartAndWaitService()` |
| Приложение 2 | Проверка родителя | `ParentIsService()` через `CreateToolhelp32Snapshot` |
| Приложение 3 | «Выход» в меню → остановка службы | `ExitViaService()` → `RequestServiceStop()` (RPC) |
| Приложение 4 | «Выход» в трее → остановка службы | То же |
