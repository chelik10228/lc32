#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>

#define MEMORY_SIZE 16777216

#define R1(r8) (r8 % 4)
#define R2(r8) ((r8 >> 2) % 4)
#define R3(r8) ((r8 >> 4) % 4)
#define R4(r8) ((r8 >> 6) % 4)

#define SETF(f, bf_addr) (f |= (1 << bf_addr))
#define RESETF(f, bf_addr) (f &= ~(1 << bf_addr))

// Названия регистров
enum {
  A = 0b00,
  B = 0b01,
  C = 0b10,
  D = 0b11
};
char* regnames[5] = {"A", "B", "C", "D"};
// Названия флагов
enum {
  ZF = 0,
  NF = 1
};
typedef struct {
  uint32_t reg[4];
  uint8_t  F;  // Флаги
  uint32_t PC; // Указатель на инструкцию
  uint32_t SP; // Указатель на вершину стека
  uint8_t* mem;
} CPU;

// Прочитать 8-битное число (byte) и инкрементировать PC
uint8_t fetch_byte(CPU* cpu) {
  uint8_t byte = cpu->mem[cpu->PC];
  cpu->PC++;
  return byte;
}

// Прочитать 16-битное число (word) и инкрементировать PC
uint16_t fetch_word(CPU* cpu) {
  uint16_t word = cpu->mem[cpu->PC] + (cpu->mem[cpu->PC+1] << 8);
  cpu->PC += 2;
  return word;
}

// Прочитать 32-битное число (dword) и инкрементировать PC
uint32_t fetch_dword(CPU* cpu) {
  uint32_t dword = cpu->mem[cpu->PC] + (cpu->mem[cpu->PC+1] << 8) + (cpu->mem[cpu->PC+2] << 16) + (cpu->mem[cpu->PC+3] << 24);
  cpu->PC += 4;
  return dword;
}

// Прочитать 16-битное число (word)
uint32_t read_word(CPU* cpu, uint32_t addr) {
  return cpu->mem[addr] + (cpu->mem[addr+1] << 8);
}

// Прочитать 32-битное число (dword)
uint32_t read_dword(CPU* cpu, uint32_t addr) {
  return cpu->mem[addr] + (cpu->mem[addr+1] << 8) + (cpu->mem[addr+2] << 16) + (cpu->mem[addr+3] << 24);
}

// Записать 16-битное число (word) в память
void write_word(CPU* cpu, uint32_t addr, uint16_t val) {
  cpu->mem[addr]   = val % 256;
  cpu->mem[addr+1] = val >> 8;
}

// Записать 32-битное число (dword) в память
void write_dword(CPU* cpu, uint32_t addr, uint32_t val) {
  cpu->mem[addr]   = val % 256;
  cpu->mem[addr+1] = (val >> 8) % 256;
  cpu->mem[addr+2] = (val >> 16) % 256;
  cpu->mem[addr+3] = (val >> 24) % 256;
}

// Записать 32-битное число (dword) на стек
void stack_push(CPU* cpu, uint32_t val) {
  cpu->SP -= 4;
  write_dword(cpu, cpu->SP, val);
}

// Получить 32-битное число (dword) со стека
uint32_t stack_pop(CPU* cpu) {
  uint32_t val = read_dword(cpu, cpu->SP);
  cpu->SP += 4;
  return val;
}

// Пропустить 8 байт
void skip(CPU* cpu) {
  cpu->PC += 8;
}

/* Инструкции процессора */
// Формат инструкций:
//   inst r8 disp8 rs8 i32
//   inst8 -- инструкция
//   r8    -- 1-4 регистра
//   disp8 -- сдвиг
//   rs8   -- длина элемента сдвига
//   i32   -- 32-битное число / адрес
// регистр указывается как 2-битное число:
//   A   00
//   B   01
//   C   10
//   D   11

// BR ($20) -- Прыжок в другой участок памяти
void BR(CPU* cpu) {
  uint8_t  r8    = fetch_byte(cpu);
  uint8_t  disp8 = fetch_byte(cpu);
  uint8_t  rs8   = fetch_byte(cpu);
  uint32_t a32   = fetch_dword(cpu);
  cpu->PC = a32 + disp8*rs8;
}

// BRC ($21) -- Прыжок в другой участок памяти с условием
void BRC(CPU* cpu) {
  uint8_t  r8    = fetch_byte(cpu);
  uint8_t  disp8 = fetch_byte(cpu);
  uint8_t  rs8   = fetch_byte(cpu);
  uint32_t a32   = fetch_dword(cpu);
  if (cpu->F & r8) cpu->PC = a32 + disp8*rs8;
}

