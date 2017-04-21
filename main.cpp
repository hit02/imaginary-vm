//POP 2017-01-16 projekt 2 Bagnucki Igor EiT 1 165499 Code::Blocks 16.01 gcc 4.9.3

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define FILENAME_LENGTH     256

#define EXECUTION_MODE_PRINT_ALL    0x01
#define EXECUTION_MODE_STEP         0x02
#define EXECUTION_JUST_ASSEMBLY     0x0100
#define EXECUTION_CREATE_EXAMPLE    0x0200

#define ARG_OUTPUT_FILE                 'o'
#define ARG_EXECUTION_MODE_STEP         't'
#define ARG_EXECUTION_MODE_PRINT_ALL    'a'
#define ARG_JUST_ASSEMBLY               'c'

#define INSTRUCTION_CODE_ADD 0
#define INSTRUCTION_CODE_SUB 1
#define INSTRUCTION_CODE_MUL 2
#define INSTRUCTION_CODE_DIV 3
#define INSTRUCTION_CODE_CMP 4
#define INSTRUCTION_CODE_MOV 5
#define INSTRUCTION_CODE_JMP 6
#define INSTRUCTION_CODE_LD  7
#define INSTRUCTION_CODE_IN  8
#define INSTRUCTION_CODE_OUT 9
#define INSTRUCTION_CODE_HCF 10
#define INSTRUCTION_CODE_NOP 11

#define JUMP_ALWAYS         0
#define JUMP_ZERO           1
#define JUMP_NOT_ZERO       2
#define JUMP_POSITIVE       3
#define JUMP_NEGATIVE       4
#define JUMP_NOT_NEGATIVE   5
#define JUMP_NOT_POSITIVE   6

#define ALIGMENT_DISASSEMBLY    11

typedef struct OPCODEtag {
    unsigned short int src  : 6;
    unsigned short int dest : 6;
    unsigned short int op   : 4;
    int                data;
} OPCODE, *POPCODE;

typedef struct PROGRAMtag {
    char *name;
    char *code;
} PROGRAM, *PPROGRAM;

typedef struct INSTRUCTIONtag {
    char *name;
    unsigned short int opcode : 4;
} INSTRUCTION, *PINSTRUCTION;

const PROGRAM programs[] = {
    {
        "adder",
        "in R0\r\ncmp R0, R2\r\njnz 2\r\nhcf\r\nadd R1, R0\r\nout R1\r\njmp -6"
    },
    {
        "signum",
        "in R0\r\ncmp R0, R1\r\njp 4\r\njn 6\r\nout R1\r\nhcf\r\nld R1, 1\r\nout R1\r\nhcf\r\nld R1, -1\r\nout R1\r\nhcf\r\n"
    },
    {
        "Collatz step counter",
        "in R0\r\nld R1, 1\r\nadd R5, R1\r\ncmp R0, R1\r\njnz 3\r\nout R5\r\nhcf\r\nmov R1, R0\r\nld R2, 2\r\ndiv R1, R2\r\nld R1, 0\r\ncmp R2, R1\r\njz 6\r\nld R1, 3\r\nmul R0, R1\r\nld R1, 1\r\nadd R0, R1\r\njmp -16\r\nld R1, 2\r\ndiv R0, R1\r\njmp -19\r\n"
    }
};

const INSTRUCTION instructions[] = {
    {"add", INSTRUCTION_CODE_ADD},
    {"sub", INSTRUCTION_CODE_SUB},
    {"mul", INSTRUCTION_CODE_MUL},
    {"div", INSTRUCTION_CODE_DIV},
    {"cmp", INSTRUCTION_CODE_CMP},
    {"mov", INSTRUCTION_CODE_MOV},
    {"jmp", INSTRUCTION_CODE_JMP},
    {"jz" , INSTRUCTION_CODE_JMP},
    {"jnz", INSTRUCTION_CODE_JMP},
    {"jp" , INSTRUCTION_CODE_JMP},
    {"jn" , INSTRUCTION_CODE_JMP},
    {"jnn", INSTRUCTION_CODE_JMP},
    {"jnp", INSTRUCTION_CODE_JMP},
    {"ld" , INSTRUCTION_CODE_LD },
    {"in" , INSTRUCTION_CODE_IN },
    {"out", INSTRUCTION_CODE_OUT},
    {"hcf", INSTRUCTION_CODE_HCF},
    {"nop", INSTRUCTION_CODE_NOP}
};

