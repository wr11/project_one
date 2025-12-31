# 为 libbson 提供必要的 CMake 辅助函数
# 这些函数通常由 mongo-c-driver 的构建系统提供

# mongo_setting - 定义一个设置选项
function(mongo_setting name)
    set(options)
    set(oneValueArgs TYPE DEFAULT VALUE)
    set(multiValueArgs)
    cmake_parse_arguments(MONGO_SETTING "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})
    
    if(MONGO_SETTING_TYPE STREQUAL "STRING")
        if(MONGO_SETTING_DEFAULT)
            set(${name} "${MONGO_SETTING_DEFAULT_VALUE}" CACHE STRING "${MONGO_SETTING_UNPARSED_ARGUMENTS}")
        else()
            set(${name} "" CACHE STRING "${MONGO_SETTING_UNPARSED_ARGUMENTS}")
        endif()
    elseif(MONGO_SETTING_TYPE STREQUAL "BOOL")
        if(MONGO_SETTING_DEFAULT)
            set(${name} "${MONGO_SETTING_DEFAULT_VALUE}" CACHE BOOL "${MONGO_SETTING_UNPARSED_ARGUMENTS}")
        else()
            set(${name} OFF CACHE BOOL "${MONGO_SETTING_UNPARSED_ARGUMENTS}")
        endif()
    endif()
endfunction()

# mongo_bool_setting - 定义一个布尔设置选项
function(mongo_bool_setting name)
    set(options)
    set(oneValueArgs VISIBLE_IF)
    set(multiValueArgs)
    cmake_parse_arguments(MONGO_BOOL "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})
    
    if(MONGO_BOOL_VISIBLE_IF)
        # 简化处理：总是定义选项
        option(${name} "${MONGO_BOOL_UNPARSED_ARGUMENTS}" OFF)
    else()
        option(${name} "${MONGO_BOOL_UNPARSED_ARGUMENTS}" OFF)
    endif()
endfunction()

# mongo_bool01 - 将布尔值转换为 0/1
function(mongo_bool01 output_var input_var)
    if(${input_var})
        set(${output_var} 1 PARENT_SCOPE)
    else()
        set(${output_var} 0 PARENT_SCOPE)
    endif()
endfunction()

# mongo_target_requirements - 设置目标要求
function(mongo_target_requirements)
    # 手动解析参数，因为格式是：
    # mongo_target_requirements(target1 target2 LINK_LIBRARIES PUBLIC lib1 lib2 ...)
    set(args ${ARGN})
    set(targets)
    set(current_section)
    set(link_libs)
    set(compile_defs)
    set(compile_opts)
    
    # 解析参数
    foreach(arg IN LISTS args)
        if(arg STREQUAL "LINK_LIBRARIES")
            set(current_section "LINK_LIBRARIES")
        elseif(arg STREQUAL "COMPILE_DEFINITIONS")
            set(current_section "COMPILE_DEFINITIONS")
        elseif(arg STREQUAL "COMPILE_OPTIONS")
            set(current_section "COMPILE_OPTIONS")
        elseif(NOT current_section)
            # 在遇到第一个关键字之前，都是目标名称
            list(APPEND targets ${arg})
        else()
            # 根据当前部分添加到相应的列表
            if(current_section STREQUAL "LINK_LIBRARIES")
                list(APPEND link_libs ${arg})
            elseif(current_section STREQUAL "COMPILE_DEFINITIONS")
                list(APPEND compile_defs ${arg})
            elseif(current_section STREQUAL "COMPILE_OPTIONS")
                list(APPEND compile_opts ${arg})
            endif()
        endif()
    endforeach()
    
    # 为每个目标应用设置
    foreach(target IN LISTS targets)
        if(link_libs)
            set(scope PUBLIC)
            foreach(item IN LISTS link_libs)
                if(item STREQUAL "PUBLIC" OR item STREQUAL "PRIVATE" OR item STREQUAL "INTERFACE")
                    set(scope ${item})
                else()
                    # 处理生成器表达式和普通库
                    target_link_libraries(${target} ${scope} ${item})
                endif()
            endforeach()
        endif()
        
        if(compile_defs)
            set(def_scope PRIVATE)
            foreach(item IN LISTS compile_defs)
                if(item STREQUAL "PUBLIC" OR item STREQUAL "PRIVATE" OR item STREQUAL "INTERFACE")
                    set(def_scope ${item})
                else()
                    target_compile_definitions(${target} ${def_scope} ${item})
                endif()
            endforeach()
        endif()
        
        if(compile_opts)
            set(opt_scope PRIVATE)
            foreach(item IN LISTS compile_opts)
                if(item STREQUAL "PUBLIC" OR item STREQUAL "PRIVATE" OR item STREQUAL "INTERFACE")
                    set(opt_scope ${item})
                else()
                    target_compile_options(${target} ${opt_scope} ${item})
                endif()
            endforeach()
        endif()
    endforeach()
endfunction()

# 创建 mongo::detail::c_platform 接口库
# 使用内部名称避免 CMake 版本兼容性问题
if(NOT TARGET _mongo_detail_c_platform)
    add_library(_mongo_detail_c_platform INTERFACE)
    if(WIN32)
        target_link_libraries(_mongo_detail_c_platform INTERFACE ws2_32)
    endif()
    # 创建别名以便使用命名空间语法
    add_library(mongo::detail::c_platform ALIAS _mongo_detail_c_platform)
endif()

# 设置警告选项变量（如果未定义）
if(NOT DEFINED mongoc-warning-options)
    set(mongoc-warning-options "")
endif()

# mongo_verify_headers - 占位函数（简化版本）
function(mongo_verify_headers)
    # 简化实现：不做任何操作
endfunction()

# mongo_generate_pkg_config - 占位函数（简化版本）
function(mongo_generate_pkg_config)
    # 简化实现：不做任何操作
endfunction()

# 设置一些可能需要的变量
if(NOT DEFINED MONGO_CAN_VERIFY_HEADERS)
    set(MONGO_CAN_VERIFY_HEADERS OFF)
endif()
