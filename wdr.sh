#！/bin/bash
/opt/android-ndk-linux/android-ndk-r10e/ndk-build
adb root
adb remount
adb push /WorkDir/My_Code_Pro/wdr/obj/local/armeabi/test-cameragl /system/bin
adb push /WorkDir/My_Code_Pro/wdr/obj/local/armeabi/libcameragl.so /system/lib
adb shell chmod 777 /system/bin/test-cameragl
adb shell /system/bin/test-cameragl
sleep 1
#adb pull /sdcard/wdrSrc.jpg ./
adb pull /data/local/resultAVG.jpg ./
adb pull /data/local/resultNON.jpg ./
adb pull /data/local/resultMAX.jpg ./
#adb logcat *:E
adb logcat -s mfdenoise
adb logcat -c
