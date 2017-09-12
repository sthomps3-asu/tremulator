#!/bin/bash

# Set mode
sudo i2cset -y 0 0x28 0x3D 0x1C
sudo i2cset -y 1 0x28 0x3D 0x1C


# Compile IMU program
g++ -std=c++0x -o imu imu.cpp

