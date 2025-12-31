# 故障排除指南

## CMake 配置错误

### 问题：`cmake ..` 报错

如果运行 `cmake ..` 时遇到错误，请按照以下步骤排查：

#### 1. 检查错误信息

首先查看完整的错误信息，常见的错误包括：

- **找不到函数**：如 `mongo_setting`, `mongo_bool_setting` 等
  - ✅ 已修复：已创建 `cmake/mongo-cmake-helpers.cmake` 提供这些函数

- **找不到文件**：如 `src/common/src/*.c`
  - ✅ 已修复：已创建 `cmake/fix-libbson.cmake` 自动创建缺失目录

- **找不到目标**：如 `mongo::detail::c_platform`
  - ✅ 已修复：辅助函数会自动创建此接口库

#### 2. 清理并重新构建

```bash
cd gearengine/build
rm -rf *  # Windows: rmdir /s /q .
cmake ..
```

#### 3. 检查依赖库

确保以下目录存在且包含源码：
- `libuv/` - libuv 源码
- `libbson/` - libbson 源码  
- `lua/src/` - Lua 源码

#### 4. 如果 libbson 仍然失败

如果 libbson 构建仍然失败，可以尝试：

**选项 A：修改 libbson/CMakeLists.txt**

找到第 157 行左右：
```cmake
"${mongo-c-driver_SOURCE_DIR}/src/common/src/*.c"
```

注释掉这一行：
```cmake
# "${mongo-c-driver_SOURCE_DIR}/src/common/src/*.c"
```

**选项 B：暂时跳过 libbson**

如果不需要 BSON 功能，可以暂时跳过：

1. 在主 `CMakeLists.txt` 中注释：
```cmake
# add_subdirectory(libbson)
```

2. 在 `src/CMakeLists.txt` 中注释 libbson 相关代码：
```cmake
# if(TARGET bson_static)
#     target_link_libraries(gearengine bson_static)
#     target_compile_definitions(gearengine PRIVATE BSON_STATIC)
# elseif(TARGET bson_shared)
#     target_link_libraries(gearengine bson_shared)
# endif()
```

3. 在代码中注释 BSON 相关功能

## 编译错误

### 找不到头文件

如果编译时找不到 `uv.h`, `lua.h`, `bson/bson.h`：

1. 检查 CMake 配置是否正确设置了包含目录
2. 确保库的源码目录结构正确
3. 检查 `CMakeLists.txt` 中的 `include_directories()` 设置

### 链接错误

如果链接时出错：

1. 确保所有库都已正确构建
2. 检查 `target_link_libraries()` 中的库名称是否正确
3. Windows 上确保链接了必要的系统库（ws2_32 等）

## 运行时错误

### 找不到 DLL（Windows）

如果运行时提示找不到 DLL：

1. 将 DLL 文件复制到可执行文件目录
2. 或将 DLL 所在目录添加到 PATH 环境变量

### 脚本加载失败

如果 Lua 脚本加载失败：

1. 检查脚本路径是否正确
2. 检查脚本语法是否正确
3. 查看错误消息中的具体信息

## 获取帮助

如果问题仍然存在，请：

1. 记录完整的错误信息
2. 检查 `build/CMakeCache.txt` 中的配置
3. 查看 `build/CMakeFiles/CMakeError.log` 中的详细错误