void exe_ivalidInstructionException();
void exe_accesViolationException();
void exe_diivideByZeroException();
void exe_decodeError();
int exe_getNextInstruction(POPCODE instruction, FILE * fin);
void dis_printDisassembledInstruction(POPCODE instr, POPCODE prev, int printNewLine);
void dis_printFlagsAndIp(unsigned int ip, int zeroFlag, int positiveFlag, int negativeFlag);
void dis_printRegister(int registerNumber, int value);
void dis_printModiviedRegisters(POPCODE opcode, int *registers);
int exe_execute(POPCODE code, unsigned int codeSize, unsigned int mode);
int exe_executeFile(FILE * fin, unsigned int mode);
int asm_getline(char **lineptr, size_t *n, FILE *stream);
void asm_throwError(char * message);
char *strToUpper(char * str);
char *strToLower(char * str);
int asm_getRegisterNumber(char * name);
void asm_errorRegisterExpected(unsigned int lineNumber);
void asm_assembleLine(POPCODE instruction, char *line, unsigned int lineNumber);
void asm_writeToFile(FILE * fout, POPCODE instructions, size_t nmemb);
void asm_assemble(FILE * fin, FILE * fout);
unsigned int init_parseArgumentsOrGetArguments(int argc, char *argv[], char *fin, char *fout, unsigned int *mode, unsigned int filenamesLength);

void exe_ivalidInstructionException()
{
    fprintf(stderr, "Ivalid instruction exception!");
    exit(1);
}

void exe_accesViolationException()
{
    fprintf(stderr, "Access violation exception!");
    exit(1);
}

void exe_diivideByZeroException()
{
    fprintf(stderr, "Divide by zero exception!");
    exit(1);
}

void exe_decodeError()
{
    fprintf(stderr, "Decode error!");
    exit(1);
}

int exe_getNextInstruction(POPCODE instruction, FILE * fin)
{
    //TODO Add buffer to speed up file reading
    unsigned short buf;
    fread(&buf, 2, 1, fin);
    if(feof(fin) || ferror(fin)) {
        return 0;
    }
    instruction->op = buf & 0x0F;
    instruction->dest = (buf >> 4) & 0x3F;
    instruction->src = (buf >> 10) & 0x3F;
    if(instruction->op == INSTRUCTION_CODE_LD || instruction->op == INSTRUCTION_CODE_JMP) {
        fread(&(instruction->data), 4, 1, fin);
        if(feof(fin) || ferror(fin)) {
            return 0;
        }
    }
    else {
        instruction->data = 0;
    }
    return 1;
}

