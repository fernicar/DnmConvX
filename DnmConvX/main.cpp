// main.cpp:
//
#include<windows.h> // LPCTSTR might still be used by other headers or it is an oversight. Keeping for now.
#include "DnmConvX.h"
#include <string> // For std::string
#include <iostream> // For std::cout, std::endl

// using namespace std; // Removed

//
// Global variables
//
const int CONSOLE_PRINT_TITLE = -2; // Replaced TITLE macro
const int CONSOLE_PRINT_HELP = -1;  // Replaced HELP macro
// #define ERROR 1 // This ERROR macro is not used like E_ERROR enum members. E_ERROR::E_Error is 6.

// LPCTSTR g_pjname="DnmConvX";	// Global variable: Project name // Removed

//
// Prototype function
// 
// Print console help usage
int printConsole(int option_val); // Changed signature: i32 to int, option to option_val
//
// Main entry
// 
int main(int argc, char *argv[]){ // Changed signature: i32 to int
	const char* projectName = "DnmConvX"; // Local constant for project name

	printConsole(CONSOLE_PRINT_TITLE);
	CDnmConvX dcx;
	if(argc==1)
		return printConsole(CONSOLE_PRINT_HELP);
	else{
		// Assuming E_ERROR is an enum class or enum defined in DnmConvX.h
		// And that E_DnmRead and E_MemoryError are members of it.
		E_ERROR lastError = dcx.inputDnmFile(argv[1]);
		if(lastError == E_ERROR::E_DnmRead) return printConsole(static_cast<int>(lastError));

		std::string ini_path_from_exe(argv[0]); // Qualified string
		size_t last_sep_pos = ini_path_from_exe.find_last_of("/\\");
		std::string base_path_from_exe = (last_sep_pos == std::string::npos) ? "" : ini_path_from_exe.substr(0, last_sep_pos + 1);

		std::string exe_name_part = (last_sep_pos == std::string::npos) ? ini_path_from_exe : ini_path_from_exe.substr(last_sep_pos + 1);
		size_t dot_pos = exe_name_part.find_last_of('.');
		std::string exe_basename = (dot_pos == std::string::npos) ? exe_name_part : exe_name_part.substr(0, dot_pos);

		std::string ini_file_path = base_path_from_exe + exe_basename + ".ini";

		if(static_cast<int>(lastError) < static_cast<int>(E_ERROR::E_MemoryError)) { // Cast E_ERROR to int for comparison if necessary
			lastError=dcx.inputIniFile(ini_file_path.c_str());
		}

		std::string ini_path_from_arg(argv[1]); // Qualified string
		dot_pos = ini_path_from_arg.find_last_of('.');
		std::string base_name_from_arg = (dot_pos == std::string::npos) ? ini_path_from_arg : ini_path_from_arg.substr(0, dot_pos);
		ini_file_path = base_name_from_arg + ".ini";

		if(static_cast<int>(lastError) < static_cast<int>(E_ERROR::E_MemoryError)) { // Cast E_ERROR to int for comparison
			lastError=dcx.inputIniFile(ini_file_path.c_str());
		}

        // Finalize data after all input files are processed
        if(static_cast<int>(lastError) < static_cast<int>(E_ERROR::E_MemoryError)) { // Check if previous steps were successful
            dcx.finalizeData(); // This line is already present, ensuring it stays.
        }

		if(static_cast<int>(lastError) < static_cast<int>(E_ERROR::E_MemoryError)) { // Cast E_ERROR to int for comparison
			lastError=dcx.outputToXFile();
		}
		return static_cast<int>(lastError); // Return int
	}
}
// Print console help usage
int printConsole(int option_val){ // Changed signature: i32 to int, option to option_val
	const char* projectName = "DnmConvX"; // Local constant for project name

	if(option_val==CONSOLE_PRINT_TITLE)
	{
		std::cout<<projectName<<" version 1.0\n" // Qualified cout, g_pjname to projectName
			<< "Convert YS Flight DNM file format to DirectX file.\n\n";
	}

	if(option_val==CONSOLE_PRINT_HELP)
	{
		std::cout<<"Usage:\n" // Qualified cout
			<<projectName<<".exe <input file> [output file]\n\n" // g_pjname to projectName
			<<std::endl // Qualified endl
			<<"  <input file> Specify the input file path.\n"
			<<"Example:\n\n"
			<<projectName<<".exe airplane.dnm\n\n" // g_pjname to projectName
			<<"              It will output to airplane.x\n";
		return 1; // Typically help message display might return non-zero to indicate non-standard execution path
	}

	// Assuming E_ERROR enum values are positive or zero.
	// The original ERROR macro was 1. E_ERROR::E_Error is 6.
	// This condition checks if option_val is one of the error codes from E_ERROR.
	if(option_val >= static_cast<int>(E_ERROR::E_NotError)) // E_NotError is 0, other errors are > 0
	{
		std::cout<< "An error occurred (code: " << option_val << "). Please check your input file or parameters.\n";
		std::cout<< "Invalid command line? please check the usage help\n\n" // Qualified cout
			<< "Try typping:\n\t\t "<<projectName<<".exe /H\n\n" // g_pjname to projectName
			<< "Or\n\n"
			<< "Try typping:\n\t\t "<<projectName<<".exe <input file>\n\n"; // g_pjname to projectName
		return option_val;
	}
	return 0;
} // end of "Print console help usage"
;