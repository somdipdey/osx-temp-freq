/*
 * Copyright @ Somdip Dey, 2018
 *
 * Inspired by:
 * Apple System Management Control (SMC) Tool.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include <stdio.h>
#include <termios.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <IOKit/IOKitLib.h>

#include "smc.h"

static io_connect_t conn;

UInt32 _strtoul(char *str, int size, int base)
{
    UInt32 total = 0;
    int i;

    for (i = 0; i < size; i++)
    {
        if (base == 16)
            total += str[i] << (size - 1 - i) * 8;
        else
           total += (unsigned char) (str[i] << (size - 1 - i) * 8);
    }
    return total;
}

void _ultostr(char *str, UInt32 val)
{
    str[0] = '\0';
    sprintf(str, "%c%c%c%c", 
            (unsigned int) val >> 24,
            (unsigned int) val >> 16,
            (unsigned int) val >> 8,
            (unsigned int) val);
}

kern_return_t SMCOpen(void)
{
    kern_return_t result;
    io_iterator_t iterator;
    io_object_t   device;

    CFMutableDictionaryRef matchingDictionary = IOServiceMatching("AppleSMC");
    result = IOServiceGetMatchingServices(kIOMasterPortDefault, matchingDictionary, &iterator);
    if (result != kIOReturnSuccess)
    {
        printf("Error: IOServiceGetMatchingServices() = %08x\n", result);
        return 1;
    }

    device = IOIteratorNext(iterator);
    IOObjectRelease(iterator);
    if (device == 0)
    {
        printf("Error: no SMC found\n");
        return 1;
    }

    result = IOServiceOpen(device, mach_task_self(), 0, &conn);
    IOObjectRelease(device);
    if (result != kIOReturnSuccess)
    {
        printf("Error: IOServiceOpen() = %08x\n", result);
        return 1;
    }

    return kIOReturnSuccess;
}

kern_return_t SMCClose()
{
    return IOServiceClose(conn);
}


kern_return_t SMCCall(int index, SMCKeyData_t *inputStructure, SMCKeyData_t *outputStructure)
{
    size_t   structureInputSize;
    size_t   structureOutputSize;

    structureInputSize = sizeof(SMCKeyData_t);
    structureOutputSize = sizeof(SMCKeyData_t);

    #if MAC_OS_X_VERSION_10_5
    return IOConnectCallStructMethod( conn, index,
                            // inputStructure
                            inputStructure, structureInputSize,
                            // ouputStructure
                            outputStructure, &structureOutputSize );
    #else
    return IOConnectMethodStructureIStructureO( conn, index,
                                                structureInputSize, /* structureInputSize */
                                                &structureOutputSize,   /* structureOutputSize */
                                                inputStructure,        /* inputStructure */
                                                outputStructure);       /* ouputStructure */
    #endif

}

kern_return_t SMCReadKey(UInt32Char_t key, SMCVal_t *val)
{
    kern_return_t result;
    SMCKeyData_t  inputStructure;
    SMCKeyData_t  outputStructure;

    memset(&inputStructure, 0, sizeof(SMCKeyData_t));
    memset(&outputStructure, 0, sizeof(SMCKeyData_t));
    memset(val, 0, sizeof(SMCVal_t));

    inputStructure.key = _strtoul(key, 4, 16);
    inputStructure.data8 = SMC_CMD_READ_KEYINFO;

    result = SMCCall(KERNEL_INDEX_SMC, &inputStructure, &outputStructure);
    if (result != kIOReturnSuccess)
        return result;

    val->dataSize = outputStructure.keyInfo.dataSize;
    _ultostr(val->dataType, outputStructure.keyInfo.dataType);
    inputStructure.keyInfo.dataSize = val->dataSize;
    inputStructure.data8 = SMC_CMD_READ_BYTES;

    result = SMCCall(KERNEL_INDEX_SMC, &inputStructure, &outputStructure);
    if (result != kIOReturnSuccess)
        return result;

    memcpy(val->bytes, outputStructure.bytes, sizeof(outputStructure.bytes));

    return kIOReturnSuccess;
}

