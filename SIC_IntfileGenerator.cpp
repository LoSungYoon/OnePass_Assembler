#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <iomanip>
#include <map>
#include <algorithm>

using namespace std;

// SIC 명령어 형식 크기 (바이트 단위)
const int INSTRUCTION_SIZE = 3;
const int WORD_SIZE = 3;
const int RESW_MULTIPLIER = 3;
const int RESB_MULTIPLIER = 1;

// 명령어 정보를 저장하기 위한 구조체
struct Instruction {
    string mnemonic;
    int format;
};

// SIC 명령어 집합 초기화
map<string, Instruction> createInstructionSet() {
    map<string, Instruction> instructions;
    // 포맷3 명령어 (표준 SIC 명령어)
    vector<string> format3 = {"ADD", "AND", "COMP", "DIV", "J", "JEQ", "JGT", "JLT", 
                             "JSUB", "LDA", "LDCH", "LDL", "LDX", "MUL", "OR", "RD", 
                             "RSUB", "STA", "STCH", "STL", "STSW", "STX", "SUB", "TD", 
                             "TIX", "WD"};
    
    for (const auto& instr : format3) {
        instructions[instr] = {instr, 3};
    }
    
    // 어셈블러 지시어
    instructions["START"] = {"START", 0};
    instructions["END"] = {"END", 0};
    instructions["RESW"] = {"RESW", 0};
    instructions["RESB"] = {"RESB", 0};
    instructions["WORD"] = {"WORD", 3};
    instructions["BYTE"] = {"BYTE", 1};
    
    return instructions;
}

// BYTE 연산자의 크기 계산
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

// 16진수 문자열을 정수로 변환
int hexToInt(const string& hex_str) {
    return stoi(hex_str, nullptr, 16);
}

int main(int argc, char* argv[]) {
    // 올바른 수의 명령행 인수가 제공되었는지 확인
    if (argc != 3) {
        cout << "Usage: " << argv[0] << " <srcfile> <intfile>" << endl;
        return 1;
    }

    // 읽기용 소스 파일 열기
    ifstream srcFile(argv[1]);
    if (!srcFile) {
        cout << "Error: Cannot open source file " << argv[1] << endl;
        return 1;
    }

    // 쓰기용 중간 파일 열기
    ofstream intFile(argv[2]);
    if (!intFile) {
        cout << "Error: Cannot create intermediate file " << argv[2] << endl;
        srcFile.close();
        return 1;
    }

    string line;
    int lineNum = 1;  // Start line numbers from 1, incrementing by 1
    
    // 중간 파일에 헤더 작성
    intFile << left << setw(10) << "Line" 
            << setw(10) << "Loc" 
            << setw(15) << "Label" 
            << setw(15) << "Opcode" 
            << setw(15) << "Operand" << endl;
    intFile << string(65, '-') << endl;

    // 명령어 집합 초기화
    auto instructions = createInstructionSet();
    
    // 위치 카운터 변수
    int LOCCTR = 0;
    bool programStarted = false;
    
    // 소스 파일의 각 줄 처리
    while (getline(srcFile, line)) {
        string label, opcode, operand;
        
        // 빈 줄과 주석 처리 (점으로 시작하는 줄)
        if (line.empty()) {
            // 빈 줄을 LOC 없이 중간 파일에 작성
            intFile << left << setw(10) << lineNum 
                   << setw(10) << "" 
                   << setw(15) << "" 
                   << setw(15) << "" 
                   << setw(15) << "" << endl;
            lineNum += 1;
            continue;
        }
        
        // 주석 줄 건너뛰기
        if (line[0] == '.') {
            lineNum += 1;
            continue;
        }

        // 구문 분석을 위한 문자열 스트림 생성
        istringstream iss(line);
        
        // 줄이 공백으로 시작하는지 확인 (레이블 없음)
        if (line[0] == ' ' || line[0] == '\t') {
            label = "";
            iss >> opcode;
            getline(iss, operand);
        } else {
            iss >> label >> opcode;
            getline(iss, operand);
        }

        // 연산자의 앞뒤 공백 제거
        while (!operand.empty() && (operand[0] == ' ' || operand[0] == '\t'))
            operand = operand.substr(1);
        
        // 대소문자 구분 없는 비교를 위해 opcode를 대문자로 변환
        transform(opcode.begin(), opcode.end(), opcode.begin(), ::toupper);
        
        // START 지시어 처리
        if (opcode == "START") {
            if (!operand.empty()) {
                LOCCTR = hexToInt(operand);
            }
            programStarted = true;
        }
        
        // 위치 카운터를 16진수 형식으로 변환
        stringstream locHex;
        locHex << uppercase << hex << setw(4) << setfill('0') << LOCCTR;
        
        // 중간 파일에 쓰기
        intFile << left << setw(10) << lineNum 
                << setw(10) << locHex.str()
                << setw(15) << label 
                << setw(15) << opcode 
                << setw(15) << operand << endl;

        // 명령어 타입에 따라 위치 카운터 업데이트
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
                // 찾을 수 없는 경우 표준 명령어로 가정
                LOCCTR += INSTRUCTION_SIZE;
            }
        }

        lineNum += 1;
    }

    // 파일 닫기
    srcFile.close();
    intFile.close();

    cout << "Intermediate file generated successfully." << endl;
    return 0;
}