<div align="center">

<img src="docs/logo-256.png" alt="WacOS logo" width="128" height="128">

# WacOS

**macOS on top of Windows 11: menu bar, Dock, Finder, Spotlight, Control Center and Liquid Glass, in native C++.**

[![Stars](https://img.shields.io/github/stars/renzyxx/WacOS?style=flat&logo=github&label=Stars)](https://github.com/renzyxx/WacOS/stargazers)
[![Forks](https://img.shields.io/github/forks/renzyxx/WacOS?style=flat&logo=github&label=Forks)](https://github.com/renzyxx/WacOS/network/members)
[![Downloads](https://img.shields.io/github/downloads/renzyxx/WacOS/total?style=flat&label=Downloads&color=brightgreen)](https://github.com/renzyxx/WacOS/releases)
[![Latest release](https://img.shields.io/github/v/release/renzyxx/WacOS?style=flat&label=Release)](https://github.com/renzyxx/WacOS/releases/latest)
[![Build](https://img.shields.io/github/actions/workflow/status/renzyxx/WacOS/build.yml?branch=main&style=flat&label=Build)](https://github.com/renzyxx/WacOS/actions/workflows/build.yml)
![Windows 11](https://img.shields.io/badge/Windows-11-0078D4?style=flat&logo=windows11&logoColor=white)
![C++](https://img.shields.io/badge/C%2B%2B-native-00599C?style=flat&logo=cplusplus&logoColor=white)

[![Download WacOS](https://img.shields.io/badge/DOWNLOAD-WacOS_for_Windows_11-0A84FF?style=for-the-badge&logo=windows11&logoColor=white)](https://github.com/renzyxx/WacOS/releases/latest)
[![Join the WacOS Discord](https://img.shields.io/badge/DISCORD-Join_us-5865F2?style=for-the-badge&logo=discord&logoColor=white)](https://discord.gg/bDVydgSKPv)
[![Buy me a coffee](https://img.shields.io/badge/BUY_ME_A_COFFEE-renzyx-FFDD00?style=for-the-badge&logo=buymeacoffee&logoColor=black)](https://buymeacoffee.com/renzyx)

</div>

WacOS runs on top of Windows (same kernel, drivers and apps) and replaces what you see and touch: the menu bar,
the Dock, Finder, Spotlight, Control Center, Notification Center, Mission Control, Launchpad, window title bars and a
set of built-in apps. Pick the new macOS 27 **Golden Gate** look, the macOS Tahoe look with Liquid Glass, the
macOS Sequoia look, or the classic **Mac OS X Leopard** (2007) Aqua look.

Everything is native C++ (Win32, Direct2D, DirectComposition). No Electron, no web views. It uses about 750 MB of RAM
and under 1% CPU on a 3440×1440 desktop.

https://github.com/user-attachments/assets/02e58b90-9ccb-4b63-852a-8c0ebd2c3375

**Contents:** [Screenshots](#screenshots) · [Features](#features) · [Siri with AI](#siri-with-ai) · [Install](#install) · [Is it safe?](#is-it-safe) ·
[Roadmap](#roadmap) · [Support the project](#support-the-project) · [Build](#build) · [License](#license)

## Screenshots

Four themes, in System Settings > Appearance > Theme. Switching themes never changes your own wallpaper.

### 1. macOS 27 Golden Gate (new)

The newest look, matched to Apple's macOS 27 screenshots: frosted Liquid Glass on windows, menus and the Dock,
unified toolbars, the macOS 27 icons and the Golden Gate wallpaper.

![WacOS desktop in the macOS 27 Golden Gate theme](docs/screenshots/golden-gate-desktop.png)

| | |
|---|---|
| ![Quick Look, Golden Gate](docs/screenshots/golden-gate-quick-look.png) **Quick Look** plays video and music, with a frosted header and an animated open/close | ![Photos, Golden Gate](docs/screenshots/golden-gate-photos.png) **Photos** with search |
| ![Finder, Golden Gate](docs/screenshots/golden-gate-finder.png) **Finder** | ![System Settings, Golden Gate](docs/screenshots/golden-gate-system-settings.png) **System Settings** > Appearance, for every theme |
| ![Notes, Golden Gate](docs/screenshots/golden-gate-notes.png) **Notes** | ![Calendar, Golden Gate](docs/screenshots/golden-gate-calendar.png) **Calendar** |
| ![Reminders, Golden Gate](docs/screenshots/golden-gate-reminders.png) **Reminders** | ![Music, Golden Gate](docs/screenshots/golden-gate-music.png) **Music** with search |
| ![Clock, Golden Gate](docs/screenshots/golden-gate-clock.png) **Clock** with sunrise and sunset times | ![Weather, Golden Gate](docs/screenshots/golden-gate-weather.png) **Weather** |
| ![Activity Monitor, Golden Gate](docs/screenshots/golden-gate-activity-monitor.png) **Activity Monitor** | ![Preview, Golden Gate](docs/screenshots/golden-gate-preview.png) **Preview** |
| ![Calculator, Golden Gate](docs/screenshots/golden-gate-calculator.png) **Calculator** | ![Terminal, Golden Gate](docs/screenshots/golden-gate-terminal.png) **Terminal** |
| ![TextEdit, Golden Gate](docs/screenshots/golden-gate-textedit.png) **TextEdit** | |

Also new with this theme: QuickTime Player with a frosted title bar over the video, the option to hide desktop icons,
**Edit Widgets…** on the desktop menu, clicking the empty strip beside the Dock to show the desktop, Win+D handled by
the shell, grouped notifications that open on click, and smoother glass over animated wallpapers.

### Liquid Glass in motion

Real glass over whatever is behind it, live: refraction, chromatic aberration and the frost of each theme.

![A window passing behind the Dock](docs/screenshots/liquid-glass-dock.gif)

| | |
|---|---|
| ![The app switcher over moving content](docs/screenshots/liquid-glass-alt-tab.gif) **App switcher** (Alt+Tab) | ![Control Center over moving content](docs/screenshots/liquid-glass-control-center.gif) **Control Center** |

### 2. macOS Tahoe (Liquid Glass)

The macOS 26 look: clear Liquid Glass with real refraction and chromatic aberration.

![WacOS desktop in the Tahoe theme](docs/screenshots/desktop.png)

| | |
|---|---|
| ![Control Center in Liquid Glass](docs/screenshots/control-center.png) **Control Center** in Liquid Glass, with real refraction and chromatic aberration | ![Notification Center](docs/screenshots/notification-center.png) **Notification Center** with widgets |
| ![Spotlight](docs/screenshots/spotlight.png) **Spotlight** | ![Spotlight Applications mode](docs/screenshots/spotlight-apps.png) **Applications** (Spotlight's Apps mode, with categories) |
| ![Finder](docs/screenshots/finder.png) **Finder** | ![System Settings](docs/screenshots/system-settings.png) **System Settings** |
| ![Activity Monitor](docs/screenshots/activity-monitor.png) **Activity Monitor** | ![Force Quit Applications](docs/screenshots/force-quit.png) **Force Quit Applications** (Ctrl+Alt+Esc) |
| ![macOS alert](docs/screenshots/alert.png) Windows message boxes become **macOS alerts** | ![Dock icon styles](docs/screenshots/dock-icon-styles.png) **Dock icon styles**: Default, Dark, Clear and Tinted |

### 3. macOS Sequoia

The same shell with the macOS 15 look (System Settings > Appearance > Theme).

![WacOS desktop in the Sequoia theme](docs/screenshots/sequoia-desktop.png)

| | |
|---|---|
| ![Control Center, Sequoia](docs/screenshots/sequoia-control-center.png) **Control Center** | ![Notification Center, Sequoia](docs/screenshots/sequoia-notification-center.png) **Notification Center** |
| ![Launchpad](docs/screenshots/sequoia-launchpad.png) **Launchpad** | ![Spotlight, Sequoia](docs/screenshots/sequoia-spotlight.png) **Spotlight** |
| ![Finder, Sequoia](docs/screenshots/sequoia-finder.png) **Finder** | ![System Settings, Sequoia](docs/screenshots/sequoia-system-settings.png) **System Settings** with the Sequoia wallpapers |

### 4. Mac OS X Leopard (new)

The 2007 Aqua look, drawn from scratch (nothing reused from the modern themes): the glossy 3D Dock shelf with
reflections, the translucent menu bar, gel buttons, brushed-metal windows, Cover Flow in Finder, System Preferences
laid out like 10.5, Quick Look as a dark HUD, Dashboard, Spaces, Time Machine and the Leopard cursor (your previous cursor comes back when you switch
themes).

![Dock, Leopard](docs/screenshots/leopard-dock.png)

| | |
|---|---|
| ![Finder with Cover Flow, Leopard](docs/screenshots/leopard-finder.png) **Finder** with Cover Flow | ![System Preferences, Leopard](docs/screenshots/leopard-system-preferences.png) **System Preferences** |
| ![Quick Look, Leopard](docs/screenshots/leopard-quick-look.png) **Quick Look** HUD | ![About This Mac, Leopard](docs/screenshots/leopard-about.png) **About This Mac** with your PC's real specs |

## Features

- **Menu bar** with the active app's real menus, the Apple menu, status icons and the system tray. Hidden tray icons
  open from a list that shows each app's icon and name. New: you can move the menu bar to the **bottom, beside the
  Dock** (System Settings > Control Center).
- **Dock** with magnification, running indicators, stacks (Downloads), minimized windows and the Trash. Put it on
  the **bottom, left or right**, hide it always or only when a window touches it (**smart hide**), and customize
  spacing, distance from the edge, corner rounding, glass color and opacity, labels, indicators and the divider.
- **Change Icon…**: right-click an app in the Dock or Launchpad to use your own PNG/ICO (and restore the original).
- **Genie** minimize animation drawn by the compositor, with smooth edges all the way into the Dock.
- **Finder** (file manager), **Spotlight**, **Control Center**, **Notification Center**, **Mission Control**,
  **Launchpad / Apps**, **Quick Look**, **Stage Manager** and **Hot Corners**.
- **Spotlight** searches apps, files, folders and the web, does math, and completes the best match as you type, with
  Applications, Files, Actions and Clipboard modes. New: it also finds **Windows settings and tools**
  ("device manager", "add or remove programs", services, display, Wi-Fi and about 200 Control Panel tasks), each
  with its own icon.
- **Mac title bars** (traffic lights, rounded corners, shadows) on other apps' windows, including GTK apps like
  GIMP 3. Chrome, Discord, Spotify and other Chromium/Electron apps keep their own bar but get rounded corners and a
  Mac-style border that matches the app's own light or dark look.
- **Mac buttons in Chrome, Discord and other Electron apps** (new, **experimental**, off by default): the traffic
  lights go into the app's own title bar, in place of the Windows buttons. Turn it on in System Settings > Desktop &
  Dock > "Mac buttons in apps with their own title bar". Feedback is welcome in the issues.
- **macOS alerts** in place of Windows message boxes, and the Mac Open/Save panel through Finder.
- **Built-in apps**: System Settings, Activity Monitor (with memory pressure), Terminal, Calculator, Notes,
  Reminders, Calendar, TextEdit, Preview, Photos, Music, QuickTime Player, Clock, Weather, Contacts,
  Time Machine, Screenshot and Force Quit.
- **Siri with AI** (new): ask by typing or by voice, from Spotlight, the menu bar or Win+S. Uses the AI service you
  choose with your own key, a model running on your PC, or an AI app you already have. See [Siri with AI](#siri-with-ai).
- **Memory management like macOS**: memory pressure, App Nap and purgeable memory.
- **Themes**: macOS 27 Golden Gate, macOS Tahoe, macOS Sequoia and Mac OS X Leopard.
- **Dynamic Island** (optional, System Settings > Control Center): music, timers, notifications and volume in a pill
  or a **notch** at the top of the screen, plus a **clipboard history** with your last 20 copies (text and files),
  kept across restarts and encrypted for your Windows user.
- **Game Mode** like macOS: when a game comes to the front, WacOS turns off what costs the most (real glass, mouse
  effects, Hot Corners, file indexing) until the game quits. Game launchers (Riot Client, EA app, Ubisoft Connect,
  Battle.net and others) don't count as games. System Settings > Energy.
- **Launchpad**: hide any app (right-click > Hide from Launchpad), or all of Windows' system tools at once.
- **Icon styles** (Default, Dark, Clear, Tinted), accent colors, light and dark appearance.
- **Customize the desktop** (icon size, grid spacing, text size, widgets), **notifications** (banner duration,
  Notification Center widgets) and the **menu bar** (show it on every display or only the main one).
- **Calendar** with your country's holidays and Monday/Sunday start, **Weather** with any city as your location,
  **Clock** with custom cities and alarms.
- **12 languages**: English (default), Portuguese, Spanish, French, German, Italian, Japanese, Chinese, Korean,
  Russian, Polish and Turkish (System Settings > General > Language). Dates follow the language too.

## Siri with AI

New in WacOS: **Siri** that actually answers, powered by the AI you choose. The answer appears in a glass panel at
the top of the screen, word by word, with the Apple Intelligence glow running around it while it thinks.

<p align="center"><img src="docs/screenshots/siri-demo.gif" alt="Siri answering a question on WacOS" width="600"></p>

- **Ask from anywhere:** press **Win+S**, click the Siri icon in the menu bar, or type a question in Spotlight and pick
  **Ask Siri**. Start typing after an answer to ask the next one.
- **Talk to it:** click the microphone and speak. Dictation uses Windows' own speech recognition, in the WacOS language.
- **Your AI, your key:** Google Gemini, OpenAI, Anthropic Claude, Groq, OpenRouter, Mistral, DeepSeek or xAI Grok with
  your own API key. The model list comes from the service, so new models show up with **Refresh**.
- **Fully local, if you want:** Ollama or LM Studio running a model on your PC, with nothing leaving it.
- **No key at all:** use the Codex or Claude Code app you're already signed in to on your PC.
- **Search the web** with Gemini, for questions about what's happening now.
- **Writing Tools in Notes** use the same AI to proofread, rewrite or summarize your text, pull out key points, or
  turn it into a list or a table.

**Privacy:** WacOS has no server. Your question goes straight from your PC to the service you picked (or stays on
your PC with a local model). Your key is encrypted with Windows (DPAPI) for your user only, and it's never written to
the settings file or the log.

Turn it on in **System Settings > Apple Intelligence & Siri**: pick a service, paste your key and choose a model.
Gemini and Groq have free plans that are enough to get started.

## Install

1. Download the `WacOS-<date>.zip` from the [latest release](https://github.com/renzyxx/WacOS/releases/latest).
2. Extract it anywhere and open **`WacOSSetup.exe`**: one app that installs, updates, changes and uninstalls WacOS,
   in the style of the macOS Installer.

The release zip includes these fonts, sounds and cursors (they aren't in the repository, and `WacOSSetup.exe`
doesn't download them). They belong to their owners and aren't covered by the WacOS license (see [License](#license)):

- **SF Pro and SF Mono**, Apple's fonts (`fontes` folder; [Apple's license](https://developer.apple.com/fonts/) applies).
  Installing them is optional.
- **macOS alert and interface sounds**, Apple's (`sons` folder). They are copied along with WacOS.
- **macOS Tahoe cursor** by [jimmyxd2 on DeviantArt](https://www.deviantart.com/jimmyxd2/art/1208233550) (`cursor` folder).
  Optional, and only offered with the full WacOS.
- **Mac OS X cursor**, from a pack by RaZcaL inSIDe that credits Apple as the probable original author
  (`cursor-leopard` folder). It is copied along with WacOS and used by the Leopard theme.

If the fonts, alert sounds or Tahoe cursor are missing (for example, in a package you built yourself), the old
`instalar.cmd` can offer to get them: the fonts from Apple (unpacked with 7-Zip, installed through `winget` if needed), 14 alert
sounds from [extratone/macOSsystemsounds](https://github.com/extratone/macOSsystemsounds), and the cursor from its
DeviantArt page, which you download yourself.

WacOS installs for your user only (no administrator rights) in `%LOCALAPPDATA%\Programs\WacOS`.
Everything it changes is logged in `instalacao.log` there. To uninstall, use **Settings > Apps > Installed apps >
WacOS > Uninstall** (or open `WacOSSetup.exe` again). The old `instalar.cmd` / `desinstalar.cmd` scripts are still in
the package for advanced use.

- Quit WacOS: `Ctrl+Alt+Shift+F12`. Emergency exit (brings Explorer back): `Ctrl+Alt+Shift+F11`.

### Use only the apps, or only the title bars

Have your own setup? `WacOSSetup.exe` asks what to install:

1. **Full WacOS** (menu bar, Dock, Finder...).
2. **Only the apps you pick** (Finder, Notes, Calculator, Music, Preview...): shortcuts go to a **WacOS** folder in
   the Start menu. Each app draws its own Mac title bar, with or without the shell running.
3. **Only the Mac title bars** on your other apps: `WacOSTitleBars.exe`, with an optional start at sign-in.
   Turn it off with `WacOSTitleBars.exe --sair` (it also stops when the full shell starts, which takes over).
4. **Apps + title bars**.

Options 2 to 4 keep the Windows taskbar, Explorer and your folders as they are, and don't add the shell's sign-in
task. Themes and language are read from System Settings (install it as one of the apps to change them).
Apps that should never get the Mac title bar go one per line in `%APPDATA%\Shell\janelas-excluidas.txt`.
The built-in apps keep the window corners Windows draws, so mods that change the corner radius (like Windhawk's
rounded corners) should no longer leave a square frame around them.

## Updates

New versions are published as numbered releases (`build-N`). In **System Settings > General > Software Update**,
WacOS checks GitHub on its own, shows the installed version, the latest one and the commits you don't have yet, and
**Update Now** opens `WacOSSetup.exe`, which downloads the new package, checks its SHA-256 against the one GitHub
publishes and installs it (only programs, icons and sounds are replaced; your settings stay).
GitHub Actions checks that every commit to `main` builds.

## Is it safe?

Fair question for anything that replaces your taskbar. You don't have to trust the author:

- **Source available.** Every file is in this repository: read it, and build it yourself (see [Build](#build)).
- **Network.** No telemetry, no accounts. These are the hosts WacOS's own code contacts:
  - `api.open-meteo.com`: the forecast. Receives the coordinates of your chosen cities (until you pick one, a city
    guessed from your Windows time zone). The shell asks at startup and every 15 minutes, even with the widget
    hidden (not in "Dock and menu bar only" mode); the Weather app does the same while it's open.
  - `geocoding-api.open-meteo.com`: city search in Weather and Clock. Receives the name you search for and your
    language.
  - `date.nager.at`: public holidays in Calendar (on by default; untick that calendar to stop it). Receives the year
    and the country code from your Windows region.
  - `suggestqueries.google.com`: Spotlight suggestions. Receives what you type in Spotlight and your language, as
    you type. On by default; `spotlight_google=0` in `%APPDATA%\Shell\ajustes.txt` turns it off.
  - `api.github.com`, with `github.com` as the fallback: updates. The shell checks 90 seconds after it starts and every
    6 hours (turning off the update notification doesn't stop the check), and so do the Software Update page and
    `WacOSSetup.exe` when you update. The check also sends the installed commit's ID, to list what changed.
  - `github.com` and `release-assets.githubusercontent.com`: the package download, when you choose to update.
  - Only in the old `instalar.cmd`, for files missing from the package and only if you say yes:
    `devimages-cdn.apple.com` (fonts), `raw.githubusercontent.com` (sounds) and `winget`, which uses its own hosts
    (7-Zip, to unpack the fonts).

  Links, web searches and the DeviantArt cursor page open in your browser. Run WacOS behind a firewall like
  simplewall or GlassWire and watch for yourself (the update check shows up as `powershell.exe`).
- **What it installs.** One scheduled task to start at sign-in, with normal user rights. No services, no drivers,
  no administrator rights. Uninstalling (Installed apps > WacOS) removes everything.
- **Games are left alone.** The title bar DLL never loads into games, launchers or anything with anti-cheat: it
  checks the program's folder, engine and anti-cheat files next to it, engine window classes and exclusive
  fullscreen, and even if loaded it refuses to start inside a game. Add your own exceptions in
  `%APPDATA%\Shell\janelas-excluidas.txt`.
- **Antivirus.** A shell has to do things that look suspicious to heuristics: start at sign-in, use keyboard and
  mouse hooks for shortcuts like the Win key and hot corners, and load its title bar DLL into other apps. The
  build-12 package shows [1/66 on VirusTotal](https://www.virustotal.com/gui/file/310ab4f16824500a8d77225fef66a62e499c32d6d5484a8752b119b7f24e67f3),
  a single AI-based heuristic. The install scripts are plain PowerShell you can read: no inline C#, no hidden
  windows, and the updater checks the package's SHA-256 against the one GitHub publishes. If Microsoft Defender flags it on your PC, it's a false positive that has been
  reported to Microsoft; please [open an issue](https://github.com/renzyxx/WacOS/issues) with the threat name.

## Testing and stability

WacOS is tested on the author's PC after every change, and the [change log](docs/registro-mudancas.txt) (Portuguese)
records what was changed, how it was checked and what wasn't. Some of the checks:

- **Stress test** (15 to 30 minutes): test programs open, minimize and close windows non-stop while Mission Control
  and Launchpad open and close, and the shell's memory, handles and GDI/USER objects are sampled. They go up and down
  with Mission Control instead of growing; the test apps with the title bar DLL stay flat after 60+ cycles.
- **Watchdog:** the shell restarts itself if it really freezes. A false alarm under heavy animation (it restarted a
  working shell) was found in the stress test and fixed: zero restarts in the following runs.
- **Animations frame by frame:** the genie and other compositor animations are recorded with Desktop Duplication
  (a normal screenshot can't see them) and checked frame by frame. That's how the "pixelated" genie was traced to
  stair-stepped slices and fixed with trapezoid slices.
- **Title bars across updates:** the title bars load through a small fixed loader (`WacOSGancho.dll`), so updating or
  restarting WacOS doesn't use up Windows' limited hook slots anymore; tested by swapping the title bar DLL three
  times in one sign-in.
- **Freeze hunting:** a sentinel thread writes the stack of any interface stall of 1 second or more to `shell.log`,
  and the shell and the title bar DLL never wait for the disk, the Registry or another process on a window's
  thread. On a busy disk this took freezes of several seconds in the shell and in apps like Discord and Chrome
  down to a few milliseconds.
- **Reported bugs** from Reddit, GitHub and Discord are reproduced when possible (videos help a lot!).

Known limits: apps running as administrator can't get the Mac title bar from a shell that isn't elevated, and the
first install of build-18 or later needs one sign-out so the new title bar loader takes over.

## Roadmap

- **Dynamic Island**: more activities (inspired by [Atoll](https://github.com/Ebullioscopic/Atoll); no code copied).
- **Signed releases**, so antivirus heuristics stop flagging the installer scripts.
- Your idea? [Open an issue](https://github.com/renzyxx/WacOS/issues).

## Support the project

WacOS is free and made in my spare time. If you like it, a coffee helps a lot and keeps the updates coming ☕

<a href="https://buymeacoffee.com/renzyx"><img src="https://cdn.buymeacoffee.com/buttons/v2/default-yellow.png" alt="Buy Me a Coffee" height="50"></a>

Starring the repository and sharing it also helps.

## Build

Requires Visual Studio 2022 (or Build Tools 2022) with the C++ workload.

```
build.cmd            everything
build.cmd shell      the shell only (WacOSShell.exe: menu bar, Dock, Spotlight...)
build.cmd finder     Finder only
build.cmd janelas    Mac title bars for other apps (WacOSWindow.dll and its fixed loader WacOSGancho.dll,
                     32 and 64 bit, and WacOSTitleBars.exe)
build.cmd titulos    WacOSTitleBars.exe only (the title bars without the shell)
tools\empacotar.ps1  the installer package (dist\WacOS-<date>.zip)
```

## Layout

- `src/shell`: the shell (menu bar, Dock, Spotlight, Control Center, Mission Control, Liquid Glass...).
- `src/finder`, `src/ajustes` (System Settings), `src/monitor` (Activity Monitor) and the other apps in `src/*`.
- `src/janelas`: Mac title bars injected into other apps' windows (`gancho.cpp` is the fixed loader Windows hooks
  in; it loads the current `macjanela.cpp` DLL); `src/titulos`: the title bars without the shell.
- `src/menu`: context menus on top of the real Windows shell extensions.
- `src/comum`: shared code (names, language, glass, shadows).
- `instalador`: install, update and uninstall scripts. `tools`: build and packaging tools.
- `icones`: icon packs (Sequoia, Tahoe) and SF Symbols.

## Performance

Measured on the author's PC (i9-14900K, RTX 3070 Ti, 3440×1440 at 174 Hz). At rest the shell uses about 1% of one
CPU core and no GPU time; with a game in front (Game Mode on) about 2% of one core and no GPU time. Memory: about
350 to 500 MB for the shell (most of it is the Spotlight file index), plus around 100 MB for the hidden Finder that
makes Finder windows open instantly. Animations run on the display's refresh clock, not timers, and nothing polls while
idle. Minimizing a window takes about 55 ms of preparation (window snapshot, detail levels and ~400 animated slices)
before the genie starts; the rest runs on the GPU in the Windows compositor.

On laptops with two GPUs, set `WacOSShell.exe` to **High performance** in Windows Settings > System > Display >
Graphics. If animations still stutter, System Settings > Appearance > Transparency & Effects > **Reduce transparency**
turns off the Liquid Glass blur, the heaviest effect.

## License

**WacOS is source-available, not open source.** Copyright (c) 2026 renzyxx, all rights reserved.
See [LICENSE](LICENSE) for the full terms.

- ✅ You can read the code, build it and run WacOS on your own PCs for personal use, and send pull requests.
- ❌ You can't copy the code (or parts of it) into another project, redistribute WacOS (re-uploads, mirrors,
  repacks, modded builds) or use it commercially without written permission.

Want to use something from WacOS in your own project, or port it? Open an issue and ask first.

Apple's icons, fonts, sounds and wallpapers, and the third-party icon and cursor packs bundled in the release package,
are not covered by the license: they belong to their owners and are included for personal use only. One Windhawk mod
in `docs/windhawk/bloqueio-sequoia/` is derived from GPL-3.0 code and keeps that license.

## Note

Apple, macOS, Finder and related names and icons belong to Apple Inc. This is a personal, non-commercial project
and is not affiliated with Apple. The bundled icons, fonts and sounds are for personal use only.

<details>
<summary><b>PT-BR version</b></summary>
  
---

# WacOS (Português Brasileiro)

<div align="center">

**macOS sobre o Windows 11: barra de menus, Dock, Finder, Spotlight, Central de Controle e Liquid Glass, em C++ nativo.**

</div>

O WacOS funciona sobre o Windows (com o mesmo kernel, drivers e aplicativos) e substitui os elementos com os quais você interage: barra de menus, Dock, Finder, Spotlight, Central de Controle, Central de Notificações, Mission Control, Launchpad, barras de título das janelas e um conjunto de aplicativos integrados. Escolha o novo visual do macOS 27 **Golden Gate**, o visual do macOS Tahoe com Liquid Glass, o visual do macOS Sequoia ou o clássico visual Aqua do **Mac OS X Leopard** (2007).

Tudo é desenvolvido em C++ nativo (Win32, Direct2D, DirectComposition). Sem Electron e sem visualizações web. O projeto utiliza aproximadamente 750 MB de RAM e menos de 1% de CPU em um desktop com resolução de 3440×1440.

https://github.com/user-attachments/assets/02e58b90-9ccb-4b63-852a-8c0ebd2c3375

**Conteúdo:** [Capturas de tela](#capturas-de-tela) · [Recursos](#recursos) · [Siri com IA](#siri-com-ia) · [Instalação](#instalação) · [É seguro?](#é-seguro) · [Testes e estabilidade](#testes-e-estabilidade) · [Roadmap](#roadmap) · [Apoie o projeto](#apoie-o-projeto) · [Compilação](#compilação) · [Estrutura](#estrutura) · [Desempenho](#desempenho) · [Licença](#licença) · [Observação](#observação)

## Capturas de tela

Quatro temas disponíveis em Ajustes do Sistema > Aparência > Tema. Alternar entre os temas nunca altera o papel de parede escolhido por você.

### 1. macOS 27 Golden Gate (novo)

O visual mais recente, baseado nas capturas de tela do macOS 27 da Apple: Liquid Glass fosco nas janelas, nos menus e no Dock, barras de ferramentas unificadas, ícones do macOS 27 e o papel de parede Golden Gate.

![Área de trabalho do WacOS no tema macOS 27 Golden Gate](docs/screenshots/golden-gate-desktop.png)

| | |
|---|---|
| ![Quick Look, Golden Gate](docs/screenshots/golden-gate-quick-look.png) **Quick Look** reproduz vídeos e músicas, com cabeçalho fosco e animação de abertura e fechamento | ![Fotos, Golden Gate](docs/screenshots/golden-gate-photos.png) **Fotos** com pesquisa |
| ![Finder, Golden Gate](docs/screenshots/golden-gate-finder.png) **Finder** | ![Ajustes do Sistema, Golden Gate](docs/screenshots/golden-gate-system-settings.png) **Ajustes do Sistema** > Aparência, para todos os temas |
| ![Notas, Golden Gate](docs/screenshots/golden-gate-notes.png) **Notas** | ![Calendário, Golden Gate](docs/screenshots/golden-gate-calendar.png) **Calendário** |
| ![Lembretes, Golden Gate](docs/screenshots/golden-gate-reminders.png) **Lembretes** | ![Música, Golden Gate](docs/screenshots/golden-gate-music.png) **Música** com pesquisa |
| ![Relógio, Golden Gate](docs/screenshots/golden-gate-clock.png) **Relógio** com horários do nascer e do pôr do sol | ![Clima, Golden Gate](docs/screenshots/golden-gate-weather.png) **Clima** |
| ![Monitor de Atividade, Golden Gate](docs/screenshots/golden-gate-activity-monitor.png) **Monitor de Atividade** | ![Pré-Visualização, Golden Gate](docs/screenshots/golden-gate-preview.png) **Pré-Visualização** |
| ![Calculadora, Golden Gate](docs/screenshots/golden-gate-calculator.png) **Calculadora** | ![Terminal, Golden Gate](docs/screenshots/golden-gate-terminal.png) **Terminal** |
| ![TextEdit, Golden Gate](docs/screenshots/golden-gate-textedit.png) **TextEdit** | |

Também são novidades deste tema: QuickTime Player com barra de título fosca sobre o vídeo, opção para ocultar os ícones da área de trabalho, **Editar Widgets…** no menu da área de trabalho, clique na faixa vazia ao lado do Dock para mostrar a área de trabalho, Win+D controlado pelo shell, notificações agrupadas que abrem ao serem clicadas e efeito de vidro mais suave sobre papéis de parede animados.

### Liquid Glass em movimento

Vidro real sobre o que estiver atrás dele, em tempo real: refração, aberração cromática e o efeito fosco de cada tema.

![Uma janela passando atrás do Dock](docs/screenshots/liquid-glass-dock.gif)

| | |
|---|---|
| ![Seletor de aplicativos sobre conteúdo em movimento](docs/screenshots/liquid-glass-alt-tab.gif) **Seletor de aplicativos** (Alt+Tab) | ![Central de Controle sobre conteúdo em movimento](docs/screenshots/liquid-glass-control-center.gif) **Central de Controle** |

### 2. macOS Tahoe (Liquid Glass)

O visual do macOS 26: Liquid Glass transparente, com refração real e aberração cromática.

![Área de trabalho do WacOS no tema Tahoe](docs/screenshots/desktop.png)

| | |
|---|---|
| ![Central de Controle em Liquid Glass](docs/screenshots/control-center.png) **Central de Controle** em Liquid Glass, com refração real e aberração cromática | ![Central de Notificações](docs/screenshots/notification-center.png) **Central de Notificações** com widgets |
| ![Spotlight](docs/screenshots/spotlight.png) **Spotlight** | ![Modo Aplicativos do Spotlight](docs/screenshots/spotlight-apps.png) **Aplicativos** (modo Aplicativos do Spotlight, com categorias) |
| ![Finder](docs/screenshots/finder.png) **Finder** | ![Ajustes do Sistema](docs/screenshots/system-settings.png) **Ajustes do Sistema** |
| ![Monitor de Atividade](docs/screenshots/activity-monitor.png) **Monitor de Atividade** | ![Forçar Encerramento de Aplicativos](docs/screenshots/force-quit.png) **Forçar Encerramento de Aplicativos** (Ctrl+Alt+Esc) |
| ![Alerta do macOS](docs/screenshots/alert.png) As caixas de mensagem do Windows são substituídas por **alertas do macOS** | ![Estilos de ícones do Dock](docs/screenshots/dock-icon-styles.png) **Estilos de ícones do Dock**: Padrão, Escuro, Transparente e Matizado |

### 3. macOS Sequoia

O mesmo shell com o visual do macOS 15 (Ajustes do Sistema > Aparência > Tema).

![Área de trabalho do WacOS no tema Sequoia](docs/screenshots/sequoia-desktop.png)

| | |
|---|---|
| ![Central de Controle, Sequoia](docs/screenshots/sequoia-control-center.png) **Central de Controle** | ![Central de Notificações, Sequoia](docs/screenshots/sequoia-notification-center.png) **Central de Notificações** |
| ![Launchpad](docs/screenshots/sequoia-launchpad.png) **Launchpad** | ![Spotlight, Sequoia](docs/screenshots/sequoia-spotlight.png) **Spotlight** |
| ![Finder, Sequoia](docs/screenshots/sequoia-finder.png) **Finder** | ![Ajustes do Sistema, Sequoia](docs/screenshots/sequoia-system-settings.png) **Ajustes do Sistema** com os papéis de parede do Sequoia |

### 4. Mac OS X Leopard (novo)

O visual Aqua de 2007, desenhado do zero (sem reaproveitar elementos dos temas modernos): base 3D brilhante do Dock com reflexos, barra de menus translúcida, botões em gel, janelas de metal escovado, Cover Flow no Finder, Preferências do Sistema organizadas como no 10.5, Quick Look como um HUD escuro, Dashboard, Spaces, Time Machine e o cursor do Leopard (o cursor anterior é restaurado ao trocar de tema).

![Dock, Leopard](docs/screenshots/leopard-dock.png)

| | |
|---|---|
| ![Finder com Cover Flow, Leopard](docs/screenshots/leopard-finder.png) **Finder** com Cover Flow | ![Preferências do Sistema, Leopard](docs/screenshots/leopard-system-preferences.png) **Preferências do Sistema** |
| ![Quick Look, Leopard](docs/screenshots/leopard-quick-look.png) HUD do **Quick Look** | ![Sobre Este Mac, Leopard](docs/screenshots/leopard-about.png) **Sobre Este Mac** com as especificações reais do seu PC |

## Recursos

- **Barra de menus** com os menus reais do aplicativo ativo, menu Apple, ícones de status e bandeja do sistema. Os ícones ocultos da bandeja podem ser acessados por uma lista que mostra o ícone e o nome de cada aplicativo. Novidade: é possível mover a barra de menus para a **parte inferior, ao lado do Dock** (Ajustes do Sistema > Central de Controle).
- **Dock** com ampliação de ícones, indicadores de aplicativos em execução, pilhas (Downloads), janelas minimizadas e Lixeira. Posicione-o na **parte inferior, à esquerda ou à direita**, mantenha-o sempre oculto ou oculte-o apenas quando uma janela se aproximar (**ocultação inteligente**). Personalize espaçamento, distância da borda, arredondamento dos cantos, cor e opacidade do vidro, rótulos, indicadores e divisor.
- **Alterar Ícone…**: clique com o botão direito em um aplicativo no Dock ou no Launchpad para usar seu próprio arquivo PNG/ICO e restaurar o ícone original quando quiser.
- **Animação Genie** de minimização, desenhada pelo compositor, com bordas suaves durante todo o percurso até o Dock.
- **Finder** (gerenciador de arquivos), **Spotlight**, **Central de Controle**, **Central de Notificações**, **Mission Control**, **Launchpad / Aplicativos**, **Quick Look**, **Stage Manager** e **Cantos Ativos**.
- **Spotlight** pesquisa aplicativos, arquivos, pastas e a web, realiza cálculos e completa a melhor correspondência enquanto você digita, com modos de Aplicativos, Arquivos, Ações e Área de Transferência. Novidade: também encontra **configurações e ferramentas do Windows** ("gerenciador de dispositivos", "adicionar ou remover programas", serviços, tela, Wi-Fi e cerca de 200 tarefas do Painel de Controle), cada uma com seu próprio ícone.
- **Barras de título estilo Mac** (botões de fechar, minimizar e maximizar, cantos arredondados e sombras) nas janelas de outros aplicativos, inclusive aplicativos GTK como GIMP 3. Chrome, Discord, Spotify e outros aplicativos Chromium/Electron mantêm sua própria barra, mas recebem cantos arredondados e uma borda no estilo Mac que acompanha a aparência clara ou escura do próprio aplicativo.
- **Botões Mac no Chrome, Discord e outros aplicativos Electron** (novo, **experimental**, desativado por padrão): os botões coloridos passam para a barra de título do próprio aplicativo, substituindo os botões do Windows. Ative em Ajustes do Sistema > Mesa e Dock > "Botões Mac em aplicativos com barra de título própria". Comentários são bem-vindos nas issues.
- **Alertas do macOS** no lugar das caixas de mensagem do Windows e painel Mac de Abrir/Salvar por meio do Finder.
- **Aplicativos integrados**: Ajustes do Sistema, Monitor de Atividade (com pressão de memória), Terminal, Calculadora, Notas, Lembretes, Calendário, TextEdit, Pré-Visualização, Fotos, Música, QuickTime Player, Relógio, Clima, Contatos, Time Machine, Captura de Tela e Forçar Encerramento.
- **Siri com IA** (novo): faça perguntas digitando ou por voz, pelo Spotlight, pela barra de menus ou com Win+S. Utiliza o serviço de IA escolhido por você com sua própria chave, um modelo em execução no PC ou um aplicativo de IA que já esteja instalado. Consulte [Siri com IA](#siri-com-ia).
- **Gerenciamento de memória como no macOS**: pressão de memória, App Nap e memória liberável.
- **Temas**: macOS 27 Golden Gate, macOS Tahoe, macOS Sequoia e Mac OS X Leopard.
- **Dynamic Island** (opcional, Ajustes do Sistema > Central de Controle): música, temporizadores, notificações e volume em uma cápsula ou em um **recorte** na parte superior da tela, além de um **histórico da área de transferência** com as últimas 20 cópias (textos e arquivos), preservadas entre reinicializações e criptografadas para o usuário do Windows.
- **Modo de Jogo** como no macOS: quando um jogo passa para primeiro plano, o WacOS desativa os recursos de maior custo (vidro real, efeitos do mouse, Cantos Ativos e indexação de arquivos) até o jogo ser fechado. Inicializadores de jogos (Riot Client, EA app, Ubisoft Connect, Battle.net e outros) não são considerados jogos. Ajustes do Sistema > Energia.
- **Launchpad**: oculte qualquer aplicativo (clique com o botão direito > Ocultar do Launchpad) ou todas as ferramentas de sistema do Windows de uma só vez.
- **Estilos de ícones** (Padrão, Escuro, Transparente e Matizado), cores de destaque e aparência clara e escura.
- **Personalização da área de trabalho** (tamanho dos ícones, espaçamento da grade, tamanho do texto e widgets), **notificações** (duração dos banners e widgets da Central de Notificações) e **barra de menus** (exibição em todos os monitores ou somente no principal).
- **Calendário** com feriados do seu país e início da semana na segunda-feira ou no domingo, **Clima** com qualquer cidade como localização e **Relógio** com cidades personalizadas e alarmes.
- **12 idiomas**: inglês (padrão), português, espanhol, francês, alemão, italiano, japonês, chinês, coreano, russo, polonês e turco (Ajustes do Sistema > Geral > Idioma). As datas também acompanham o idioma selecionado.

## Siri com IA

Novidade no WacOS: uma **Siri** que realmente responde, usando a inteligência artificial escolhida por você. A resposta aparece em um painel de vidro na parte superior da tela, palavra por palavra, enquanto o brilho do Apple Intelligence percorre sua borda durante o processamento.

<p align="center"><img src="docs/screenshots/siri-demo.gif" alt="Siri respondendo a uma pergunta no WacOS" width="600"></p>

- **Pergunte de qualquer lugar:** pressione **Win+S**, clique no ícone da Siri na barra de menus ou digite uma pergunta no Spotlight e selecione **Perguntar à Siri**. Comece a digitar depois de uma resposta para fazer a próxima pergunta.
- **Fale com ela:** clique no microfone e fale. A transcrição utiliza o reconhecimento de fala do próprio Windows, no idioma configurado no WacOS.
- **Sua IA, sua chave:** Google Gemini, OpenAI, Anthropic Claude, Groq, OpenRouter, Mistral, DeepSeek ou xAI Grok com sua própria chave de API. A lista de modelos vem do serviço, então novos modelos aparecem ao selecionar **Atualizar**.
- **Totalmente local, se preferir:** Ollama ou LM Studio executando um modelo no seu PC, sem que os dados saiam dele.
- **Sem precisar de chave:** utilize o aplicativo Codex ou Claude Code no qual você já esteja conectado no PC.
- **Pesquise na web** com o Gemini para perguntas sobre acontecimentos atuais.
- **Ferramentas de Escrita no Notas** utilizam a mesma IA para revisar, reescrever ou resumir textos, destacar pontos importantes ou transformá-los em uma lista ou tabela.

**Privacidade:** o WacOS não possui servidor próprio. Sua pergunta vai diretamente do PC para o serviço escolhido (ou permanece no computador quando você utiliza um modelo local). Sua chave é criptografada pelo Windows (DPAPI) exclusivamente para o seu usuário e nunca é gravada no arquivo de configurações ou no log.

Ative em **Ajustes do Sistema > Apple Intelligence e Siri**: escolha um serviço, insira sua chave e selecione um modelo. Gemini e Groq possuem planos gratuitos suficientes para começar.

## Instalação

1. Baixe o arquivo `WacOS-<date>.zip` na página da [versão mais recente](https://github.com/renzyxx/WacOS/releases/latest).
2. Extraia o arquivo em qualquer pasta e abra **`WacOSSetup.exe`**: um único aplicativo para instalar, atualizar, modificar e desinstalar o WacOS, seguindo o estilo do instalador do macOS.

O arquivo ZIP da versão inclui estas fontes, sons e cursores (eles não estão no repositório, e o `WacOSSetup.exe` não os baixa). Esses recursos pertencem aos seus respectivos proprietários e não estão cobertos pela licença do WacOS (consulte [Licença](#licença)):

- **SF Pro e SF Mono**, fontes da Apple (pasta `fontes`; aplica-se a [licença da Apple](https://developer.apple.com/fonts/)). A instalação é opcional.
- **Sons de alertas e da interface do macOS**, da Apple (pasta `sons`). Eles são copiados junto com o WacOS.
- **Cursor do macOS Tahoe**, criado por [jimmyxd2 no DeviantArt](https://www.deviantart.com/jimmyxd2/art/1208233550) (pasta `cursor`). Opcional e oferecido somente com a versão completa do WacOS.
- **Cursor do Mac OS X**, de um pacote de RaZcaL inSIDe que atribui à Apple a provável autoria original (pasta `cursor-leopard`). Ele é copiado junto com o WacOS e utilizado pelo tema Leopard.

Caso as fontes, os sons de alerta ou o cursor do Tahoe estejam ausentes (por exemplo, em um pacote compilado por você), o antigo `instalar.cmd` pode oferecer a opção de obtê-los: as fontes diretamente da Apple (extraídas com 7-Zip e instaladas pelo `winget`, se necessário), 14 sons de alerta de [extratone/macOSsystemsounds](https://github.com/extratone/macOSsystemsounds) e o cursor pela página do DeviantArt, cujo download deve ser feito por você.

O WacOS é instalado somente para o seu usuário (sem exigir direitos de administrador) em `%LOCALAPPDATA%\Programs\WacOS`. Tudo o que ele altera é registrado em `instalacao.log`, nesse diretório. Para desinstalar, acesse **Configurações > Aplicativos > Aplicativos instalados > WacOS > Desinstalar** (ou abra novamente o `WacOSSetup.exe`). Os antigos scripts `instalar.cmd` e `desinstalar.cmd` continuam disponíveis no pacote para uso avançado.

- Encerrar o WacOS: `Ctrl+Alt+Shift+F12`. Saída de emergência (restaura o Explorer): `Ctrl+Alt+Shift+F11`.

### Usar somente os aplicativos ou somente as barras de título

Já possui sua própria configuração? O `WacOSSetup.exe` permite escolher o que instalar:

1. **WacOS completo** (barra de menus, Dock, Finder...).
2. **Somente os aplicativos escolhidos** (Finder, Notas, Calculadora, Música, Pré-Visualização...): os atalhos são adicionados a uma pasta **WacOS** no menu Iniciar. Cada aplicativo desenha sua própria barra de título Mac, com ou sem o shell em execução.
3. **Somente as barras de título Mac** nos demais aplicativos: `WacOSTitleBars.exe`, com opção de inicialização ao entrar no Windows. Desative com `WacOSTitleBars.exe --sair` (ele também é encerrado quando o shell completo é iniciado e assume o controle).
4. **Aplicativos + barras de título**.

As opções 2 a 4 mantêm a barra de tarefas do Windows, o Explorer e suas pastas como estão, e não adicionam a tarefa de inicialização do shell. Os temas e o idioma são lidos dos Ajustes do Sistema (instale esse aplicativo para alterá-los). Os aplicativos que nunca devem receber a barra de título Mac devem ser adicionados, um por linha, em `%APPDATA%\Shell\janelas-excluidas.txt`.

Os aplicativos integrados mantêm os cantos das janelas desenhados pelo Windows, para que modificações que alteram o raio dos cantos (como os cantos arredondados do Windhawk) não deixem mais uma moldura quadrada ao redor deles.

## Atualizações

Novas versões são publicadas como releases numeradas (`build-N`). Em **Ajustes do Sistema > Geral > Atualização de Software**, o WacOS consulta o GitHub automaticamente, mostra a versão instalada, a versão mais recente e os commits que ainda não estão presentes na sua instalação. A opção **Atualizar Agora** abre o `WacOSSetup.exe`, que baixa o novo pacote, verifica seu SHA-256 em comparação com o valor publicado pelo GitHub e instala a atualização (somente programas, ícones e sons são substituídos; suas configurações são preservadas).

O GitHub Actions verifica se cada commit enviado para `main` pode ser compilado.

## É seguro?

É uma pergunta justa para qualquer programa que substitua a barra de tarefas. Você não precisa confiar apenas no autor:

- **Código-fonte disponível.** Todos os arquivos estão neste repositório: você pode ler o código e compilar o projeto por conta própria (consulte [Compilação](#compilação)).
- **Rede.** Sem telemetria e sem contas. Estes são os endereços acessados pelo próprio código do WacOS:
  - `api.open-meteo.com`: previsão do tempo. Recebe as coordenadas das cidades escolhidas por você (até que uma cidade seja selecionada, utiliza uma cidade estimada a partir do fuso horário do Windows). O shell consulta o serviço na inicialização e a cada 15 minutos, mesmo com o widget oculto (exceto no modo "Somente Dock e barra de menus"); o aplicativo Clima faz o mesmo enquanto está aberto.
  - `geocoding-api.open-meteo.com`: pesquisa de cidades no Clima e no Relógio. Recebe o nome pesquisado e o idioma utilizado.
  - `date.nager.at`: feriados nacionais no Calendário (ativado por padrão; desmarque esse calendário para interromper as consultas). Recebe o ano e o código do país definidos pela região do Windows.
  - `suggestqueries.google.com`: sugestões do Spotlight. Recebe o que você digita no Spotlight e seu idioma, conforme você escreve. Está ativado por padrão; `spotlight_google=0` em `%APPDATA%\Shell\ajustes.txt` desativa o recurso.
  - `api.github.com`, com `github.com` como alternativa: atualizações. O shell verifica 90 segundos após iniciar e a cada 6 horas (desativar a notificação de atualização não interrompe a verificação). A página Atualização de Software e o `WacOSSetup.exe` também fazem consultas durante a atualização. A verificação também envia o identificador do commit instalado para listar o que mudou.
  - `github.com` e `release-assets.githubusercontent.com`: download do pacote quando você escolhe atualizar.
  - Somente no antigo `instalar.cmd`, para arquivos ausentes do pacote e apenas se você autorizar: `devimages-cdn.apple.com` (fontes), `raw.githubusercontent.com` (sons) e `winget`, que utiliza seus próprios endereços (7-Zip, para extrair as fontes).

  Links, pesquisas na web e a página do cursor no DeviantArt são abertos no navegador. Execute o WacOS atrás de um firewall como simplewall ou GlassWire para monitorar as conexões por conta própria (a verificação de atualizações aparece como `powershell.exe`).
- **O que é instalado.** Uma tarefa agendada para iniciar quando você entrar no Windows, com permissões normais de usuário. Nenhum serviço, nenhum driver e nenhuma permissão de administrador. A desinstalação (Aplicativos instalados > WacOS) remove tudo.
- **Os jogos são preservados.** A DLL das barras de título nunca é carregada em jogos, inicializadores ou programas com sistemas antitrapaça: ela verifica a pasta do programa, arquivos da engine e do sistema antitrapaça próximos ao executável, classes de janela da engine e tela cheia exclusiva. Mesmo que seja carregada, recusa-se a iniciar dentro de um jogo. Adicione suas próprias exceções em `%APPDATA%\Shell\janelas-excluidas.txt`.
- **Antivírus.** Um shell precisa executar ações que podem parecer suspeitas para sistemas heurísticos: iniciar junto com o Windows, utilizar hooks de teclado e mouse para atalhos como a tecla Win e os Cantos Ativos, além de carregar sua DLL de barras de título em outros aplicativos. O pacote build-12 apresenta [1/66 no VirusTotal](https://www.virustotal.com/gui/file/310ab4f16824500a8d77225fef66a62e499c32d6d5484a8752b119b7f24e67f3), uma única detecção heurística baseada em IA. Os scripts de instalação são arquivos PowerShell simples que podem ser lidos: não há C# incorporado, janelas ocultas ou código disfarçado, e o atualizador verifica o SHA-256 do pacote em comparação com o valor publicado pelo GitHub. Caso o Microsoft Defender sinalize o programa no seu PC, trata-se de um falso positivo já reportado à Microsoft; abra uma [issue](https://github.com/renzyxx/WacOS/issues) informando o nome da ameaça.

## Testes e estabilidade

O WacOS é testado no PC do autor após cada alteração, e o [registro de mudanças](docs/registro-mudancas.txt) (em português) documenta o que foi modificado, como foi verificado e o que não foi testado. Alguns dos procedimentos:

- **Teste de estresse** (15 a 30 minutos): programas de teste abrem, minimizam e fecham janelas continuamente enquanto o Mission Control e o Launchpad são abertos e fechados. A memória do shell, os handles e os objetos GDI/USER são monitorados. Esses recursos aumentam e diminuem de acordo com o uso do Mission Control, em vez de crescerem continuamente; os aplicativos de teste com a DLL de barras de título permanecem estáveis após mais de 60 ciclos.
- **Watchdog:** o shell reinicia sozinho quando realmente trava. Um falso alarme durante animações pesadas (que reiniciava um shell ainda funcional) foi identificado no teste de estresse e corrigido: não ocorreram novos reinícios nos testes seguintes.
- **Animações quadro a quadro:** a animação Genie e as demais animações do compositor são gravadas com Desktop Duplication (uma captura de tela comum não consegue registrá-las) e verificadas quadro a quadro. Foi assim que o efeito "pixelado" da animação Genie foi associado aos recortes em degraus e corrigido com recortes trapezoidais.
- **Barras de título entre atualizações:** as barras de título são carregadas por meio de um pequeno carregador fixo (`WacOSGancho.dll`), evitando que atualizar ou reiniciar o WacOS consuma os slots limitados de hooks do Windows. Isso foi testado substituindo a DLL das barras de título três vezes durante uma única sessão de usuário.
- **Investigação de travamentos:** uma thread de monitoramento registra em `shell.log` a pilha de chamadas de qualquer bloqueio da interface com duração igual ou superior a 1 segundo. O shell e a DLL das barras de título nunca aguardam o disco, o Registro ou outro processo na thread de uma janela. Em um disco ocupado, isso reduziu travamentos de vários segundos no shell e em aplicativos como Discord e Chrome para apenas alguns milissegundos.
- **Bugs relatados** no Reddit, GitHub e Discord são reproduzidos sempre que possível (vídeos ajudam bastante!).

Limitações conhecidas: aplicativos executados como administrador não podem receber a barra de título Mac quando o shell não está elevado; além disso, a primeira instalação do build-18 ou posterior exige sair da sessão do Windows uma vez para que o novo carregador das barras de título assuma o controle.

## Roadmap

- **Dynamic Island**: mais atividades (inspiradas no [Atoll](https://github.com/Ebullioscopic/Atoll); nenhum código foi copiado).
- **Releases assinadas**, para evitar que heurísticas de antivírus sinalizem os scripts do instalador.
- Tem alguma ideia? [Abra uma issue](https://github.com/renzyxx/WacOS/issues).

## Apoie o projeto

O WacOS é gratuito e desenvolvido no tempo livre do autor. Se você gosta do projeto, um café ajuda bastante e contribui para manter as atualizações ☕

<a href="https://buymeacoffee.com/renzyx"><img src="https://cdn.buymeacoffee.com/buttons/v2/default-yellow.png" alt="Compre um café para mim" height="50"></a>

Adicionar uma estrela ao repositório e compartilhar o projeto também ajuda.

## Compilação

É necessário ter o Visual Studio 2022 (ou Build Tools 2022) com a carga de trabalho de desenvolvimento em C++.

```text
build.cmd            everything
build.cmd shell      somente o shell (WacOSShell.exe: barra de menus, Dock, Spotlight...)
build.cmd finder     somente o Finder
build.cmd janelas    barras de título Mac para outros aplicativos (WacOSWindow.dll e seu carregador
                     fixo WacOSGancho.dll, versões de 32 e 64 bits, e WacOSTitleBars.exe)
build.cmd titulos    somente WacOSTitleBars.exe (barras de título sem o shell)
tools\empacotar.ps1  pacote de instalação (dist\WacOS-<date>.zip)
```

## Estrutura

- `src/shell`: o shell (barra de menus, Dock, Spotlight, Central de Controle, Mission Control, Liquid Glass...).
- `src/finder`, `src/ajustes` (Ajustes do Sistema), `src/monitor` (Monitor de Atividade) e os demais aplicativos em `src/*`.
- `src/janelas`: barras de título Mac injetadas nas janelas de outros aplicativos (`gancho.cpp` é o carregador fixo usado pelos hooks do Windows; ele carrega a DLL atual `macjanela.cpp`); `src/titulos`: barras de título sem o shell.
- `src/menu`: menus de contexto sobre as extensões reais do shell do Windows.
- `src/comum`: código compartilhado (nomes, idioma, vidro e sombras).
- `instalador`: scripts de instalação, atualização e desinstalação. `tools`: ferramentas de compilação e empacotamento.
- `icones`: pacotes de ícones (Sequoia, Tahoe) e SF Symbols.

## Desempenho

Medições realizadas no PC do autor (i9-14900K, RTX 3070 Ti, 3440×1440 a 174 Hz). Em repouso, o shell utiliza aproximadamente 1% de um núcleo da CPU e nenhum tempo de GPU; com um jogo em primeiro plano (Modo de Jogo ativado), utiliza cerca de 2% de um núcleo e nenhum tempo de GPU. Memória: aproximadamente 350 a 500 MB para o shell (a maior parte corresponde ao índice de arquivos do Spotlight), além de cerca de 100 MB para o Finder oculto, que permite abrir janelas do Finder instantaneamente. As animações são executadas no ritmo de atualização da tela, e não por temporizadores. Quando o sistema está ocioso, nada fica fazendo consultas periódicas. Minimizar uma janela exige cerca de 55 ms de preparação (captura da janela, níveis de detalhe e aproximadamente 400 recortes animados) antes de a animação Genie começar; o restante é executado pela GPU no compositor do Windows.

Em notebooks com duas GPUs, configure o `WacOSShell.exe` como **Alto desempenho** em Configurações do Windows > Sistema > Tela > Elementos gráficos. Caso as animações ainda apresentem travamentos, Ajustes do Sistema > Aparência > Transparência e Efeitos > **Reduzir transparência** desativa o desfoque do Liquid Glass, o efeito de maior custo.

## Licença

**O WacOS disponibiliza seu código-fonte, mas não é open source.** Copyright (c) 2026 renzyxx, todos os direitos reservados. 
Consulte [LICENSE](LICENSE) para ver os termos completos.

- ✅ Você pode ler o código, compilar e executar o WacOS em seus próprios computadores para uso pessoal e enviar pull requests.
- ❌ Você não pode copiar o código (ou partes dele) para outro projeto, redistribuir o WacOS (reuploads, espelhos, pacotes redistribuídos ou versões modificadas) ou utilizá-lo comercialmente sem autorização por escrito.

Quer utilizar algum recurso do WacOS em seu próprio projeto ou realizar uma adaptação? Abra uma issue e solicite autorização primeiro.

Os ícones, fontes, sons e papéis de parede da Apple, assim como os pacotes de ícones e cursores de terceiros incluídos no pacote de lançamento, não estão cobertos pela licença: pertencem aos seus respectivos proprietários e são incluídos somente para uso pessoal. Um mod do Windhawk em `docs/windhawk/bloqueio-sequoia/` deriva de código licenciado sob GPL-3.0 e mantém essa licença.

## Observação

Apple, macOS, Finder e nomes e ícones relacionados pertencem à Apple Inc. Este é um projeto pessoal e não comercial, sem vínculo ou afiliação com a Apple. Os ícones, fontes e sons incluídos destinam-se somente ao uso pessoal.

</details>