double SMCGetTemperature(char *key)
{
    SMCVal_t val;
    kern_return_t result;

    result = SMCReadKey(key, &val);
    if (result == kIOReturnSuccess) {
        // read succeeded - check returned value
        if (val.dataSize > 0) {
            if (strcmp(val.dataType, DATATYPE_SP78) == 0) {
                // convert sp78 value to temperature
                int intValue = val.bytes[0] * 256 + (unsigned char)val.bytes[1];
                return intValue / 256.0;
            }
        }
    }
    // read failed
    return 0.0;
}


double convertToFahrenheit(double celsius) {
  return (celsius * (9.0 / 5.0)) + 32.0;
}

uint64_t rdtsc(void)
{
    uint32_t ret0[2];
    __asm__ __volatile__("rdtsc" : "=a"(ret0[0]), "=d"(ret0[1]));
    return ((uint64_t)ret0[1] << 32) | ret0[0];
}

// Get CPU temperature
double getCPUTemperature(){
    SMCOpen();
    double temperature = SMCGetTemperature(SMC_KEY_CPU_TEMP);
    SMCClose();
    
    return temperature;
}

// Get CPU frequency
uint64_t getCPUFrequency(){
    uint64_t startCount = rdtsc();
    sleep(1);
    uint64_t endCount = rdtsc();
    
    uint64_t frequency = (endCount - startCount);
    
    return frequency;
}

// Write to CSV
int writeToCsv(char *filename,double temperature,uint64_t frequency){

    //current time -->
    time_t rawtime;
    struct tm * timeinfo;
    
    time ( &rawtime );
    timeinfo = localtime ( &rawtime );
    
    FILE *fp;
    
    if( access( filename, F_OK ) != -1 ) {
        // file exists
        fp=fopen(filename,"a");
    } else {
        // file doesn't exist
        fp=fopen(filename,"w+");
    }
    
    fprintf(fp,"%d:%d:%d,%llu,%0.1f\n", timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec, frequency, temperature);
    fclose(fp);
    printf(".");
    return 101;
}

// For key board hit
int kbhit(void)
{
    struct termios oldt, newt;
    int ch;
    int oldf;
    
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    oldf = fcntl(STDIN_FILENO, F_GETFL, 0);
    fcntl(STDIN_FILENO, F_SETFL, oldf | O_NONBLOCK);
    
    ch = getchar();
    
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    fcntl(STDIN_FILENO, F_SETFL, oldf);
    
    if(ch != EOF)
    {
        ungetc(ch, stdin);
        return 1;
    }
    
    return 0;
}


int main(int argc, char *argv[])
{
    double temperature = 0;
    uint64_t frequency = 0;
    char scale = 'C';
    
    // for sleep -->
    struct timespec time1, time2;
    time1.tv_sec  = 0;
    time1.tv_nsec = 10000L;//0.00001 secs
    
    temperature = getCPUTemperature();
    frequency = getCPUFrequency();
    
    //current time -->
    time_t rawtime;
    struct tm * timeinfo;
    
    time ( &rawtime );
    timeinfo = localtime ( &rawtime );
    
    printf("CPU Temperature at %d:%d:%d : %0.1f°%c", timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec, temperature, scale);
    printf("\n");
    printf("CPU Frequency at %d:%d:%d : %llu", timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec, frequency);
    printf("\n");

    if(argc > 1)
    {
        char *filename = argv[1];
        printf("\nRecording CPU frequency and temperature in %s\n",filename);
        while(!kbhit())
        {
            temperature = getCPUTemperature();
            frequency = getCPUFrequency();
            writeToCsv(filename,temperature,frequency);
            nanosleep(&time1 , &time2);
        }
        printf("\nProgram exited successfully with code: 0\n");
    }
    
    return 0;
}
