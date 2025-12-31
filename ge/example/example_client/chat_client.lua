-- Chat Client Lua Script
-- This script provides client-side logic for the chat client

-- Client state
local client_state = {
    name = "Guest",
    connected = false
}

-- Connection callback
function on_connected()
    client_state.connected = true
    gear_client.log("Connected to chat server!")
    gear_client.log("Type your messages and press Enter to send")
    gear_client.log("Type '/help' for commands")
end

-- Server message callback
function on_server_message(message)
    -- Remove trailing newline for display
    message = message:gsub("\r?\n$", "")
    
    -- Print message (server already formats it)
    print(message)
end

-- User input callback
function on_user_input(input)
    if not gear_client.is_connected() then
        print("Not connected to server")
        return
    end
    
    -- Handle commands
    if input:match("^/") then
        handle_command(input)
    else
        -- Send message to server
        gear_client.send(input .. "\n")
    end
end

-- Disconnection callback
function on_disconnected()
    client_state.connected = false
    gear_client.log("Disconnected from server")
end

-- Handle client commands
function handle_command(command)
    local cmd = command:match("^/(%w+)")
    local args = command:match("^/%w+%s+(.*)$") or ""
    
    if cmd == "help" then
        print("Available commands:")
        print("  /help     - Show this help")
        print("  /name <name> - Change your name (send to server)")
        print("  /quit     - Disconnect from server")
        print("  /exit     - Exit client")
        
    elseif cmd == "quit" or cmd == "exit" then
        if cmd == "quit" then
            gear_client.send("/quit\n")
        end
        gear_client.disconnect()
        os.exit(0)
        
    else
        -- Send command to server (server will handle it)
        gear_client.send(command .. "\n")
    end
end

gear_client.log("Chat client script loaded")
