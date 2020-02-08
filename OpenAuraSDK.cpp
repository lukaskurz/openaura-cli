/******************************************************************************************\
*                                                                                          *
*   OpenAuraSDK.cpp                                                                        *
*                                                                                          *
*       Functions for communicating with Asus Aura devices on Windows and Linux            *
*                                                                                          *
\******************************************************************************************/

#include "AuraController.cpp"
#include "i2c_smbus.cpp"
#include <vector>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <cstring>
#include <sstream>
#include <iostream>

#include "i2c_smbus_linux.cpp"
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>

std::vector<AuraController *> controllers;
std::vector<i2c_smbus_interface *> busses;

/******************************************************************************************\
*                                                                                          *
*   DetectI2CBusses (Linux)                                                                *
*                                                                                          *
*       Detects available SMBUS adapters and enumerates i2c_smbus_interface objects for    *
*       them                                                                               *
*                                                                                          *
\******************************************************************************************/

void DetectI2CBusses()
{
    i2c_smbus_linux *       bus;
    char                    device_string[1024];
    DIR *                   dir;
    char                    driver_path[512];
    struct dirent *         ent;
    int                     test_fd;

    // Start looking for I2C adapters in /sys/class/i2c-adapter/
    strcpy(driver_path, "/sys/class/i2c-adapter/");
    dir = opendir(driver_path);

    if(dir == NULL)
    {
        return;
    }

    // Loop through all entries in i2c-adapter list
    ent = readdir(dir);
    while(ent != NULL)
    {
        if(ent->d_type == DT_DIR || ent->d_type == DT_LNK)
        {
            if(strncmp(ent->d_name, "i2c-", 4) == 0)
            {
                strcpy(device_string, driver_path);
                strcat(device_string, ent->d_name);
                strcat(device_string, "/name");
                test_fd = open(device_string, O_RDONLY);

                if(test_fd)
                {
                    read(test_fd, device_string, sizeof(device_string));
                    close(test_fd);

                    //ignore nvidia adapters for now
                    if(strncmp(device_string, "NVIDIA", 6) == 0)
                    {
                        ent = readdir(dir);
                        continue;
                    }

                    bus = new i2c_smbus_linux();
                    strcpy(bus->device_name, device_string);

                    strcpy(device_string, "/dev/");
                    strcat(device_string, ent->d_name);
                    test_fd = open(device_string, O_RDWR);

                    if (test_fd < 0)
                    {
                        delete bus;
                        ent = readdir(dir);
                        continue;
                    }

                    bus->handle = test_fd;
                    busses.push_back(bus);
                }
            }
        }
        ent = readdir(dir);
    }

}   /* DetectI2CBusses() */

/******************************************************************************************\
*                                                                                          *
*   TestForAuraController                                                                  *
*                                                                                          *
*       Tests the given address to see if an Aura controller exists there.  First does a   *
*       quick write to test for a response, and if so does a simple read at 0xA0 to test   *
*       for incrementing values 0...F which was observed at this location during data dump *
*                                                                                          *
\******************************************************************************************/

bool TestForAuraController(i2c_smbus_interface* bus, unsigned char address)
{
    bool pass = false;

    int res = bus->i2c_smbus_write_quick(address, I2C_SMBUS_WRITE);
    
    if (res >= 0)
    {
        pass = true;

        for (int i = 0xA0; i < 0xB0; i++)
        {
            res = bus->i2c_smbus_read_byte_data(address, i);

            if (res != (i - 0xA0))
            {
                pass = false;
            }
        }
    }

    return(pass);

}   /* TestForAuraController() */


/******************************************************************************************\
*                                                                                          *
*   DetectAuraControllers                                                                  *
*                                                                                          *
*       Detect Aura controllers on the enumerated I2C busses.  Searches for Aura-enabled   *
*       RAM at 0x77 and tries to initialize their slot addresses, then searches for them   *
*       at their correct initialized addresses.  Also looks for motherboard controller at  *
*       address 0x4E.                                                                      *
*                                                                                          *
*           bus - pointer to i2c_smbus_interface where Aura device is connected            *
*           dev - I2C address of Aura device                                               *
*                                                                                          *
\******************************************************************************************/

void DetectAuraControllers()
{
    AuraController* new_controller;

    for (unsigned int bus = 0; bus < busses.size(); bus++)
    {
        // Remap Aura-enabled RAM modules on 0x77
        for (unsigned int slot = 0; slot < 8; slot++)
        {
            int res = busses[bus]->i2c_smbus_write_quick(0x77, I2C_SMBUS_WRITE);

            if (res < 0)
            {
                break;
            }

            AuraController temp_controller(busses[bus], 0x77);

            temp_controller.AuraRegisterWrite(AURA_REG_SLOT_INDEX, slot);
            temp_controller.AuraRegisterWrite(AURA_REG_I2C_ADDRESS, 0xE0 + (slot << 1));
        }

        // Add Aura-enabled controllers at their remapped addresses
        for (unsigned int slot = 0; slot < 8; slot++)
        {
            if (TestForAuraController(busses[bus], 0x70 + slot))
            {
                new_controller = new AuraController(busses[bus], 0x70 + slot);
                controllers.push_back(new_controller);
            }
        }

        // Check for Aura controller at 0x40
        if (TestForAuraController(busses[bus], 0x40))
        {
            new_controller = new AuraController(busses[bus], 0x40);
            controllers.push_back(new_controller);
        }

        // Check for Aura controller at 0x4E
        if (TestForAuraController(busses[bus], 0x4E))
        {
            new_controller = new AuraController(busses[bus], 0x4E);
            controllers.push_back(new_controller);
        }

        // Check for Aura controller at 0x4F
        if (TestForAuraController(busses[bus], 0x4F))
        {
            new_controller = new AuraController(busses[bus], 0x4F);
            controllers.push_back(new_controller);
        }

        // Check for Aura controller at 0x66
        if (TestForAuraController(busses[bus], 0x66))
        {
            new_controller = new AuraController(busses[bus], 0x66);
            controllers.push_back(new_controller);
        }
    }

}   /* DetectAuraControllers() */


