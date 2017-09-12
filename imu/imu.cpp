#include <linux/i2c-dev.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <cstdlib>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <fstream>
#include <stddef.h>
#include "bno055.h"
#include <string.h>
#include <iostream>
#include <ctime>
#include <signal.h>
#include <chrono>

#define O_RDWR 02
#define COLUMNS 14
#define DEV_ADDR 0x28

using namespace std;

__s32 rowData[COLUMNS-1];

int DATA_REGISTERS[13] = {BNO055_ACCEL_DATA_X_LSB_ADDR, BNO055_ACCEL_DATA_Y_LSB_ADDR, BNO055_ACCEL_DATA_Z_LSB_ADDR, BNO055_GYRO_DATA_X_LSB_ADDR, BNO055_GYRO_DATA_Y_LSB_ADDR, BNO055_GYRO_DATA_Z_LSB_ADDR, BNO055_MAG_DATA_X_LSB_ADDR, BNO055_MAG_DATA_Y_LSB_ADDR, BNO055_MAG_DATA_Z_LSB_ADDR, BNO055_QUATERNION_DATA_W_LSB_ADDR, BNO055_QUATERNION_DATA_X_LSB_ADDR, BNO055_QUATERNION_DATA_Y_LSB_ADDR, BNO055_QUATERNION_DATA_Z_LSB_ADDR};

void readData( __s32 *rowData, int file);
void delay(int del);
void writeToCsv( __s32 *rowData);
void sigHandler(int sig);

ofstream myfile;
uint64_t now;

int main()
{
    printf("Collecting data... Press ctrl+c to exit.\n");

    signal(SIGINT, sigHandler);

    // Setup CSV output file.
    myfile.open ("imudata.csv");

    myfile << "TIMESTAMP,ACCEL_DATA_X, ACCEL_DATA_Y, ACCEL_DATA_Z, GYRO_DATA_X, GYRO_DATA_Y, GYRO_DATA_Z, MAG_DATA_X, MAG_DATA_Y, MAG_DATA_Z, QUATERNION_DATA_W, QUATERNION_DATA_X, QUATERNION_DATA_Y, QUATERNION_DATA_Z\n";

    // Setup I2C device
    int file;
    int adapter_nr = 1; /* probably dynamically determined */
    char filename[20];
    snprintf(filename, 19, "/dev/i2c-%d", adapter_nr);
    file = open(filename, 2);
    if (file < 0)
    {
        /* ERROR HANDLING; you can check errno to see what went wrong */
        printf("ERROR: Setting up I2C device\n");
        return 1;
    }

    if (ioctl(file, I2C_SLAVE, DEV_ADDR) < 0)
    {
        /* ERROR HANDLING; you can check errno to see what went wrong */
        printf("ERROR: Initializing I2C.\n");
        exit(1);
    }

    // Collect data until ctrl-c is pressed
    while (1)
    {
        readData(rowData,file);
        writeToCsv(rowData);
        //delay(1000);
    }

    myfile.close();

    return 0;
}

void delay(int del)
{
    for (int i = 0; i < del; i++)
    {

    }
}

void readData( __s32 *rowData, int file)
{
    __s32 res;

    // Grab time of data capture
    now = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();

    // Read all registers
    for (int i=0; i < COLUMNS - 1; i++)
    {
        res = i2c_smbus_read_word_data(file, DATA_REGISTERS[i]);
        if (res < 0)
        {
            printf("ERROR: reading register data.");
        }
        else
        {
            rowData[i] = res;
        }
    }
    return;
}

void writeToCsv( __s32 *rowData)
{
    myfile << to_string(now) + ",";
    for (int i=0; i < COLUMNS - 2; i++)
    {
        myfile << to_string(rowData[i]) + ",";
    }
    myfile << rowData[COLUMNS - 2];
    myfile << '\n';
}

void sigHandler(int sig)
{
    myfile.close();
    exit(1);
}