// LD reg i32 ($40) -- загрузить 32-битное число в регистр
void LD_reg_i32(CPU* cpu) {
  uint8_t  r8    = fetch_byte(cpu);
  uint8_t  disp8 = fetch_byte(cpu);
  uint8_t  rs8   = fetch_byte(cpu);
  uint32_t i32   = fetch_dword(cpu);
  cpu->reg[R1(r8)] = i32;
}

// ADD reg i32 ($41) -- сложить 32-битное число с регистром
void ADD_reg_i32(CPU* cpu) {
  uint8_t  r8    = fetch_byte(cpu);
  uint8_t  disp8 = fetch_byte(cpu);
  uint8_t  rs8   = fetch_byte(cpu);
  uint32_t i32   = fetch_dword(cpu);
  cpu->reg[R1(r8)] += i32;
}

// SUB reg i32 ($42) -- вычесть 32-битное число из регистра
void SUB_reg_i32(CPU* cpu) {
  uint8_t  r8    = fetch_byte(cpu);
  uint8_t  disp8 = fetch_byte(cpu);
  uint8_t  rs8   = fetch_byte(cpu);
  uint32_t i32   = fetch_dword(cpu);
  cpu->reg[R1(r8)] -= i32;
}

// MUL reg i32 ($43) -- умножить 32-битное число и регистр
void MUL_reg_i32(CPU* cpu) {
  uint8_t  r8    = fetch_byte(cpu);
  uint8_t  disp8 = fetch_byte(cpu);
  uint8_t  rs8   = fetch_byte(cpu);
  uint32_t i32   = fetch_dword(cpu);
  cpu->reg[R1(r8)] *= i32;
}

// DIV reg i32 ($44) -- разделить регистр на 32-битное число
void DIV_reg_i32(CPU* cpu) {
  uint8_t  r8    = fetch_byte(cpu);
  uint8_t  disp8 = fetch_byte(cpu);
  uint8_t  rs8   = fetch_byte(cpu);
  uint32_t i32   = fetch_dword(cpu);
  cpu->reg[R1(r8)] /= i32;
}

// LD reg a32 ($50) -- загрузить 32-битное число из адреса в регистр
void LD_reg_a32(CPU* cpu) {
  uint8_t  r8    = fetch_byte(cpu);
  uint8_t  disp8 = fetch_byte(cpu);
  uint8_t  rs8   = fetch_byte(cpu);
  uint32_t a32   = fetch_dword(cpu);
  switch (rs8) {
    case 0x01: cpu->reg[R1(r8)] = cpu->mem[a32+disp8*rs8];        break; // Прочитать 1 байт
    case 0x02: cpu->reg[R1(r8)] = read_word(cpu, a32+disp8*rs8);  break; // Прочитать 2 байта
    case 0x04: cpu->reg[R1(r8)] = read_dword(cpu, a32+disp8*rs8); break; // Прочитать 4 байта
    default:   printf("lasto: error: unknown scale %d\n", rs8);   exit(1);
  }
}

// LDR reg a32 ($51) -- загрузить 32-битное число из mem[R] в регистр
void LDR_reg_a32(CPU* cpu) {
  uint8_t  r8    = fetch_byte(cpu);
  uint8_t  disp8 = fetch_byte(cpu);
  uint8_t  rs8   = fetch_byte(cpu);
  uint32_t a32   = fetch_dword(cpu);
  switch (rs8) {
    case 0x01: cpu->reg[R1(r8)] = cpu->mem[cpu->reg[R2(r8)]];        break; // Прочитать 1 байт
    case 0x02: cpu->reg[R1(r8)] = read_word(cpu, cpu->reg[R2(r8)]);  break; // Прочитать 2 байта
    case 0x04: cpu->reg[R1(r8)] = read_dword(cpu, cpu->reg[R2(r8)]); break; // Прочитать 4 байта
    default:   printf("lasto: error: unknown scale %d\n", rs8);   exit(1);
  }
}

// ST reg a32 ($58) -- загрузить регистр в 32-битный адрес
void ST_reg_a32(CPU* cpu) {
  uint8_t  r8    = fetch_byte(cpu);
  uint8_t  disp8 = fetch_byte(cpu);
  uint8_t  rs8   = fetch_byte(cpu);
  uint32_t a32   = fetch_dword(cpu);
  switch (rs8) {
    case 0x01: cpu->mem[a32+disp8*rs8] = cpu->reg[R1(r8)];        break; // Записать 1 байт
    case 0x02: write_word(cpu, a32+disp8*rs8, cpu->reg[R1(r8)]);  break; // Записать 2 байта
    case 0x04: write_dword(cpu, a32+disp8*rs8, cpu->reg[R1(r8)]); break; // Записать 4 байта
    default:   printf("lasto: error: unknown scale %d\n", rs8);   exit(1);
  }
}