void dis_printDisassembledInstruction(POPCODE instr, POPCODE prev, int printNewLine) {
    unsigned int amountOfInstructionCodes = sizeof(instructions) / sizeof(INSTRUCTION);
    if(prev != NULL && prev->op != INSTRUCTION_CODE_IN && printNewLine)
        putchar('\n');
    for(unsigned int i = 0; i < ALIGMENT_DISASSEMBLY; ++i) putchar(' ');
    for(unsigned int i = 0; i < amountOfInstructionCodes; ++i) {
        if(instructions[i].opcode == instr->op) {
            if(instr->op == INSTRUCTION_CODE_JMP) {
                switch(instr->dest) {
                    case JUMP_ALWAYS:
                        printf("jmp %-15i", instr->data);
                        break;
                    case JUMP_ZERO:
                        printf("jz %-16i", instr->data);
                        break;
                    case JUMP_NOT_ZERO:
                        printf("jnz %-15i", instr->data);
                        break;
                    case JUMP_POSITIVE:
                        printf("jp %-16i", instr->data);
                        break;
                    case JUMP_NEGATIVE:
                        printf("jn %-16i", instr->data);
                        break;
                    case JUMP_NOT_NEGATIVE:
                        printf("jnn %-15i", instr->data);
                        break;
                    case JUMP_NOT_POSITIVE:
                        printf("jnp %-15i", instr->data);
                        break;
                }
            }
            else {
                if(instr->op == INSTRUCTION_CODE_LD) {
                    printf("ld R%-2i , %-10i", instr->dest, instr->data);
                }
                else if(instr->op == INSTRUCTION_CODE_IN || instr->op == INSTRUCTION_CODE_OUT) {
                    printf("%-3s R%-14i", instructions[i].name, instr->dest);
                }
                else if(instr->op == INSTRUCTION_CODE_HCF || instr->op == INSTRUCTION_CODE_NOP) {
                    printf("%-19s", instructions[i].name);
                }
                else {
                    printf("%-3s R%-2i, R%-9i", instructions[i].name, instr->dest, instr->src);
                }
            }
            return;
        }
    }
}

void dis_printFlagsAndIp(unsigned int ip, int zeroFlag, int positiveFlag, int negativeFlag) {
    printf("%-10u ", ip);
    zeroFlag     ? putchar('Z') : putchar(' ');
    positiveFlag ? putchar('P') : putchar(' ');
    negativeFlag ? putchar('N') : putchar(' ');
}

void dis_printRegister(int registerNumber, int value)
{
    printf(" R%-2u = %-10i", registerNumber, value);
}

void dis_printModiviedRegisters(POPCODE opcode, int *registers) {
    if(opcode->op != INSTRUCTION_CODE_CMP &&
        opcode->op != INSTRUCTION_CODE_JMP &&
        opcode->op != INSTRUCTION_CODE_OUT &&
        opcode->op != INSTRUCTION_CODE_HCF &&
        opcode->op != INSTRUCTION_CODE_NOP) {
        dis_printRegister(opcode->dest, registers[opcode->dest]);
    }
    if(opcode->op == INSTRUCTION_CODE_DIV) {
        dis_printRegister(opcode->src, registers[opcode->src]);
    }
}

