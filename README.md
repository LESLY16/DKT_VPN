# DKT VPN

A cross-platform desktop VPN application built with C++ and Qt, using the WireGuard protocol for secure connections.

![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)
![Platform](https://img.shields.io/badge/platform-Windows%20%7C%20macOS%20%7C%20Linux-lightgrey)
[![Build](https://github.com/LESLY16/DKT_VPN/actions/workflows/build.yml/badge.svg)](https://github.com/LESLY16/DKT_VPN/actions/workflows/build.yml)

## Features

- Connect to VPN servers in 10 countries:
  - 🇺🇸 United States
  - 🇬🇧 United Kingdom
  - 🇩🇪 Germany
  - 🇯🇵 Japan
  - 🇨🇦 Canada
  - 🇦🇺 Australia
  - 🇧🇷 Brazil
  - 🇫🇷 France
  - 🇳🇱 Netherlands
  - 🇸🇬 Singapore
- One-click connect/disconnect
- Real-time connection status monitoring
- Data transfer statistics (bytes sent/received)
- Connection duration timer
- Cross-platform: Windows, macOS, Linux

## Architecture

The application integrates with the system WireGuard installation, managing `.conf` files and using `wg-quick` / `wg` commands for VPN control.

- **Linux / macOS**: Uses `wg-quick up` / `wg-quick down` with privilege escalation (`pkexec` / `sudo`)
- **Windows**: Uses `wireguard.exe /installtunnelservice` / `/uninstalltunnelservice`

## Prerequisites

### All platforms
- [Qt 6](https://www.qt.io/download) (or Qt 5.15+) — Widgets and Network modules
- [CMake](https://cmake.org/) 3.16+
- [WireGuard](https://www.wireguard.com/install/) installed on the system

### Platform-specific
| Platform | WireGuard command | Notes |
|----------|-------------------|-------|
| Linux    | `wg-quick`, `wg` | Install `wireguard-tools` via your package manager |
| macOS    | `wg-quick`, `wg` | Install via `brew install wireguard-tools` |
| Windows  | `wireguard.exe`   | Install from [wireguard.com](https://www.wireguard.com/install/) |

## Configuration

Before connecting, you must add your WireGuard server credentials. Template configuration files are provided in the `configs/` directory (e.g., `configs/dkt-us.conf.template`).

1. Copy the template for your desired server, e.g.:
   ```
   cp configs/dkt-us.conf.template configs/dkt-us.conf
   ```
2. Edit the `.conf` file and fill in:
   - `PrivateKey` — your WireGuard private key
   - `Address` — your assigned VPN IP address
   - `PublicKey` — the server's public key
   - `Endpoint` — the server's IP/hostname and port

The application looks for configs in the following locations (in order):
1. Directory specified by `DKT_VPN_CONFIG_DIR` environment variable
2. `~/.config/dkt-vpn/` (Linux/macOS) or `%APPDATA%\dkt-vpn\` (Windows)
3. The `configs/` directory next to the executable

On Linux, configs are also copied to `/etc/wireguard/` (requires root) before activation.

## Building

```bash
# Clone the repository
git clone https://github.com/LESLY16/DKT_VPN.git
cd DKT_VPN

# Configure
cmake -B build -DCMAKE_BUILD_TYPE=Release

# Build
cmake --build build --config Release
```

The resulting binary is placed in `build/` (Linux/macOS) or `build/Release/` (Windows).

### Linux quick start
```bash
sudo apt install qt6-base-dev cmake wireguard-tools
cmake -B build && cmake --build build
./build/DKT_VPN
```

### macOS quick start
```bash
brew install qt cmake wireguard-tools
cmake -B build -DCMAKE_PREFIX_PATH=$(brew --prefix qt) && cmake --build build
./build/DKT_VPN
```

### Windows quick start
1. Install Qt via the [online installer](https://www.qt.io/download-qt-installer)
2. Install WireGuard from [wireguard.com](https://www.wireguard.com/install/)
3. Open *x64 Native Tools Command Prompt for VS* and run:
   ```
   cmake -B build -DCMAKE_PREFIX_PATH=C:\Qt\6.6.0\msvc2019_64
   cmake --build build --config Release
   ```

## Running

On Linux and macOS the application requires elevated privileges to manage WireGuard tunnels. You will be prompted via a graphical dialog (`pkexec` / `sudo`).

On Windows, run the application as Administrator.

```bash
# Linux / macOS
./build/DKT_VPN

# Windows (run as Administrator)
build\Release\DKT_VPN.exe
```

## License

MIT — see [LICENSE](LICENSE).