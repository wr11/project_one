-- GearEngine 示例脚本
-- 这个脚本展示了如何使用 GearEngine 的 Lua API

-- 服务器启动回调
function on_server_start()
    gear.log("服务器已启动！")
    gear.log("等待客户端连接...")
end

-- 客户端连接回调
function on_client_connect(client)
    gear.log("新客户端已连接")
    
    -- 发送欢迎消息
    local welcome_msg = "欢迎连接到 GearEngine 服务器！\n"
    gear.send_data(client, welcome_msg)
end

-- 客户端数据接收回调
function on_client_data(client, data)
    gear.log("收到客户端数据: " .. data)
    
    -- 回显数据
    local echo_msg = "服务器收到: " .. data
    gear.send_data(client, echo_msg)
    
    -- 如果收到 "quit"，关闭连接
    if data:match("quit") then
        gear.log("客户端请求断开连接")
        gear.close_client(client)
    end
end

-- 客户端断开连接回调
function on_client_disconnect(client)
    gear.log("客户端已断开连接")
    gear.log("当前连接数: " .. gear.get_client_count())
end

-- 服务器停止回调
function on_server_stop()
    gear.log("服务器正在停止...")
end

gear.log("示例脚本已加载")