int exe_execute(POPCODE code, unsigned int codeSize, unsigned int mode)
{
    int reg[64] = {0};
    int zeroFlag = 0;
    int positiveFlag = 0;
    int negativeFlag = 0;
    int jumpOffset = 1;
    unsigned int ip = 0;
    unsigned int prevIp = 0xFFFFFFFF;
    unsigned int tmp = 0;
    while(1) {
        jumpOffset = 1;
        if(ip >= codeSize) {
            exe_accesViolationException();
        }
        if(mode & EXECUTION_MODE_PRINT_ALL) {
            dis_printDisassembledInstruction(&code[ip], prevIp == 0xFFFFFFFF ? NULL : &code[prevIp], !(mode & EXECUTION_MODE_STEP));
            dis_printFlagsAndIp(ip, zeroFlag, positiveFlag, negativeFlag);
        }
        switch(code[ip].op) {
            case INSTRUCTION_CODE_ADD:
                reg[code[ip].dest] += reg[code[ip].src];
                break;
            case INSTRUCTION_CODE_SUB:
                reg[code[ip].dest] -= reg[code[ip].src];
                break;
            case INSTRUCTION_CODE_MUL:
                reg[code[ip].dest] *= reg[code[ip].src];
                break;
            case INSTRUCTION_CODE_DIV:
                if(reg[code[ip].src] == 0) {
                    exe_diivideByZeroException();
                }
                tmp = reg[code[ip].dest] / reg[code[ip].src];
                reg[code[ip].src] = reg[code[ip].dest] % reg[code[ip].src];
                reg[code[ip].dest] = tmp;
                break;
            case INSTRUCTION_CODE_CMP:
                if(reg[code[ip].dest] - reg[code[ip].src] == 0) {
                    zeroFlag = 1;
                    negativeFlag = 0;
                    positiveFlag = 0;
                }
                else {
                    zeroFlag = 0;
                    if(reg[code[ip].dest] - reg[code[ip].src] < 0) {
                        negativeFlag = 1;
                        positiveFlag = 0;
                    }
                    else {
                        positiveFlag = 1;
                        negativeFlag = 0;
                    }
                }
                break;
            case INSTRUCTION_CODE_MOV:
                reg[code[ip].dest] = reg[code[ip].src];
                break;
            case INSTRUCTION_CODE_JMP:
                tmp = 0;
                switch(code[ip].dest) {
                    case JUMP_ALWAYS:
                        tmp = 1;
                        break;
                    case JUMP_ZERO:
                        if(zeroFlag)
                            tmp = 1;
                        break;
                    case JUMP_NOT_ZERO:
                        if(!zeroFlag)
                            tmp = 1;
                        break;
                    case JUMP_POSITIVE:
                        if(positiveFlag)
                            tmp = 1;
                        break;
                    case JUMP_NEGATIVE:
                        if(negativeFlag)
                            tmp = 1;
                        break;
                    case JUMP_NOT_NEGATIVE:
                        if(positiveFlag || zeroFlag)
                            tmp = 1;
                        break;
                    case JUMP_NOT_POSITIVE:
                        if(negativeFlag || zeroFlag)
                            tmp = 1;
                        break;
                }
                if(tmp)
                    jumpOffset = code[ip].data;
                break;
            case INSTRUCTION_CODE_LD:
                reg[code[ip].dest] = code[ip].data;
                break;
            case INSTRUCTION_CODE_IN:
                if(EXECUTION_MODE_PRINT_ALL & mode)
                    putchar('\r');
                else
                    putchar('\n');
                scanf("%i", &reg[code[ip].dest]);
                //scanf puts to stdout \n :(
                //registers must be aligned
                printf("%-44c", ' ');
                break;
            case INSTRUCTION_CODE_OUT:
                putchar('\r');
                printf("%i", reg[code[ip].dest]);
                break;
            case INSTRUCTION_CODE_HCF:
                return 1;
            case INSTRUCTION_CODE_NOP:
                break;
        }
        if(mode & EXECUTION_MODE_PRINT_ALL) {
            dis_printModiviedRegisters(&code[ip], reg);
        }
        if(mode & EXECUTION_MODE_STEP) {
            getchar();
        }
        if(code[ip].op == INSTRUCTION_CODE_IN && (mode & EXECUTION_MODE_PRINT_ALL)) {
            putchar('\n');
        }
        prevIp = ip;
        ip += jumpOffset;
    }
}

int exe_executeFile(FILE * fin, unsigned int mode)
{
    size_t buffSize = 4096 * sizeof(OPCODE);
    unsigned int codeSize = 0;
    POPCODE code = (POPCODE)malloc(buffSize * sizeof(OPCODE));
    if(NULL == code)
        return 0;
    for(codeSize = 0; exe_getNextInstruction(&code[codeSize], fin); ++codeSize) {
        if(codeSize >= buffSize - 1) {
            code = (POPCODE)realloc(code, buffSize + 4096 * sizeof(OPCODE));
            if(NULL == code) {
                //free(previous_code) is not required bc system will free it on exit
                puts("Not enough memory!");
                return 0;
            }
        }
    }
    exe_execute(code, codeSize, mode);
    free(code);
    return 1;
}

