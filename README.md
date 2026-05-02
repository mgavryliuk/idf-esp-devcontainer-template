# blink_1_3

## Table of Contents
[Prerequisites](#prerequisites)  
[Architecture](#architecture)  
[ESP RFC2217 Server](#esp-rfc2217-server)  
[Flashing the Device](#flashing-the-device)  

---

## Prerequisites

Install:

- `docker`
- VS Code + `Dev Containers` extension
- `uv` (Python virtual environment manager)

---

## Architecture
Target: **ESP32-S3**

**This project uses a split host/container workflow:**

- Firmware is built inside the devcontainer using ESP-IDF
- The ESP device is connected to the host (macOS)
- A RFC2217 proxy forwards the serial port from host → container

> Docker on macOS cannot access USB devices directly. For this reason, flashing is performed via an RFC2217 proxy running on the host.

**Development workflow:**

1. Open this repository in Visual Studio Code
2. Reopen in Dev Container
3. Use the container terminal for firmware development

---

## ESP RFC2217 Server

The RFC2217 proxy sever must run on the host machine (macOS).

1. Open a local terminal:

`Cmd + Shift + P` → `Terminal: Create New Integrated Terminal (Local)`

2. Create environment:

```bash
uv venv -p 3.12 .venv
uv pip install esptool
```
3. Activate virtual environment
```bash
source .venv/bin/activate
```

4. Start proxy server (replace device path with your ESP device)
```bash
esp_rfc2217_server -v -p 4000 /dev/tty.usbmodem1301
```


## Flashing the Device

To flash the device, first start the RFC2217 server from a local terminal. Use the ESP-IDF extension over the forwarded serial port.
