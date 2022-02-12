New-Item -Path 'C:\dev\iotsdk\sdk-cpp-workspace' -ItemType Directory
Set-Location -Path 'C:\dev\iotsdk\sdk-cpp-workspace'
git clone --recursive https://github.com/aws/aws-iot-device-sdk-cpp-v2.git
New-Item -Path 'C:\dev\iotsdk\sdk-cpp-workspace\aws-iot-device-sdk-cpp-v2-build' -ItemType Directory
Set-Location -Path 'C:\dev\iotsdk\sdk-cpp-workspace\aws-iot-device-sdk-cpp-v2-build'
cmake -DCMAKE_INSTALL_PREFIX="C:\dev\iotsdk\sdk-cpp-workspace" ../aws-iot-device-sdk-cpp-v2
cmake --build . --target install --config "Release"