int asm_getline(char **lineptr, size_t *n, FILE *stream)
{
    char *bufptr = NULL;
    char *p = bufptr;
    size_t size;
    int c;
    const size_t allocationSize = 128;

    if (lineptr == NULL) {
        return -1;
    }
    if (stream == NULL) {
        return -1;
    }
    if (n == NULL) {
        return -1;
    }
    bufptr = *lineptr;
    size = *n;
    c = fgetc(stream);
    if (c == EOF) {
        return -1;
    }
    if (bufptr == NULL) {
        bufptr = (char *)malloc(allocationSize);
        if (bufptr == NULL) {
            return -1;
        }
        size = allocationSize;
    }
    p = bufptr;
    while(c != EOF) {
        if ((size_t)(p - bufptr) > (size - 1)) {
            size = size + allocationSize;
            bufptr = (char *)realloc(bufptr, size);
            if (bufptr == NULL) {
                return -1;
            }
        }
        *p++ = c;
        if (c == '\n') {
            break;
        }
        c = fgetc(stream);
    }
    *p++ = '\0';
    *lineptr = bufptr;
    *n = size;
    return p - bufptr - 1;
}

void asm_throwError(char * message)
{
    fprintf(stderr, "%s", message);
    exit(1);
}

char *strToUpper(char * str)
{
    for(unsigned int i = 0; str[i]; ++i) {
        str[i] = toupper(str[i]);
    }
    return str;
}

char *strToLower(char * str)
{
    for(unsigned int i = 0; str[i]; ++i) {
        str[i] = tolower(str[i]);
    }
    return str;
}

int asm_getRegisterNumber(char * name)
{
    unsigned int offset = 1;
    int registerNumber = 0;
    //Sereach for first digit
    while(!isdigit(name[offset]) && offset < strlen(name)) ++offset;
    if(NULL == name || strlen(name) < 1 + offset || toupper(name[-1 + offset]) != 'R' || !isdigit(name[offset]) || offset == strlen(name)) {
        return -1;
    }
    registerNumber = atoi(name + offset);
    return (registerNumber > 63 || registerNumber < 0) ? -1 : registerNumber;
}

void asm_errorRegisterExpected(unsigned int lineNumber)
{
    char errorMessage[64];
    sprintf(errorMessage, "%u: error: register expected", lineNumber + 1);
    asm_throwError(errorMessage);
}

