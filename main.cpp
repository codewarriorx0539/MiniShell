#include <iostream>
#include <cmath>
#include <ctime>
#include <pthread.h>
#include <vector>
#include <string>
#include <sstream>

#include <cstddef> 
#include<fcntl.h>

#include<unistd.h>
#include<sys/wait.h>
#include <stdio.h>
#include <stdlib.h>
#include <cstring>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/tcp.h>

#include <sys/ipc.h>
#include <sys/shm.h>
#include <errno.h>
#include <cctype>
#include <algorithm>

////////////////////////

using namespace std;

// Individual command class
class Command{

	public:
		string unixCMD;
		vector<string> strArguments;
		string reDirect;
		string reDirectArg;

};


// Argument class
class  CArg{

public:
	char **pArgArray;
	CArg(){pArgArray = NULL;};
	~CArg(){ if(pArgArray != NULL)
			delete pArgArray;
		};

};


// Function Prototypes mostly for parsing
void sepByPipes(string &strCommandLine, vector<string> &unparsedCommands);

void sepCommand(vector<string> &unparsedCommands, vector<Command> &Commands);

void sepCommandFromArgs(string &rawCMD, Command &newCommand);
		
void printCommands( vector<Command> &Commands);

void printCommand(vector<Command> &Commands, int );

void findPos(int &Pos1, int &Pos2, int &PosFinal);

void createStrArgs(Command &iCommand, char ** &args);


int main(int argc, char *argv[])
{

	vector<Command> Commands;

	int *pPipeDescript = NULL;

	int readFD;
	int writeFD;
		
	pid_t pID = -1;

	if( pID != 0) /// Will only be executed once.
	{
		
		string strCommandLine;

		vector<string> tokens;

		vector<string> unparsedCommands;
	
		string strBuffer; 	

		strCommandLine.clear();

		cout << endl << "Jake-Bash> ";

		getline (cin, strCommandLine);

		sepByPipes(strCommandLine, unparsedCommands);

		sepCommand(unparsedCommands,Commands);

		if(Commands.size() > 1) // If bigger than one create array of pipes
		{
		 	pPipeDescript = new int[ (Commands.size() - 1) * 2 ];
		}
		

	}

/////////////////////////////////////// An extremely clever way to create a bash like shell with forks and pipes

	if(Commands.size() == 0)
	{
		// NO COMMANDS SO EXIT
		exit(1); 
	}
	else if( Commands.size() == 1 )
	{
		//  DO NOTHING
		
	}
	else if( Commands.size() == 2)
	{
		// ONE PIPE 
		pipe( &pPipeDescript[0] );
	}
	else
	{
		///// Create Multiple Pipes
		int iterations = Commands.size()-1;

		for (int i =0; i < iterations ; i++ )
		{
			pipe( &pPipeDescript[ i * 2 ] );
		}

	}


    for(int i = 0; i < Commands.size(); i++)
    {	
    
	pID = fork();

	if( pID == 0 )
	{	

		CArg execArg;

		if( i == 0 )
		{


			// Input file
			if(Commands[0].reDirect.compare("<") == 0)
			{
				if( (readFD = open( Commands[0].reDirectArg.c_str() , O_RDONLY ) ) < 0 ) 
				{
					cerr << endl << "File failed" << endl;
					exit(1);
				}  

				dup2(readFD,0);
				close(readFD);

			}
			// Output file
			else if( Commands[0].reDirect.compare(">") == 0 )
			{
				if( (writeFD = open( Commands[0].reDirectArg.c_str() , O_RDWR | O_CREAT  ) ) < 0 ) 
				{
					cerr << endl << "File failed" << endl;
					exit(1);
				}  

				dup2(writeFD,1);
				close(writeFD);
			}

			// If more than one command
			if(Commands.size() > 1)
			{
				dup2(pPipeDescript[ 1 ], 1); 
				close(pPipeDescript[ 1 ]);
			} 	

		}
		// last Command
		else if ( i == ( Commands.size() - 1 ) )
		{
			
			// Redirect to file
			if(Commands[i].reDirect.compare(">") == 0 )
			{
				if( (writeFD = open( Commands[i].reDirectArg.c_str() , O_RDWR | O_CREAT ) ) < 0 ) 
				{
					cerr << endl << "File failed" << endl;
					exit(1);
				}  

				dup2(writeFD,1);
				close(writeFD);

			}
		
			dup2(pPipeDescript[ (i * 2) - 2], 0);
			close(pPipeDescript[(i * 2) - 2 ]);
			
		}
		else
		// Middle Node
		{
			dup2(pPipeDescript[ ( i + (i - 2))  ] , 0); 	
			close(pPipeDescript[ ( i + (i - 2)) ]); 

			/////////////
			close(pPipeDescript[i * 2]);
			////////////

			dup2(pPipeDescript[ (i * 2) + 1 ], 1);
			close(pPipeDescript[ (i * 2) + 1 ]); 	

		}

		// Create c-like strings
		createStrArgs(Commands[i], execArg.pArgArray);

		// Execute the command and arguments
		execvp(Commands[i].unixCMD.c_str(), execArg.pArgArray);
		cout << endl << "Unknown Command" << endl; 
		exit(1);
		// CHILD WILL NEVER EXEC COUT 
        }
	else if(pID < 0) /* FAIL */
	{
		exit(1);
	}
	else /* IN PARENT CLOSING */
	{
		
		if( i == 0 ) // First node
		{

			if(Commands.size() > 1)
			{
				close(pPipeDescript[ 1 ]); 
			}
		}
		else if ( i == ( Commands.size() - 1 ) ) // Last Node
		{
			close(pPipeDescript[(i * 2) - 2 ]);
		}
		else					// Middle Node
		{
			close(pPipeDescript[ ( i + (i - 2)) ]);
			
			close(pPipeDescript[ (i * 2) + 1 ]);
		}
	}

    }

	// Wait for children to complete
	while ( (pID = wait( (int *) 0 ) ) != -1) 
	    ;   

	if(pPipeDescript != NULL)// If array created delete it.
		delete pPipeDescript;

	return EXIT_SUCCESS;

}

