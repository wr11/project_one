-- Chat Server Lua Script for GearEngine
-- This script implements a simple chat server

-- Client storage: map client pointer to client info
local clients = {}
local client_count = 0

-- Server startup callback
function on_server_start()
    gear.log("Chat server started!")
    gear.log("Waiting for clients to connect...")
end

-- Client connection callback
function on_client_connect(client)
    client_count = client_count + 1
    local client_id = tostring(client):sub(-8)  -- Use last 8 chars as ID
    
    -- Store client info
    clients[client] = {
        id = client_id,
        name = "User" .. client_count,
        connected = true
    }
    
    gear.log("Client connected: " .. clients[client].name .. " (ID: " .. client_id .. ")")
    gear.log("Total clients: " .. gear.get_client_count())
    
    -- Send welcome message
    local welcome = "Welcome to Chat Server! Your name is: " .. clients[client].name .. "\n"
    welcome = welcome .. "Type '/help' for commands, '/quit' to disconnect\n"
    gear.send_data(client, welcome)
    
    -- Broadcast join message to all other clients
    broadcast_message(clients[client].name .. " joined the chat", client)
end

-- Client data callback
function on_client_data(client, data)
    -- Remove trailing newline
    data = data:gsub("\r?\n$", "")
    
    if not clients[client] then
        return
    end
    
    local client_info = clients[client]
    gear.log("[" .. client_info.name .. "] " .. data)
    
    -- Handle commands
    if data:match("^/") then
        handle_command(client, data)
    else
        -- Broadcast message to all clients
        local message = "[" .. client_info.name .. "] " .. data .. "\n"
        broadcast_message(message, nil)
    end
end

-- Client disconnect callback
function on_client_disconnect(client)
    if clients[client] then
        local client_info = clients[client]
        gear.log("Client disconnected: " .. client_info.name)
        
        -- Broadcast leave message
        broadcast_message(client_info.name .. " left the chat", nil)
        
        -- Remove from clients table
        clients[client] = nil
        client_count = client_count - 1
    end
    
    gear.log("Total clients: " .. gear.get_client_count())
end

-- Server stop callback
function on_server_stop()
    gear.log("Chat server stopping...")
    -- Send goodbye message to all clients
    for client, info in pairs(clients) do
        if info.connected then
            gear.send_data(client, "Server is shutting down. Goodbye!\n")
        end
    end
end

-- Broadcast message to all clients except sender
function broadcast_message(message, exclude_client)
    for client, info in pairs(clients) do
        if client ~= exclude_client and info.connected then
            gear.send_data(client, message)
        end
    end
end

-- Handle client commands
function handle_command(client, command)
    local client_info = clients[client]
    if not client_info then
        return
    end
    
    local cmd = command:match("^/(%w+)")
    local args = command:match("^/%w+%s+(.*)$") or ""
    
    if cmd == "help" then
        local help_msg = "Available commands:\n"
        help_msg = help_msg .. "  /help     - Show this help\n"
        help_msg = help_msg .. "  /name <name> - Change your name\n"
        help_msg = help_msg .. "  /list     - List all connected users\n"
        help_msg = help_msg .. "  /quit     - Disconnect from server\n"
        gear.send_data(client, help_msg)
        
    elseif cmd == "name" then
        local new_name = args:match("^%s*(%S+)")
        if new_name and #new_name > 0 and #new_name <= 20 then
            local old_name = client_info.name
            client_info.name = new_name
            gear.send_data(client, "Name changed to: " .. new_name .. "\n")
            broadcast_message(old_name .. " is now known as " .. new_name, client)
        else
            gear.send_data(client, "Invalid name. Use: /name <name> (1-20 characters)\n")
        end
        
    elseif cmd == "list" then
        local user_list = "Connected users (" .. gear.get_client_count() .. "):\n"
        for c, info in pairs(clients) do
            if info.connected then
                user_list = user_list .. "  - " .. info.name .. "\n"
            end
        end
        gear.send_data(client, user_list)
        
    elseif cmd == "quit" then
        gear.send_data(client, "Goodbye!\n")
        gear.close_client(client)
        
    else
        gear.send_data(client, "Unknown command: " .. cmd .. ". Type /help for help.\n")
    end
end

gear.log("Chat server script loaded")