void asm_assembleLine(POPCODE instruction, char *line, unsigned int lineNumber)
{
    unsigned int amountOfInstructionCodes = sizeof(instructions) / sizeof(INSTRUCTION);
    char errorMessage[256];
    char *current = NULL;
    current = strtok(line, " ;\n");
    int tmpRegisterNumber = 0;
    instruction->op   = 0;
    instruction->src  = 0;
    instruction->dest = 0;
    instruction->data = 0;
    if(NULL == current) {
        sprintf(errorMessage, "%u: error: token incorrectly ended", lineNumber + 1);
        asm_throwError(errorMessage);
    }
    for(unsigned int i = 0; i < (amountOfInstructionCodes); ++i) {
        if(!strcmp(strToLower(line), instructions[i].name)) {
            instruction->op = instructions[i].opcode;
            switch(instruction->op) {
                case INSTRUCTION_CODE_ADD:
                case INSTRUCTION_CODE_SUB:
                case INSTRUCTION_CODE_MUL:
                case INSTRUCTION_CODE_DIV:
                case INSTRUCTION_CODE_CMP:
                case INSTRUCTION_CODE_MOV:
                    current = strtok(NULL, ",");
                    if(NULL == current || ((tmpRegisterNumber = asm_getRegisterNumber(current)) == -1)) {
                        asm_errorRegisterExpected(lineNumber);
                        return;
                    }
                    instruction->dest = tmpRegisterNumber;
                    current = strtok(NULL, " ;\n");
                    if(NULL == current || ((tmpRegisterNumber = asm_getRegisterNumber(current)) == -1)) {
                        asm_errorRegisterExpected(lineNumber);
                        return;
                    }
                    instruction->src = tmpRegisterNumber;
                    break;
                case INSTRUCTION_CODE_IN :
                case INSTRUCTION_CODE_OUT:
                    current = strtok(NULL, " ;\n");
                    if(NULL == current || ((tmpRegisterNumber = asm_getRegisterNumber(current)) == -1)) {
                        asm_errorRegisterExpected(lineNumber);
                        return;
                    }
                    instruction->dest = tmpRegisterNumber;
                    break;
                case INSTRUCTION_CODE_JMP:
                    //check which instruction it is and write informaation to dest
                    if(!strcmp(strToLower(line), "jmp")) instruction->dest = JUMP_ALWAYS;
                    if(!strcmp(strToLower(line), "jz" )) instruction->dest = JUMP_ZERO;
                    if(!strcmp(strToLower(line), "jnz")) instruction->dest = JUMP_NOT_ZERO;
                    if(!strcmp(strToLower(line), "jp" )) instruction->dest = JUMP_POSITIVE;
                    if(!strcmp(strToLower(line), "jn" )) instruction->dest = JUMP_NEGATIVE;
                    if(!strcmp(strToLower(line), "jnn")) instruction->dest = JUMP_NOT_NEGATIVE;
                    if(!strcmp(strToLower(line), "jnp")) instruction->dest = JUMP_NOT_POSITIVE;
                    instruction->data = atoi(strtok(NULL, "\n"));
                    break;
                case INSTRUCTION_CODE_LD :
                    current = strtok(NULL, " ;\n");
                    if(NULL == current || ((tmpRegisterNumber = asm_getRegisterNumber(current)) == -1)) {
                        asm_errorRegisterExpected(lineNumber);
                        return;
                    }
                    instruction->dest = tmpRegisterNumber;
                    instruction->data = atoi(strtok(NULL, "\n"));
                    break;
                case INSTRUCTION_CODE_HCF:
                case INSTRUCTION_CODE_NOP:
                    break;
            }
            break;
        }
        else if(i == amountOfInstructionCodes - 1){
            sprintf(errorMessage, "%u: error: unknown '%s' token", lineNumber + 1, line);
            asm_throwError(errorMessage);
            return;
        }
    }
}

void asm_writeToFile(FILE * fout, POPCODE instructions, size_t nmemb)
{
    unsigned short tmp = 0;
    //TODO Flush buffer and everything
    for(size_t i = 0; i < nmemb; ++i) {
        tmp = (((unsigned short)instructions[i].op) | (((unsigned short)instructions[i].dest) << 4) | (((unsigned short)instructions[i].src) << 10));
        switch(instructions[i].op) {
            case INSTRUCTION_CODE_JMP:
            case INSTRUCTION_CODE_LD :
                fwrite(&tmp, 1, 2, fout);
                fwrite(&instructions[i].data, 1, 4, fout);
                break;
            default:
                fwrite(&tmp, 2, 1, fout);
                break;

        }
    }
}

void asm_assemble(FILE * fin, FILE * fout)
{
    const unsigned int buffSize = 2048;
    OPCODE instructions[buffSize];
    size_t lineSize = 0;
    char *line = NULL;
    unsigned int lineNumber = 0;
    unsigned int offset = 0;

    while(-1 != asm_getline(&line, &lineSize, fin)) {
        for(offset = 0; offset < lineSize && isspace(line[offset]); ++offset);
        if(line[offset] == ';' || line[offset] == '\0')
            continue;
        asm_assembleLine(&instructions[lineNumber % buffSize], line + offset, lineNumber);
        ++lineNumber;
        if(lineNumber % buffSize == 0) {
            asm_writeToFile(fout, instructions, buffSize);
        }
    }
    asm_writeToFile(fout, instructions, lineNumber % buffSize);
    free(line);
}

