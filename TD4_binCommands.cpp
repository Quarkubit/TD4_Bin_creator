#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdint.h>
#include <errno.h>
#include <locale.h>
#include <windows.h>

// Структура для хранения информации о команде
typedef struct {
    const char* name;
    uint8_t opcode;
    int has_immediate;
} CommandInfo;

// Таблица команд TD4
CommandInfo commands[] = {
    {"ADD A", 0x00, 1},
    {"MOV A, B", 0x10, 0},
    {"IN A", 0x20, 0},
    {"MOV A", 0x30, 1},
    {"MOV B, A", 0x40, 0},
    {"ADD B", 0x50, 1},
    {"IN B", 0x60, 0},
    {"MOV B", 0x70, 1},
    {"OUT B", 0x90, 0},
    {"OUT", 0xB0, 1},
    {"JNC", 0xE0, 1},
    {"JMP", 0xF0, 1}
};

#define NUM_COMMANDS (sizeof(commands) / sizeof(commands[0]))

// Функция для преобразования строки в верхний регистр
void to_upper(char* str) {
    for (int i = 0; str[i]; i++) {
        str[i] = toupper(str[i]);
    }
}

// Функция для удаления пробелов из строки
void remove_spaces(char* str) {
    char* dst = str;
    for (char* src = str; *src; src++) {
        if (!isspace(*src)) {
            *dst++ = *src;
        }
    }
    *dst = '\0';
}

// Функция для разбора команды
int parse_command(char* input, uint8_t* opcode, uint8_t* immediate) {
    char clean_input[256];
    strcpy_s(clean_input, sizeof(clean_input), input);
    to_upper(clean_input);
    remove_spaces(clean_input);

    // Пробуем найти команду в таблице
    for (int i = 0; i < NUM_COMMANDS; i++) {
        char clean_cmd[256];
        strcpy_s(clean_cmd, sizeof(clean_cmd), commands[i].name);
        remove_spaces(clean_cmd);
        to_upper(clean_cmd);

        // Если команда без непосредственного значения
        if (!commands[i].has_immediate) {
            if (strcmp(clean_input, clean_cmd) == 0) {
                *opcode = commands[i].opcode;
                *immediate = 0;
                return 1;
            }
        }
        // Если команда с непосредственным значением
        else {
            // Проверяем, начинается ли ввод с названия команды
            int cmd_len = strlen(clean_cmd);
            if (strncmp(clean_input, clean_cmd, cmd_len) == 0) {
                // Извлекаем числовое значение
                char* value_str = clean_input + cmd_len;

                // Пропускаем возможные разделители
                while (*value_str && !isdigit(*value_str)) {
                    value_str++;
                }

                if (*value_str) {
                    char* endptr;
                    long value = strtol(value_str, &endptr, 0);

                    if (endptr != value_str && *endptr == '\0' && value >= 0 && value <= 15) {
                        *opcode = commands[i].opcode;
                        *immediate = value;
                        return 1;
                    }
                    else {
                        printf("Ошибка: значение должно быть между 0 и 15\n");
                        return 0;
                    }
                }
                else {
                    printf("Ошибка: отсутствует значение\n");
                    return 0;
                }
            }
        }
    }

    return 0;
}

int main() {
    // Установка русской локали и кодовой страницы консоли
    setlocale(LC_ALL, "Russian");
    SetConsoleCP(1251);
    SetConsoleOutputCP(1251);

    char filename[256];
    printf("Введите имя файла для сохранения программы: ");
    scanf_s("%255s", filename, (unsigned)_countof(filename));

    // Добавляем расширение .bin если его нет
    if (strstr(filename, ".") == NULL) {
        strcat_s(filename, sizeof(filename), ".bin");
    }

    FILE* outfile;
    errno_t err = fopen_s(&outfile, filename, "wb");
    if (err != 0 || !outfile) {
        fprintf(stderr, "Ошибка создания файла: %s\n", filename);
        return 1;
    }

    printf("Введите команды TD4. Для завершения введите \".\"\n");
    printf("Доступные команды:\n");
    for (int i = 0; i < NUM_COMMANDS; i++) {
        printf("  %s%s\n", commands[i].name, commands[i].has_immediate ? " Im" : "");
    }
    printf("\n");

    uint8_t program[16];
    int program_size = 0;
    char input[256];

    // Пропускаем оставшийся символ новой строки
    while (getchar() != '\n');

    while (1) {
        printf("> ");
        if (!fgets(input, sizeof(input), stdin)) {
            break;
        }

        // Удаляем символ новой строки
        input[strcspn(input, "\n")] = '\0';

        if (strcmp(input, ".") == 0) {
            break;
        }

        if (strlen(input) == 0) {
            continue;
        }

        uint8_t opcode, immediate;
        if (parse_command(input, &opcode, &immediate)) {
            if (program_size >= 16) {
                printf("Ошибка: программа превысила максимальный размер (16 байт)\n");
                continue;
            }

            uint8_t instruction = opcode | immediate;
            program[program_size++] = instruction;
            printf("Success: записана команда 0x%02X\n", instruction);
        }
        else {
            printf("Unknown command: %s\n", input);
        }
    }

    // Записываем программу в файл
    if (program_size > 0) {
        fwrite(program, 1, program_size, outfile);
        printf("Программа успешно сохранена в файл %s\n", filename);
        printf("Размер программы: %d байт\n", program_size);
    }
    else {
        printf("Программа пуста, файл не создан\n");
    }

    fclose(outfile);
    return 0;
}