# Chat Server

A simple TCP chat server implemented with Lua scripts for GearEngine.

## Usage

```bash
# Start server on default port 8080
./gearengine 8080 server/chat_server.lua

# Start server on custom port
./gearengine 8888 server/chat_server.lua
```

## Features

- Multi-client chat support
- Client name management
- Command handling
- Broadcast messages to all clients
- Lua scriptable behavior

## Server Commands

Clients can use these commands:

- `/help` - Show available commands
- `/name <name>` - Change your name
- `/list` - List all connected users
- `/quit` - Disconnect from server

## Lua API

The server provides a `gear` table with:

- `gear.send_data(client, data)` - Send data to specific client
- `gear.send_bson(client, bson_data)` - Send BSON data to client
- `gear.close_client(client)` - Close client connection
- `gear.get_client_count()` - Get number of connected clients
- `gear.log(message)` - Print log message

## Callbacks

The server script should define these functions:

- `on_server_start()` - Called when server starts
- `on_client_connect(client)` - Called when new client connects
- `on_client_data(client, data)` - Called when data received from client
- `on_client_disconnect(client)` - Called when client disconnects
- `on_server_stop()` - Called when server stops

## Example

See `chat_server.lua` for a complete implementation example.