unsigned int init_parseArgumentsOrGetArguments(int argc, char *argv[], char *fin, char *fout, unsigned int *mode, unsigned int filenamesLength)
{
    *mode = 0;
    int tmp = 0;
    unsigned int example = 0;
    for(int i = 0; i < argc; ++i) {
        if('-' == argv[i][0] && 2 == strlen(argv[i])) {
            switch(argv[i][1]) {
                case ARG_EXECUTION_MODE_STEP:
                    *mode = EXECUTION_MODE_STEP | EXECUTION_MODE_PRINT_ALL;
                    break;
                case ARG_EXECUTION_MODE_PRINT_ALL:
                    *mode = EXECUTION_MODE_PRINT_ALL;
                    break;
                case ARG_JUST_ASSEMBLY:
                    *mode = EXECUTION_JUST_ASSEMBLY;
                    break;
                case ARG_OUTPUT_FILE:
                    ++i;
                    if(i < argc && strlen(argv[i]) < filenamesLength) {
                        strcpy(fout, argv[i]);
                    }
                    break;
            }
        }
        else {
            ++i;
            if(i < argc && strlen(argv[i]) < filenamesLength) {
                strcpy(fin, argv[i]);
            }
        }            
    }
    if(1 == argc) {
        puts("[0] - assemble file");
        puts("[1] - create example source file");
        puts("[2] - execute in silent mode");
        puts("[3] - execute in print mode");
        puts("[4] - execute in step mode");
        printf("choose: ");
        scanf("%i", &tmp);
        switch(tmp) {
            case 0:
                *mode = EXECUTION_JUST_ASSEMBLY;
                break;
            case 1:
                *mode = EXECUTION_CREATE_EXAMPLE;
                for(unsigned int i = 0; i < sizeof(programs) / sizeof(PROGRAM); ++i) {
                    printf("[%u] - %s\n", i, programs[i].name);
                }
                printf("choose: ");
                scanf("%i", &example);
                if(example >= sizeof(programs) / sizeof(PROGRAM)) {
                    puts("Invalid input. Exiting.");
                    exit(1);
                }
                break;
            case 2:
                *mode = 0;
                break;
            case 3:
                *mode = EXECUTION_MODE_PRINT_ALL;
                break;
            case 4:
                *mode = EXECUTION_MODE_STEP | EXECUTION_MODE_PRINT_ALL;
                break;
            default:
                puts("Invalid input. Exiting.");
                exit(1);
        }
        if(!(*mode & EXECUTION_CREATE_EXAMPLE)) {
            printf("input file name: ");
            scanf("%256s", fin);
        }
        if(*mode & (EXECUTION_JUST_ASSEMBLY | EXECUTION_CREATE_EXAMPLE)) {
            printf("output file name: ");
            scanf("%256s", fout);
        }
    }
    return example;
}

int main(int argc, char *argv[])
{
    char finName[FILENAME_LENGTH] = {};
    char foutName[FILENAME_LENGTH] = {};
    FILE * fin = NULL;
    FILE * fout = NULL;
    unsigned int mode = 0;
    unsigned int example = 0;
    example = init_parseArgumentsOrGetArguments(argc, argv, finName, foutName, &mode, FILENAME_LENGTH);
    if(!(mode & EXECUTION_CREATE_EXAMPLE) && (NULL == (fin = fopen(finName, "rt")))) {
        puts("ERROR: Can't open input file.");
        return 1;
    }
    if((mode & (EXECUTION_JUST_ASSEMBLY | EXECUTION_CREATE_EXAMPLE)) && (NULL == (fout = fopen(foutName, "wb")))) {
        fclose(fin);
        puts("ERROR: Can't open output file.");
        return 1;
    }
    if(mode & EXECUTION_CREATE_EXAMPLE) {
        fprintf(fout, "%s", programs[example].code);
    }
    else if(mode & EXECUTION_JUST_ASSEMBLY) {
        asm_assemble(fin, fout);
    }
    else {
        exe_executeFile(fin, mode);
    }
    return 0;
}
