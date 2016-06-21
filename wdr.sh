#！/bin/bash
ndk-build
#adb root
#adb remount
#adb push /WorkDir/My_Code_Pro/wdr/obj/local/armeabi/test-cameragl /system/bin
#adb push /WorkDir/My_Code_Pro/wdr/obj/local/armeabi/libcameragl.so /system/lib
#adb shell chmod 777 /system/bin/test-cameragl
#adb shell /system/bin/test-cameragl
#sleep 1
#adb pull /data/local/result.jpg ./
#adb pull /data/local/srcImage.jpg ./
#adb pull /data/local/avgLum.jpg ./
#adb pull /data/local/nonLum.jpg ./
#adb pull /data/local/maxLum.jpg ./
#adb logcat *:E
#adb logcat -s mfdenoise
#adb logcat -c
