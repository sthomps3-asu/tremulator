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

__s32 rowData_bus0[COLUMNS-1];
__s32 rowData_bus1[COLUMNS-1];

int DATA_REGISTERS[13] = {BNO055_ACCEL_DATA_X_LSB_ADDR, BNO055_ACCEL_DATA_Y_LSB_ADDR, BNO055_ACCEL_DATA_Z_LSB_ADDR, BNO055_GYRO_DATA_X_LSB_ADDR, BNO055_GYRO_DATA_Y_LSB_ADDR, BNO055_GYRO_DATA_Z_LSB_ADDR, BNO055_MAG_DATA_X_LSB_ADDR, BNO055_MAG_DATA_Y_LSB_ADDR, BNO055_MAG_DATA_Z_LSB_ADDR, BNO055_QUATERNION_DATA_W_LSB_ADDR, BNO055_QUATERNION_DATA_X_LSB_ADDR, BNO055_QUATERNION_DATA_Y_LSB_ADDR, BNO055_QUATERNION_DATA_Z_LSB_ADDR};

void readData( __s32 *rowData_bus0, __s32 *rowData_bus1, int file_0, int file_1);
void delay(int del);
void writeToCsv( __s32 *rowData_bus0, __s32 *rowData_bus1);
void sigHandler(int sig);
void checkCalibration(int file_0, int file_1);
void printData(__s32 *rowData_bus0);

ofstream csv_bus0;
ofstream csv_bus1;
uint64_t now;

int main(int argc, char **argv)
{

    signal(SIGINT, sigHandler);

    time_t rawtime;
    struct tm * timeinfo;
    char buffer[80];

    time (&rawtime);
    timeinfo = localtime(&rawtime);

    strftime(buffer,sizeof(buffer),"%d-%m-%Y %I:%M:%S",timeinfo);

    string ts_string = buffer;
    // Setup CSV output file.
    csv_bus0.open ("imudata_bus0_" + ts_string + ".csv");
    csv_bus1.open ("imudata_bus1_" + ts_string + ".csv");


    csv_bus0 << "TIMESTAMP,ACCEL_DATA_X, ACCEL_DATA_Y, ACCEL_DATA_Z, GYRO_DATA_X, GYRO_DATA_Y, GYRO_DATA_Z, MAG_DATA_X, MAG_DATA_Y, MAG_DATA_Z, QUATERNION_DATA_W, QUATERNION_DATA_X, QUATERNION_DATA_Y, QUATERNION_DATA_Z\n";
    csv_bus1 << "TIMESTAMP,ACCEL_DATA_X, ACCEL_DATA_Y, ACCEL_DATA_Z, GYRO_DATA_X, GYRO_DATA_Y, GYRO_DATA_Z, MAG_DATA_X, MAG_DATA_Y, MAG_DATA_Z, QUATERNION_DATA_W, QUATERNION_DATA_X, QUATERNION_DATA_Y, QUATERNION_DATA_Z\n";

    // Setup I2C device
    int file_0;
    int file_1;
    int adapter_nr_0 = 0;
    int adapter_nr_1 = 1; /* probably dynamically determined */
    char filename_bus_0[20];
    char filename_bus_1[20];
    snprintf(filename_bus_0, 19, "/dev/i2c-%d", adapter_nr_0);
    snprintf(filename_bus_1, 19, "/dev/i2c-%d", adapter_nr_1);
    file_0 = open(filename_bus_0, 2);
    file_1 = open(filename_bus_1, 2);
    if (file_0 < 0 || file_1 < 0)
    {
        /* ERROR HANDLING; you can check errno to see what went wrong */
        printf("ERROR: Setting up I2C device\n");
        return 1;
    }

    if (ioctl(file_0, I2C_SLAVE, DEV_ADDR) < 0)
    {
        /* ERROR HANDLING; you can check errno to see what went wrong */
        printf("ERROR: Initializing I2C on bus 0.\n");
        exit(1);
    }

    if (ioctl(file_1, I2C_SLAVE, DEV_ADDR) < 0)
    {
        /* ERROR HANDLING; you can check errno to see what went wrong */
        printf("ERROR: Initializing I2C on bus 1.\n");
        exit(1);
    }

    // Poll on calibration register until fully calibrated

    if (argc != 2)
    {
        checkCalibration(file_0,file_1);
    }

    printf("Collecting data... Press ctrl+c to exit.\n");

    // Collect data until ctrl-c is pressed
    while (1)
    {
        readData(rowData_bus0,rowData_bus1,file_0,file_1);
        writeToCsv(rowData_bus0,rowData_bus1);
        //delay(1000);
    }

    csv_bus0.close();
    csv_bus1.close();

    return 0;
}

void delay(int del)
{
    for (int i = 0; i < del; i++)
    {

    }
}

void printData(__s32 *rowData_bus0)
{
    printf("\n");
    for (int i = 0; i < COLUMNS-4; i++)
    {
        printf("%d :: ", rowData_bus0[i]);
    }
    printf("\n\n");
}

void readData( __s32 *rowData_bus0, __s32 *rowData_bus1, int file_0, int file_1)
{
    __s32 res_bus0;
    __s32 res_bus1;

    // Grab time of data capture
    now = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();

    // Read all registers
    for (int i=0; i < COLUMNS - 1; i++)
    {
        res_bus0 = i2c_smbus_read_word_data(file_0, DATA_REGISTERS[i]);
        res_bus1 = i2c_smbus_read_word_data(file_1, DATA_REGISTERS[i]);
        if (res_bus0 < 0)
        {
            printf("ERROR: reading register data on bus 0.");
        }
        else if (res_bus1 < 0)
        {
            printf("ERROR: reading register data on bus 1.");
        }
        else
        {
            rowData_bus0[i] = res_bus0;
            rowData_bus1[i] = res_bus1;
        }
    }
    return;
}

void writeToCsv( __s32 *rowData_bus0, __s32 *rowData_bus1)
{
    csv_bus0 << to_string(now) + ",";
    csv_bus1 << to_string(now) + ",";
    for (int i=0; i < COLUMNS - 2; i++)
    {
        csv_bus0 << to_string(rowData_bus0[i]) + ",";
        csv_bus1 << to_string(rowData_bus1[i]) + ",";
    }
    csv_bus0 << rowData_bus0[COLUMNS - 2];
    csv_bus1 << rowData_bus1[COLUMNS - 2];
    csv_bus0 << '\n';
    csv_bus1 << '\n';
}

void sigHandler(int sig)
{
    csv_bus0.close();
    csv_bus1.close();
    exit(1);
}

void checkCalibration(int file_0, int file_1)
{
    __s32 res0;
    __s32 res1;

    printf("Waiting for calibration. Move sensors in figure 8 pattern.\n");
    while (res0 != 0xFF || res1 != 0xFF)
    {
        res0 = i2c_smbus_read_byte_data(file_0, BNO055_CALIB_STAT_ADDR);
        res1 = i2c_smbus_read_byte_data(file_1, BNO055_CALIB_STAT_ADDR);
        printf("res0 %d\n",res0);
        printf("res1 %d\n",res1);

    }
    printf("Calibration complete.\n");
    return;
}
