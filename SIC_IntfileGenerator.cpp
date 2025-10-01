#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <iomanip>
#include <map>
#include <algorithm>

using namespace std;

// SIC instruction format size (in bytes)
const int INSTRUCTION_SIZE = 3;
const int WORD_SIZE = 3;
const int RESW_MULTIPLIER = 3;
const int RESB_MULTIPLIER = 1;

// Structure to store instruction information
struct Instruction {
    string mnemonic;
    int format;
};

// Initialize SIC instruction set
map<string, Instruction> createInstructionSet() {
    map<string, Instruction> instructions;
    // Format 3 instructions (standard SIC instructions)
    vector<string> format3 = {"ADD", "AND", "COMP", "DIV", "J", "JEQ", "JGT", "JLT", 
                             "JSUB", "LDA", "LDCH", "LDL", "LDX", "MUL", "OR", "RD", 
                             "RSUB", "STA", "STCH", "STL", "STSW", "STX", "SUB", "TD", 
                             "TIX", "WD"};
    
    for (const auto& instr : format3) {
        instructions[instr] = {instr, 3};
    }
    
    // Assembler directives
    instructions["START"] = {"START", 0};
    instructions["END"] = {"END", 0};
    instructions["RESW"] = {"RESW", 0};
    instructions["RESB"] = {"RESB", 0};
    instructions["WORD"] = {"WORD", 3};
    instructions["BYTE"] = {"BYTE", 1};
    
    return instructions;
}

// Calculate the size of BYTE operand
int calculateByteOperandSize(const string& operand) {
    if (operand.empty() || operand.length() < 3) return 1;
    
    char type = operand[0];
    string content = operand.substr(2, operand.length() - 3);
    
    if (type == 'C') {
        return content.length();
    } else if (type == 'X') {
        return (content.length() + 1) / 2;
    }
    return 1;
}

// Convert hex string to integer
int hexToInt(const string& hex_str) {
    return stoi(hex_str, nullptr, 16);
}

int main(int argc, char* argv[]) {
    // Check if correct number of arguments are provided
    if (argc != 3) {
        cout << "Usage: " << argv[0] << " <srcfile> <intfile>" << endl;
        return 1;
    }

    // Open source file for reading
    ifstream srcFile(argv[1]);
    if (!srcFile) {
        cout << "Error: Cannot open source file " << argv[1] << endl;
        return 1;
    }

    // Open intermediate file for writing
    ofstream intFile(argv[2]);
    if (!intFile) {
        cout << "Error: Cannot create intermediate file " << argv[2] << endl;
        srcFile.close();
        return 1;
    }

    string line;
    int lineNum = 5;  // Start line numbers from 5, incrementing by 5
    
    // Write header to intermediate file
    intFile << left << setw(10) << "Line" 
            << setw(10) << "Loc" 
            << setw(15) << "Label" 
            << setw(15) << "Opcode" 
            << setw(15) << "Operand" << endl;
    intFile << string(65, '-') << endl;

    // Initialize instruction set
    auto instructions = createInstructionSet();
    
    // Variables for location counter
    int LOCCTR = 0;
    bool programStarted = false;
    
    // Process each line of the source file
    while (getline(srcFile, line)) {
        string label, opcode, operand;
        
        // Skip empty lines and comments (lines starting with .)
        if (line.empty() || line[0] == '.') {
            lineNum += 5;
            continue;
        }

        // Create a string stream for parsing
        istringstream iss(line);
        
        // Check if the line starts with whitespace (no label)
        if (line[0] == ' ' || line[0] == '\t') {
            label = "";
            iss >> opcode;
            getline(iss, operand);
        } else {
            iss >> label >> opcode;
            getline(iss, operand);
        }

        // Trim leading/trailing whitespace from operand
        while (!operand.empty() && (operand[0] == ' ' || operand[0] == '\t'))
            operand = operand.substr(1);
        
        // Convert opcode to uppercase for case-insensitive comparison
        transform(opcode.begin(), opcode.end(), opcode.begin(), ::toupper);
        
        // Handle START directive
        if (opcode == "START") {
            if (!operand.empty()) {
                LOCCTR = hexToInt(operand);
            }
            programStarted = true;
        }
        
        // Format location counter as hexadecimal
        stringstream locHex;
        locHex << uppercase << hex << setw(4) << setfill('0') << LOCCTR;
        
        // Write to intermediate file
        intFile << left << setw(10) << lineNum 
                << setw(10) << locHex.str()
                << setw(15) << label 
                << setw(15) << opcode 
                << setw(15) << operand << endl;

        // Update location counter based on instruction type
        if (programStarted && opcode != "END") {
            if (instructions.find(opcode) != instructions.end()) {
                if (opcode == "RESW") {
                    LOCCTR += RESW_MULTIPLIER * stoi(operand);
                } else if (opcode == "RESB") {
                    LOCCTR += RESB_MULTIPLIER * stoi(operand);
                } else if (opcode == "BYTE") {
                    LOCCTR += calculateByteOperandSize(operand);
                } else if (opcode == "WORD") {
                    LOCCTR += WORD_SIZE;
                } else {
                    LOCCTR += instructions[opcode].format;
                }
            } else {
                // Assume it's a standard instruction if not found
                LOCCTR += INSTRUCTION_SIZE;
            }
        }

        lineNum += 5;
    }

    // Close files
    srcFile.close();
    intFile.close();

    cout << "Intermediate file generated successfully." << endl;
    return 0;
}