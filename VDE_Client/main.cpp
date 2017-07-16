//============================================================================
// Name        : main.cpp
// Author      : sricker
// Version     :
// Copyright   : Your copyright notice
// Description : main program for VDE Project
//============================================================================

#include <unistd.h>
#include <exception>
#include <iostream>
#include <fstream>
#include <stdio.h>

#include <string>
#include <stdlib.h>
#include <stdint.h>
#include <modbus.h>
#include <ctime>
using namespace std;

typedef union LongRegister
{
    uint8_t asBytes[4];
    uint16_t asWords[2];
    int32_t asLong;
} longRegister_t;

#define WAIT_CYCLES  	20
const std::string WAIT_TIME = "5";

const std::string cmdFile = "commands_new.txt";

std::string GetLineFromFileContaining(std::string match)
{
    std::ifstream fileInput;
    unsigned int curLine = 0;
    std::string line;

    fileInput.open("log.txt");

    while (getline(fileInput, line))
    {
        curLine++;

        if (line.find(match, 0) != string::npos)
        {
            break;
        }
    }

    fileInput.close();

    return line;
}

std::string GetOutputFile()
{
    std::string content;
    std::ifstream infile;

    infile.open("log.txt");

    // explicitly reserved space for file contents
    infile.seekg(0, std::ios::end);
    content.reserve(infile.tellg());
    infile.seekg(0, std::ios::beg);

    content.assign((std::istreambuf_iterator<char>(infile)),
            std::istreambuf_iterator<char>(infile));

    infile.close();

    return content;
}

void WriteToOutputFile(std::string text)
{
    std::ofstream outfile;
    outfile.open("log.txt", std::ios_base::app);
    outfile << text << std::flush;
    outfile.close();
}

std::string ExecuteShellCommand(std::string command)
{
    FILE *in;
    char buff[256];
    static std::string str;

    str.clear();
    WriteToOutputFile(command);

    try
    {
        if (command[command.size() - 1] == '\r')
            command.erase(command.size() - 1); // remove \r

        if (command.size() > 0)
        {
            if (!(in = popen((command + " 2>&1").c_str(), "r")))
            {
                return nullptr;
            }

            while (fgets(buff, sizeof(buff), in) != 0)
            {
                str.append(buff);
            }

            pclose(in);
        }
    } catch (exception& e)
    {
        pclose(in);
        WriteToOutputFile(e.what());
    }

    WriteToOutputFile(str);
    return str;
}

int Modbus()
{
    int errno;

    modbus_t *ctx = modbus_new_rtu("/dev/ttyUSB0", 115200, 'N', 8, 1);

    modbus_set_slave(ctx, 1);

    if (modbus_connect(ctx) == -1)
    {
        cout << "Connection failed:" << modbus_strerror(errno) << endl;
        modbus_free(ctx);
        return -1;
    }

    std::time_t result = std::time(0);
    std::cout << std::asctime(std::localtime(&result)) << result
            << " seconds since the Epoch\n";

    longRegister_t l_register;

    // write Epoch time
    //    l_register.asLong = result ;
    //
    //    uint8_t temp = l_register.asBytes[0];
    //    l_register.asBytes[0] = l_register.asBytes[1];
    //    l_register.asBytes[1] = temp;
    //    temp = l_register.asBytes[2];
    //    l_register.asBytes[2] = l_register.asBytes[3];
    //    l_register.asBytes[3] = temp;

    //    modbus_flush(ctx);

    //    int m_result = modbus_write_registers(ctx, 0x100, 2, &l_register.asWords[0]);

    //    if(m_result == -1)
    //    {
    //    	cout << "Message failed:" << modbus_strerror(errno) << endl;
    //    	modbus_free(ctx);
    //    	return -1;
    //    }

    modbus_flush(ctx);

    int m_result = modbus_read_input_registers(ctx, 0x100, 2,
            &l_register.asWords[0]);

    if (m_result == -1)
    {
        cout << "Message failed:" << modbus_strerror(errno) << endl;
        modbus_free (ctx);
        return -1;
    }

    cout << "result = " << m_result << endl;
    cout << "value = " << l_register.asLong << endl;

    std::cout << "time from micro = "
            << std::asctime(std::localtime((time_t *) &l_register.asLong))
            << endl;

    modbus_close(ctx);
    modbus_free(ctx);

}

int main()
{
    int tries = 0;
    int errno;
    std::string result;
    std::size_t position;

    system("ls -al");
    system("sleep 5");
    system("mv log.txt logbak.txt");

    try
    {

        Modbus();


        cout << "checking connection" << endl << std::flush;

        do
        {
            tries++;
            result = ExecuteShellCommand("ping -c 1 dropbox.com");
            position = result.find("1 packets transmitted, 1 received");

            if (position < result.size()) // host unknown == failed ping
            {
                break;
            }
            else
            {
                system("sleep 5");
            }

        } while (tries < WAIT_CYCLES);

        if (tries >= WAIT_CYCLES)
        {
            WriteToOutputFile("echo exceeded number of tries to establish connection. aborting cycle\r");
            //    	ExecuteShellCommand("");  // shutdown

        }

        ExecuteShellCommand("./uploader download commands.txt commands_new.txt\r");

        std::ifstream cmdfile;

        cmdfile.open("commands_new.txt", std::ifstream::in);

        if(!cmdfile.good())
        {
            cmdfile.open("Debug/commands_new.txt", std::ifstream::in);
        }

        std::string line;

        while (getline(cmdfile, line))
        {
            if(line.find("gphoto2") == 0)
            {
                ExecuteShellCommand(line);
            }
            else if(line.find("command=") == 0)
            {
                ExecuteShellCommand(line.substr(8)); // execute everything after "="
            }
        }

        std::string outFile = GetLineFromFileContaining("Saving");

        std::size_t pos = outFile.find("Saving");

        if (pos < outFile.length())
        {
            std::string filename = outFile.substr(pos + 15, 18);

            if (filename.find("jpg") == filename.length() - 3) // make sure we have a valid filename to upload
            {
                std::string uploadcmd = "./uploader upload " + filename + " "
                        + filename; // build commmand for upload

                ExecuteShellCommand(uploadcmd);
            }
        }

        system("./uploader upload log.txt oplog.txt");
    } catch (std::exception const &exc)
    {
        std::cerr << "Exception caught " << exc.what() << "\n";
    } catch (...)
    {
        std::cerr << "Unknown exception caught\n";
    }

    exit(0);
}

