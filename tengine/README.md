# Tengine

## TIM-VX 接入
### 需要了解的过程
- 模型转换工具 convert_tool
- 模型量化工具 quant_tool_uint8

### 几个注意事项
- 建议使用 ubuntu18.04 完成环境搭建。
- 官方文档维护不是很及时，有些文档说明已过时，不能运行的程序，重新编译。
- 模型转换时，使用 onnx 源模型成功率高一些。如果报出某些算子转换不支持，就用[简化工具](https://github.com/daquexian/onnx-simplifier)先转换一次 onnx 模型。
- 尽量使用 Khadas 系列的板子，稳定。s905D3 目前刷的是 Android。
- 官方的 benchmark 工具不支持 timvx 后端，我修改一点后端配置（timvx_benchmark.cc）。把内容拷贝到过去，编译即可。
- paddle lite 的 docker，ndk 默认路径为 /opt/android-ndk-r17c
- benchmark 编译完是在 build/benchmark 目录下，别被官方文档坑了。


### 编译环境准备（以 Khadas VIM3 S905D3 为例）
```shell
# 拉 tim-vx
$ git clone https://github.com/VeriSilicon/TIM-VX.git
# 拉 tengine
$ git clone https://github.com/OAID/Tengine.git tengine-lite
# 一些拷贝操作
$ cd <tengine-lite-root-dir>
$ cp -rf ../TIM-VX/include  ./source/device/tim-vx/
$ cp -rf ../TIM-VX/src      ./source/device/tim-vx/
# 准备第三方库
$ wget -c https://github.com/VeriSilicon/TIM-VX/releases/download/v1.1.28/arm_android9_A311D_6.4.3.tgz
$ tar zxvf arm_android9_A311D_6.4.3.tgz
$ mv arm_android9_A311D_6.4.3 prebuild-sdk-android
$ cd <tengine-lite-root-dir>
$ mkdir -p ./3rdparty/tim-vx/include
$ mkdir -p ./3rdparty/tim-vx/lib/android
$ cp -rf ../prebuild-sdk-android/include/*  ./3rdparty/tim-vx/include/
$ cp -rf ../prebuild-sdk-android/lib/*      ./3rdparty/tim-vx/lib/android/
```

### 编译
```shell
$ export ANDROID_NDK=<your-ndk-root-dir>
$ cd <tengine-lite-root-dir>
$ mkdir build && cd build
$ cmake -DTENGINE_ENABLE_TIM_VX=ON -DCMAKE_TOOLCHAIN_FILE=$ANDROID_NDK/build/cmake/android.toolchain.cmake -DANDROID_ABI="armeabi-v7a" -DANDROID_ARM_NEON=ON -DANDROID_PLATFORM=android-25 ..
$ make -j`nproc` && make install
```

### 运行
```shell
# 把 libtengine-lite.so、模型拷贝到设备
export LD_LIBRARY_PATH=.:$LD_LIBRARY_PATH

./tm_benchmark
```