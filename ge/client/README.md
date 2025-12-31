# Chat Client

A simple TCP chat client for GearEngine chat server.

## Building

The client is built automatically when building the main project:

```bash
cd gearengine
./build-msys2.sh
```

The executable will be in `build/bin/gearchat_client.exe` (Windows) or `build/bin/gearchat_client` (Linux/macOS).

## Usage

```bash
# Connect to localhost:8080 with default script
./gearchat_client

# Connect to specific host and port
./gearchat_client localhost 8080

# Connect with custom Lua script
./gearchat_client localhost 8080 chat_client.lua
```

## Features

- Connect to chat server
- Send and receive messages
- Handle server commands
- Lua scriptable behavior

## Commands

- `/help` - Show help
- `/quit` - Disconnect from server
- `/exit` - Exit client

## Lua API

The client provides a `gear_client` table with:

- `gear_client.send(message)` - Send message to server
- `gear_client.is_connected()` - Check connection status
- `gear_client.disconnect()` - Disconnect from server
- `gear_client.log(message)` - Print log message

## Callbacks

You can define these functions in your Lua script:

- `on_connected()` - Called when connected to server
- `on_server_message(message)` - Called when message received from server
- `on_user_input(input)` - Called when user types input
- `on_disconnected()` - Called when disconnected from server
