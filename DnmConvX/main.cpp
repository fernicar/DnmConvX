// main.cpp: Entry point for the DnmConvX application.
// This file handles command-line argument parsing, program flow control,
// and basic console output for titles, help messages, and errors.

#include <windows.h> // Included for LPCTSTR historically, review if still needed. For now, kept.
#include "DnmConvX.h"
#include <string>      // For std::string
#include <iostream>    // For std::cout, std::endl
#include <filesystem>  // For std::filesystem::path (C++17)
#include <vector>      // For std::vector (though not directly used in this snippet, often useful)


// Global constants for console output options
const int CONSOLE_PRINT_TITLE = -2; ///< Option value to print only the program title.
const int CONSOLE_PRINT_HELP = -1;  ///< Option value to print help/usage instructions.
const int HELP_DISPLAYED_EXIT_CODE = 1; ///< Exit code when help is displayed.


// Prototype for the console printing function
int printConsole(int option_val);

/**
 * @brief Main entry point of the DnmConvX application.
 *
 * Handles command-line arguments to specify input DNM file and optionally an output X file.
 * It attempts to load INI configuration files from two locations:
 * 1. Alongside the executable with the same base name as the executable.
 * 2. Alongside the input DNM file with the same base name as the DNM file.
 *
 * The program flow is:
 * - Print program title.
 * - Check for sufficient arguments; print help if none.
 * - Load DNM file.
 * - Attempt to load INI from executable's directory.
 * - Attempt to load INI from input DNM file's directory (settings from this can override the first INI).
 * - Finalize data (applies configurations, calculates normals, etc.).
 * - Output the converted data to an X file.
 * - Return an error code based on success or failure of operations.
 *
 * @param argc Number of command-line arguments.
 * @param argv Array of command-line argument strings.
 * @return Program exit code (0 for success, E_ERROR enum values for failures, HELP_DISPLAYED_EXIT_CODE if help was shown).
 */
int main(int argc, char *argv[]){
	const char* projectName = "DnmConvX"; // Local constant for project name.

	printConsole(CONSOLE_PRINT_TITLE); // Print program title and version.
	CDnmConvX dcx; // Create the main converter object.

	// If no arguments are provided (only program name), print help and exit.
	if(argc==1)
		return printConsole(CONSOLE_PRINT_HELP);

	// Process the primary input DNM file.
	E_ERROR lastError = dcx.inputDnmFile(argv[1]);
	if(lastError == E_ERROR::E_DnmRead) { // If DNM read failed, print error and exit.
        std::cerr << "Error: Failed to read DNM file: " << argv[1] << std::endl;
        return printConsole(static_cast<int>(lastError));
    }

	// Attempt to load INI file from the executable's directory.
	// This allows for global/default settings.
	if(static_cast<int>(lastError) < static_cast<int>(E_ERROR::E_MemoryError)) { // Proceed if no critical error yet.
		std::filesystem::path exe_fs_path(argv[0]);
        std::filesystem::path ini_path_alongside_exe = exe_fs_path.parent_path() / (exe_fs_path.stem().string() + ".ini");
        // std::cout << "Attempting to load INI from: " << ini_path_alongside_exe.string() << std::endl; // Debug
		lastError=dcx.inputIniFile(ini_path_alongside_exe.string().c_str());
        if (lastError == E_ERROR::E_IniPath || lastError == E_ERROR::E_IniRead) {
            // This is not necessarily a fatal error, an INI might be optional or located with DNM.
            // std::cout << "Info: Could not load INI from executable directory: " << ini_path_alongside_exe.string() << std::endl; // Debug
            lastError = E_ERROR::E_NotError; // Reset error if INI not found here is acceptable.
        }
	}

	// Attempt to load INI file from the input DNM file's directory.
	// Settings from this INI can override those from the executable's directory INI.
	if(static_cast<int>(lastError) < static_cast<int>(E_ERROR::E_MemoryError)) { // Proceed if no critical error yet.
		std::filesystem::path dnm_file_fs_path(argv[1]);
        std::filesystem::path ini_file_path_related_to_dnm = dnm_file_fs_path;
        ini_file_path_related_to_dnm.replace_extension(".ini");
        // std::cout << "Attempting to load INI from: " << ini_file_path_related_to_dnm.string() << std::endl; // Debug
		lastError=dcx.inputIniFile(ini_file_path_related_to_dnm.string().c_str());
        if (lastError == E_ERROR::E_IniPath || lastError == E_ERROR::E_IniRead) {
            // std::cout << "Info: Could not load INI from DNM directory: " << ini_file_path_related_to_dnm.string() << std::endl; // Debug
            lastError = E_ERROR::E_NotError; // Reset error if INI not found here is acceptable.
        }
	}

    // Finalize data after all input files are processed (applies INI settings, calculates normals, etc.).
    if(static_cast<int>(lastError) < static_cast<int>(E_ERROR::E_MemoryError)) {
        dcx.finalizeData();
    }

	// Output to X file. If an output path is specified as argv[2], it will be used by outputToXFile.
    // Otherwise, outputToXFile will generate a filename based on the input DNM file.
	if(static_cast<int>(lastError) < static_cast<int>(E_ERROR::E_MemoryError)) {
        if (argc > 2) { // Output file path is provided
            lastError = dcx.outputToXFile(argv[2]);
        } else { // No output file path provided, generate default
            lastError = dcx.outputToXFile();
        }
	}

    if (lastError != E_ERROR::E_NotError) {
        std::cerr << "Error during conversion process (code: " << static_cast<int>(lastError) << ")" << std::endl;
        return printConsole(static_cast<int>(lastError));
    }

	return static_cast<int>(lastError);
}