/******************************************************************************************\
*                                                                                          *
*   DetectI2C                                                                              *
*                                                                                          *
*       Prints a list of all detected I2C addresses on the given bus                       *
*                                                                                          *
*           bus - pointer to i2c_smbus_interface to scan                                   *
*           mode - one of AUTO, QUICK, READ, FUNC - method of access                       *
*                                                                                          *
*       Code adapted from i2cdetect.c from i2c-tools Linux package                         *
*                                                                                          *
\******************************************************************************************/

#define MODE_AUTO   0
#define MODE_QUICK  1
#define MODE_READ   2
#define MODE_FUNC   3

std::string DetectI2C(i2c_smbus_interface * bus, int mode)
{
    int i, j;
    int res;
    int slave_addr;
    char line[128];
    std::string text;

    freopen("i2cdetect.txt", "a", stdout);

    sprintf(line, "     0  1  2  3  4  5  6  7  8  9  a  b  c  d  e  f\r\n");
    text.append(line);

    for (i = 0; i < 128; i += 16)
    {
        sprintf(line, "%02x: ", i);
        text.append(line);

        for (j = 0; j < 16; j++)
        {
            /* Set slave address */
            slave_addr = i + j;

            /* Probe this address */
            switch (mode)
            {
            case MODE_QUICK:
                res = bus->i2c_smbus_write_quick(slave_addr, I2C_SMBUS_WRITE);
                break;
            case MODE_READ:
                res = bus->i2c_smbus_read_byte(slave_addr);
                break;
            default:
                if ((i + j >= 0x30 && i + j <= 0x37)
                 || (i + j >= 0x50 && i + j <= 0x5F))
                    res = bus->i2c_smbus_read_byte(slave_addr);
                else
                    res = bus->i2c_smbus_write_quick(slave_addr, I2C_SMBUS_WRITE);
                break;
            }

            if (res < 0)
            {
                sprintf(line, "-- ");
                text.append(line);
            }
            else
            {
                sprintf(line, "%02x ", i + j);
                text.append(line);
            }
        }
        sprintf(line, "\r\n");
        text.append(line);
    }

    return text;

}   /* DetectI2C() */


/******************************************************************************************\
*                                                                                          *
*   DumpAuraRegisters                                                                      *
*                                                                                          *
*       Dumps register values from an Aura device                                          *
*                                                                                          *
\******************************************************************************************/

void DumpAuraRegisters(AuraController * controller)
{
    int i, j;

    int start = 0x0000;

    FILE* file = freopen("auradump.txt", "a", stdout);

    printf("       0  1  2  3  4  5  6  7  8  9  a  b  c  d  e  f\r\n");

    for (i = 0; i < 0xFFFF; i += 16)
    {
        printf("%04x: ", i + start);

        for (j = 0; j < 16; j++)
        {
            printf("%02x ", controller->AuraRegisterRead(start + i + j));
        }

        printf("\r\n");
    }

    fclose(file);

}   /* DumpAuraRegisters() */



/******************************************************************************************\
*                                                                                          *
*   str_to_numbers                                                                         *
*                                                                                          *
*       String to Number function.  Check and convert String to number                     *
*                                                                                          *
\******************************************************************************************/

int str_to_numbers(char* s)
{
    std::istringstream ss(s);
    int x;
    if (!(ss >> x)) 
    {
        std::cout << "Invalid number: " << s << '\n';
        exit(1);
    } else if (!ss.eof()) 
    {
        std::cout << "Trailing characters after number: " << s << '\n';
        exit(1);
    }
    return x;
}




/******************************************************************************************\
*                                                                                          *
*   main                                                                                   *
*                                                                                          *
*       Main function.  Detects busses and Aura controllers, then opens the main window    *
*       Takes R G B values as arguments                                                    *
*                                                                                          *
\******************************************************************************************/

int main(int argc, char *argv[])
{
    //Check if there are only four arguments(including the program name at argv[0])
    if(argc!=4)
    {
        std::cout << "ERROR: Invalid number of arguments\n";
        std::cout << "Usage: ./program R G B\n";
        exit(1);
    }
    
    //convert strings to integers
    int rgb_r = str_to_numbers(argv[1]);
    int rgb_g = str_to_numbers(argv[2]);
    int rgb_b = str_to_numbers(argv[3]);

    //check if integers in range (0-255)
    if(rgb_r<0||rgb_r>255)
    {
        std::cout << "ERROR: R_value out of Range\n";
        exit(1);
    }
    if(rgb_g<0||rgb_g>255)
    {
        std::cout << "ERROR: R_value out of Range\n";
        exit(1);
    }
    if(rgb_b<0||rgb_b>255)
    {
        std::cout << "ERROR: R_value out of Range\n";
        exit(1);
    }

    DetectI2CBusses();

    DetectAuraControllers();

    for (int i = 0; i < controllers.size(); i++)
    {
        controllers[i]->SetAllColorsDirect(rgb_r,rgb_g,rgb_b);
        controllers[i]->SetDirect(true);
    }

    return 0;
}