// Print all commands in vector.
void printCommands( vector<Command> &Commands)
{
	
	for(int i = 0; i < Commands.size(); i++)
	{
		cout << endl << " Unix Command [" << i + 1 << "]: " << Commands[i].unixCMD << endl;
	
		for(int j = 0; j < Commands[i].strArguments.size(); j++)
		{
			cout << endl << " Argument Number " << j + 1 << " :" << Commands[i].strArguments[j] << endl; 

		}

		cout << endl << " Redirection Character: " << Commands[i].reDirect << endl;
		cout << endl << " redirection Argument: " << Commands[i].reDirectArg << endl;
		cout << endl << endl;
	}


}


// Break up as raw commands. First tokenize by pipe
void sepByPipes(string &strCommandLine, vector<string> &unparsedCommands)
{
	string strBuffer;
	stringstream ss(strCommandLine);

	if(strCommandLine == "")
		exit(1);		

	while ( getline(ss, strBuffer,'|') )
        	unparsedCommands.push_back(strBuffer);

	

}

// Seperate the individual elements of a command and push into vector
void sepCommand(vector<string> &unparsedCommands, vector<Command> &Commands)
{


	for(int i = 0; i < unparsedCommands.size() ; i++)
	{

		Command newCommand;
		string strBuffer;
		string strReDirectArg;
		
		string rawCMD;

		rawCMD = unparsedCommands[i];

		stringstream ss(rawCMD);

		if(string::npos != rawCMD.find("<"))
		{
			
			getline(ss, strBuffer,'<');
			
			sepCommandFromArgs(strBuffer, newCommand);
			
			ss >> newCommand.reDirectArg;

			newCommand.reDirect = "<";

		}

		else if(string::npos != rawCMD.find(">"))
		{
			getline(ss, strBuffer,'>');

			sepCommandFromArgs(strBuffer, newCommand);

			ss >> newCommand.reDirectArg;
			
			newCommand.reDirect = ">";
		}
		else
		{

			sepCommandFromArgs(rawCMD, newCommand);


		}

        	Commands.push_back(newCommand);
		

	}

}