/**
 * @brief Prints messages to the console, such as program title, help/usage, or error information.
 *
 * @param option_val An integer code determining what to print:
 *                   CONSOLE_PRINT_TITLE (-2): Prints the program title and version.
 *                   CONSOLE_PRINT_HELP (-1): Prints usage instructions.
 *                   >= E_ERROR::E_NotError (0): Prints a generic error message corresponding to an E_ERROR code.
 * @return 0 if only title was printed or no specific error/help action.
 *         HELP_DISPLAYED_EXIT_CODE if help was printed.
 *         The error code itself if an error message was printed.
 */
int printConsole(int option_val){
	const char* projectName = "DnmConvX"; // Local constant for project name.

	if(option_val==CONSOLE_PRINT_TITLE)
	{
		std::cout << projectName <<" version 1.0\n"
			      << "Convert YS Flight DNM file format to DirectX file.\n\n";
	}

	if(option_val==CONSOLE_PRINT_HELP)
	{
		std::cout << "Usage:\n"
			      << projectName <<".exe <input file> [output file]\n\n"
			      << std::endl
			      << "  <input file> Specify the input file path.\n"
			      << "  [output file] Optionally specify the output file path.\n"
                  << "                If not provided, '.x' extension is used with input file's base name.\n\n"
			      << "Example:\n\n"
			      << "  "<< projectName <<".exe airplane.dnm\n\n"
			      << "              It will output to airplane.x\n\n"
                  << "INI Configuration:\n"
                  << "  An INI file with the same base name as the executable can be placed in the\n"
                  << "  executable's directory for global settings.\n"
                  << "  An INI file with the same base name as the input .dnm file can be placed\n"
                  << "  in the .dnm file's directory for model-specific settings (overrides global).\n";
		return HELP_DISPLAYED_EXIT_CODE;
	}

	// Check if option_val corresponds to an E_ERROR code (assuming they are non-negative).
	if(option_val >= static_cast<int>(E_ERROR::E_NotError))
	{
        // Specific error messages based on E_ERROR code
        switch(static_cast<E_ERROR>(option_val)) {
            case E_ERROR::E_IniPath:
                std::cerr << "Error: Invalid .ini file path." << std::endl;
                break;
            case E_ERROR::E_IniRead:
                std::cerr << "Error: Could not read .ini file. Check permissions or if the file exists." << std::endl;
                break;
            case E_ERROR::E_MemoryError:
                std::cerr << "Error: Memory allocation failed." << std::endl;
                break;
            case E_ERROR::E_DnmRead:
                std::cerr << "Error: Could not read .dnm file. Check path, permissions, or file integrity." << std::endl;
                break;
            case E_ERROR::E_XWrite:
                std::cerr << "Error: Could not write .x output file. Check permissions or path." << std::endl;
                break;
            default:
                std::cerr << "An unknown error occurred (code: " << option_val << ")." << std::endl;
        }
        // Common advice for all errors
        std::cerr << " Please check your input file(s) or parameters.\n";
        std::cerr << "Invalid command line? Please check the usage help:\n\n"
                  << "Try typing:\n\t\t " << projectName << ".exe /?\n\n"
                  << "Or:\n\n"
                  << "Try typing:\n\t\t " << projectName << ".exe <input file>\n\n";
		return option_val; // Return the error code.
	}
	return 0; // Default return for non-error, non-help specific prints (like title only).
}
;