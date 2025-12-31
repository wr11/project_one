# 跳过 libbson 的安装步骤（如果不需要安装）
# 这个文件可以在 libbson 的 CMakeLists.txt 中 include，用于跳过有问题的安装步骤

# 重写 install(EXPORT mongo-platform-targets) 以避免错误
# 由于我们无法直接修改 libbson 的 CMakeLists.txt，我们需要在它执行之前设置变量

# 设置一个标志，表示我们不需要安装平台目标
set(_SKIP_MONGO_PLATFORM_EXPORT TRUE)