// CPR reg i32 ($C0) -- сравнить регистр с 32-битным числом
void CPR_reg_i32(CPU* cpu) {
  uint8_t  r8    = fetch_byte(cpu);
  uint8_t  disp8 = fetch_byte(cpu);
  uint8_t  rs8   = fetch_byte(cpu);
  uint32_t i32   = fetch_dword(cpu);
  if (!(cpu->reg[R1(r8)] - i32))               SETF(cpu->F, ZF);
  if (((int32_t)(cpu->reg[R1(r8)] - i32)) < 0) SETF(cpu->F, NF);
}

// STP ($F0) -- завершить работу процессора
void STP(CPU* cpu) {
  skip(cpu); // Пропустить 8 байт
  exit(0);
}

// BRK ($F1) -- вызвать прерывание
void BRK(CPU* cpu) {
  uint8_t  r8    = fetch_byte(cpu);
  uint8_t  disp8 = fetch_byte(cpu);
  uint8_t  rs8   = fetch_byte(cpu);
  uint32_t i32   = fetch_dword(cpu);
  switch (i32) {
    // Вывести символ в stdout
    case 0x00000001: {
      fputc(cpu->mem[0xF00000], stdout);
      fflush(stdout);
      break;
    }
    default: printf("lasto: error: unknown break %08X\n", i32); exit(1);
  }
}

/* Конец инструкций процессора */
// Неизвестная инструкция
void ERROR(CPU* cpu) {
  printf("lasto: error: unknown instruction $%02X at %08X\n", cpu->mem[cpu->PC-1], cpu->PC);
  exit(1);
}

// Вывести регистры
void regs(CPU* cpu) {
  printf("A:  $%08X\t%010u\t%c\n", cpu->reg[A], cpu->reg[A], cpu->reg[A]);
  printf("B:  $%08X\t%010u\t%c\n", cpu->reg[B], cpu->reg[B], cpu->reg[B]);
  printf("C:  $%08X\t%010u\t%c\n", cpu->reg[C], cpu->reg[C], cpu->reg[C]);
  printf("D:  $%08X\t%010u\t%c\n", cpu->reg[D], cpu->reg[D], cpu->reg[D]);
  printf("F:   %08b\n", cpu->F);
  printf("SP: $%08X\t%010u\t%c\n", cpu->SP, cpu->SP, cpu->SP);
  printf("PC: $%08X\t%010u\t%c\n", cpu->PC, cpu->PC, cpu->PC);
}

// Выполнить 1 инструкцию
void execute(CPU* cpu) {
  uint8_t inst = fetch_byte(cpu);
  switch (inst) {
    case 0x20: BR(cpu);          break;
    case 0x21: BRC(cpu);         break;
    case 0x40: LD_reg_i32(cpu);  break;
    case 0x41: ADD_reg_i32(cpu); break;
    case 0x42: SUB_reg_i32(cpu); break;
    case 0x43: MUL_reg_i32(cpu); break;
    case 0x44: DIV_reg_i32(cpu); break;
    case 0x50: LD_reg_a32(cpu);  break;
    case 0x51: LDR_reg_a32(cpu); break;
    case 0x58: ST_reg_a32(cpu);  break;
    case 0xC0: CPR_reg_i32(cpu); break;
    case 0xF0: STP(cpu);         break;
    case 0xF1: BRK(cpu);         break;
    default: ERROR(cpu);         break;
  }
}

int main(int argc, char** argv) {
  if (argc == 1) {
    puts("lasto: error: file not given");
    exit(1);
  }
  FILE* fl = fopen(argv[1], "r");
  if (fl == NULL) {
    printf("lasto: error: file `%s` not found\n", argv[1]);
    exit(1);
  }
  CPU cpu;
  cpu.PC = 0x000000;
  cpu.SP = 0x0FFFFF;
  // Очистить регистры A,B,C,D
  for (uint8_t i = 0; i < 4; i++) cpu.reg[i] = 0x00000000;

  // Загрузить файл в память процессора
  cpu.mem = malloc(MEMORY_SIZE);
  fread(cpu.mem, 1, MEMORY_SIZE, fl);
  fclose(fl);

  for (;;) {
    // regs(&cpu);
    execute(&cpu);
    // putchar('\n');
    // getchar();
  }
  return 0;
}
