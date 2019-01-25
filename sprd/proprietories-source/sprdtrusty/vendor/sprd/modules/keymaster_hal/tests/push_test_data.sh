#!/bin/bash
adb root
adb remount
adb shell mkdir /data/cer
adb push ec_privkey_pk8.der /data/cer
adb push rsa_privkey_pk8.der /data/cer
adb push km0_sw_rsa_512.blob /data/cer
adb push km1_sw_ecdsa_256.blob /data/cer
adb push km1_sw_rsa_512.blob /data/cer
adb push km1_sw_rsa_512_unversioned.blob /data/cer