// A very clever way to extract all the tokens from a command including the command itself and
// non printed characters
void sepCommandFromArgs(string &rawCMD, Command &newCommand)
{

	
	string strBuffer;
	stringstream ss(rawCMD);

	string strArgs;

	int firstPos;

	int firstPos1;
	int firstPos2;

	int secondPos;

	int secondPos1;
	int secondPos2;

	

	if(string::npos != rawCMD.find("\"") || string::npos != rawCMD.find("\'"))
	{

		ss >> newCommand.unixCMD;

		stringstream CMDstream;
		CMDstream << ss.rdbuf();
		strArgs = CMDstream.str();

		firstPos1 = strArgs.find("\"");
		firstPos2 = strArgs.find("\'");

		findPos(firstPos1, firstPos2, firstPos );

		while(string::npos != firstPos)
		{
			
			secondPos1 = strArgs.find("\"", firstPos + 1);
			secondPos2 = strArgs.find("\'", firstPos + 1);
				
			/////////////
			findPos(secondPos1 , secondPos2, secondPos );
			/////////////
			

			string preStr = strArgs.substr(0,firstPos);
			string Quote = strArgs.substr(firstPos+1, (secondPos-firstPos) -1);
			string postStr = strArgs.substr(secondPos+1);

			stringstream ssi(preStr);

			while( ssi >> strBuffer)
				newCommand.strArguments.push_back(strBuffer);

			newCommand.strArguments.push_back(Quote);

			strArgs = postStr;
			
			firstPos1 = strArgs.find("\"");	
			firstPos2 = strArgs.find("\'");	

			findPos(firstPos1, firstPos2, firstPos );

		}

	
		stringstream last(strArgs);

		while( last >> strBuffer)
			newCommand.strArguments.push_back(strBuffer);


	}
	else
	{

		ss >> newCommand.unixCMD;

		while ( ss >> strBuffer )
        		newCommand.strArguments.push_back(strBuffer);
	}
}

// A positioning function to find the closest token
void findPos(int &Pos1, int &Pos2, int &PosFinal)
{
	bool done1 = false;
	bool done2 = false;

	if(Pos1 == string::npos)
	{
		done1 = true;
	}
	
	if(Pos2 == string::npos)
	{
		done2 = true;
	}

	if(done1 == true && done2 == true  )
	{
		PosFinal = string::npos;
	}
	else if(done1 == true)
	{
		PosFinal = Pos2;
		
	}
	else if(done2 == true)
	{

		PosFinal = Pos1;
	}
	else
	{
		if( Pos1 < Pos2)
			PosFinal = Pos1;
		else
			PosFinal = Pos2;
	}

}

// Create an array of null terminated strings
void createStrArgs(Command &iCommand, char ** &args)
{

	args = (char**) new char(iCommand.strArguments.size() + 2);
	
	
	args[0] = new char(iCommand.unixCMD.size() + 1);
	strcpy(args[0], iCommand.unixCMD.c_str());

	for(int i = 1; i <= iCommand.strArguments.size(); i++)
	{
		args[i] = new char(iCommand.strArguments[i-1].size() + 1);
		strcpy( args[i] , iCommand.strArguments[i-1].c_str() );
	}
	

	args[ iCommand.strArguments.size() + 1 ] = '\0';

}

// Print a command
void printCommand(vector<Command> &Commands, int i)
{

	cout << endl << endl << " Unix Command [" << i + 1 << "]: " << Commands[i].unixCMD << endl;
	
	for(int j = 0; j < Commands[i].strArguments.size(); j++)
	{
		cout << endl << " Argument Number " << j + 1 << " :" << Commands[i].strArguments[j] << endl; 

	}

	cout << endl << " Redirection Character: " << Commands[i].reDirect << endl;
	cout << endl << " redirection Argument: " << Commands[i].reDirectArg << endl;
	cout << endl << endl;

}

