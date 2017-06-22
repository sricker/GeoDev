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


int main()
{
    StdCapture sc;
//    sc.BeginCapture();
    int errno;



    system("gphoto2 --usage");
    // cout << "!!!Hello World!!!" << endl; // prints !!!Hello World!!!
    modbus_t *ctx = modbus_new_rtu("/dev/ttyUSB0", 115200, 'N', 8, 1) ;
    modbus_set_slave(ctx, 1);

    if(modbus_connect(ctx) == -1)
    {
    	cout << "Connection failed:" << modbus_strerror(errno) << endl;
    	modbus_free(ctx);
    	return -1;
    }

//    uint16_t myRegister;

    std::time_t result = std::time(0);
    std::cout << std::asctime(std::localtime(&result))
                  << result << " seconds since the Epoch\n";

    ReadCommandFile commands("commands.txt");

    std::vector<std::string> nameValuePair;

//    do
//    {

//    for(int i = 0; i < 3 ; i++)
//    {
//    	nameValuePair = commands.GetNextLine();
//    	cout << nameValuePair[0] << ":" << nameValuePair[1] << endl;
//    }

    boost::property_tree::ptree pt;
    boost::property_tree::ini_parser::read_ini("commands.txt", pt);

    for(auto& section : pt)
    {
    	std::cout << '[' << section.first << "]\n" ;
    	for (auto& key : section.second)
    	{
    		std::cout << key.first << "=" << key.second.get_value<std::string>() << "\n";
    	}
    }

//    std::cout << pt.get <std::string>("Section1.Value1") << std::endl ;
//    std::cout << pt.get <std::string>("Section1.Value2") << std::endl ;


//
//    }while(nameValuePair[0] != NULL);





//    modbus_flush(ctx);

//    int result = modbus_read_input_registers(ctx,0xE00,1, &myRegister);

//    if(result == -1)
//    {
//    	cout << "Message failed:" << modbus_strerror(errno) << endl;
//    	modbus_free(ctx);
//    	return -1;
//    }

//    cout << "result = " << result << endl;
//    cout << "value = " << myRegister << endl;
//    sc.EndCapture();
//    cout << sc.GetCapture() ;

    modbus_close(ctx);
    modbus_free(ctx);

    return 0;
}



