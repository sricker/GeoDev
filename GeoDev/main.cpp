/*
 * main.cpp
 *
 *  Created on: Jun 18, 2017
 *      Author: sricker
 */



//============================================================================
// Name        : GeoDev.cpp
// Author      : Shawn Ricker
// Version     :
// Copyright   : Don't you steal it!
// Description : Hello World in C++, Ansi-style
//============================================================================

#include <unistd.h>

//#include <thread>
//#include <mutex>

#include <iostream>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>

#include <string>
#include <stdlib.h>
#include <modbus.h>
#include <ctime>
#include <sstream>
#include <vector>
#include <iterator>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>
using namespace std;

//#define CAPTURE_STDIO


class StdCapture
{
public:
    StdCapture(): m_capturing(false), m_init(false), m_oldStdOut(0), m_oldStdErr(0)
    {
        m_pipe[READ] = 0;
        m_pipe[WRITE] = 0;

        if (pipe(m_pipe) == -1)
            return;

        m_oldStdOut = dup(fileno(stdout));
        m_oldStdErr = dup(fileno(stderr));

        if (m_oldStdOut == -1 || m_oldStdErr == -1)
            return;

        m_init = true;
    }

    ~StdCapture()
    {
        if (m_capturing)
        {
            EndCapture();
        }
        if (m_oldStdOut > 0)
            close(m_oldStdOut);
        if (m_oldStdErr > 0)
            close(m_oldStdErr);
        if (m_pipe[READ] > 0)
            close(m_pipe[READ]);
        if (m_pipe[WRITE] > 0)
            close(m_pipe[WRITE]);
    }


    void BeginCapture()
    {
        if (!m_init)
            return;
        if (m_capturing)
            EndCapture();

        fflush(stdout);
        fflush(stderr);

        dup2(m_pipe[WRITE], fileno(stdout));
        dup2(m_pipe[WRITE], fileno(stderr));

        m_capturing = true;
    }

    bool EndCapture()
    {
        if (!m_init)
            return false;
        if (!m_capturing)
            return false;

        fflush(stdout);
        fflush(stderr);
        dup2(m_oldStdOut, fileno(stdout));
        dup2(m_oldStdErr, fileno(stderr));
        m_captured.clear();

        std::string buf;
        const int bufSize = 1024;
        buf.resize(bufSize);
        int bytesRead = 0;

      //  if (!eof(m_pipe[READ]))
     //   {
            bytesRead = read(m_pipe[READ], &(*buf.begin()), bufSize);
     //   }

        while(bytesRead == bufSize)
        {
            m_captured += buf;
            bytesRead = 0;

           // if (!eof(m_pipe[READ]))
          //  {
                bytesRead = read(m_pipe[READ], &(*buf.begin()), bufSize);
         //   }
        }
        if (bytesRead > 0)
        {
            buf.resize(bytesRead);
            m_captured += buf;
        }
        return true;
    }

    std::string GetCapture() const
    {
        std::string::size_type idx = m_captured.find_last_not_of("\r\n");
        if (idx == std::string::npos)
        {
            return m_captured;
        }
        else
        {
            return m_captured.substr(0, idx+1);
        }
    }

private:
    enum PIPES { READ, WRITE };
    int m_pipe[2];
    int m_oldStdOut;
    int m_oldStdErr;
    bool m_capturing;
    bool m_init;
    std::string m_captured;
};

class ReadCommandFile
{
	FILE *stream;
	char *line = NULL;
	char *filePath ;

public:

	ReadCommandFile(char const *path) : stream(NULL)
	{
		filePath = (char *)path;

		stream = fopen(filePath, "r");

		if(stream == NULL)
		{
			perror("fopen");
		}

	}
	~ReadCommandFile()
	{

	}

	template<typename Out>
	void split(const std::string &s, char delim, Out result)
	{
		std::stringstream ss;
		ss.str(s);
		std:string item;

		while(std::getline(ss, item, delim))
		{
			*(result++) = item;
		}
	}

	std::vector<std::string> GetNextLine(void)
	{
		size_t len;
		ssize_t readSize;
		std::vector<std::string> elems;

		readSize = getline(&line, &len, stream);

		if(readSize != -1)
		{
			split(line, '=', std::back_inserter(elems));
		}

		return elems;
	}



};

typedef union LongRegister
{
	uint8_t asBytes[4];
	uint16_t asWords [2];
	int32_t asLong;
}longRegister_t;


int main()
{
#ifdef CAPTURE_STDIO
    StdCapture sc;
    sc.BeginCapture();
#endif
    int errno;





//    system("gphoto2 --usage");




/*
    // cout << "!!!Hello World!!!" << endl; // prints !!!Hello World!!!
    modbus_t *ctx = modbus_new_rtu("/dev/ttyUSB0", 115200, 'N', 8, 1) ;
    modbus_set_slave(ctx, 1);

    if(modbus_connect(ctx) == -1)
    {
    	cout << "Connection failed:" << modbus_strerror(errno) << endl;
    	modbus_free(ctx);
    	return -1;
    }

    std::time_t result = std::time(0);
    std::cout << std::asctime(std::localtime(&result))
                  << result << " seconds since the Epoch\n";

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



	int m_result = modbus_read_input_registers(ctx,0x100,2, &l_register.asWords[0]);

    if(m_result == -1)
    {
    	cout << "Message failed:" << modbus_strerror(errno) << endl;
    	modbus_free(ctx);
    	return -1;
    }

	cout << "result = " << m_result << endl;
	cout << "value = " << l_register.asLong << endl;

    std::cout << "time from micro = " << std::asctime(std::localtime((time_t *)&l_register.asLong)) << endl;


	modbus_close(ctx);
	modbus_free(ctx);
	*/

//    ReadCommandFile commands("commands_new.txt");

    system("./uploader download commands.txt commands_new.txt");  // download the command file

    std::vector<std::string> nameValuePair;

    boost::property_tree::ptree pt;
    boost::property_tree::ini_parser::read_ini("commands_new.txt", pt);

    for(auto& section : pt)
    {
    	std::cout << '[' << section.first << "]\n" ;
    	for (auto& key : section.second)
    	{
    		std::cout << key.first << "=" << key.second.get_value<std::string>() << "\n";
    		if(key.first.compare(0,12,"command_line") == 0) // compare
    		{
  				system(key.second.get_value<std::string>().c_str());
    		}
    	}
    }

#ifdef CAPTURE_STDIO
    sc.EndCapture();
    ofstream outputFile;
    outputFile.open("oplog.txt");
    outputFile << sc.GetCapture() << endl; // write the output to a file
    outputFile.close();
#endif

    return 0;
}



