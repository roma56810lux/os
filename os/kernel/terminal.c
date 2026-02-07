/**
 * kernel/terminal.c - Эмулятор терминала
 */

#include "terminal.h"
#include "framebuffer.h"
#include "keyboard.h"
#include "commands.h"
#include "gui.h"
#include <stdbool.h>
#include <string.h>

// Состояние терминала
static terminal_state_t term_state;
static bool terminal_initialized = false;

// Буферы терминала
static char terminal_buffer[TERMINAL_BUFFER_SIZE];
static char command_history[MAX_HISTORY][COMMAND_MAX_LENGTH];
static int history_count = 0;
static int history_index = -1;

// Цвета терминала
static const uint32_t TERM_BG_COLOR = 0x000000;
static const uint32_t TERM_TEXT_COLOR = 0xFFFFFF;
static const uint32_t TERM_PROMPT_COLOR = 0x00FF00;
static const uint32_t TERM_ERROR_COLOR = 0xFF0000;
static const uint32_t TERM_SUCCESS_COLOR = 0x00FF00;

// Окно терминала
static int term_window_id = -1;
static uint16_t term_x = 10;
static uint16_t term_y = 30;
static uint16_t term_width = 580;
static uint16_t term_height = 340;

/**
 * Инициализация терминала
 */
void terminal_init(void) {
    if (terminal_initialized) return;
    
    // Инициализируем состояние терминала
    memset(&term_state, 0, sizeof(terminal_state_t));
    term_state.cursor_x = 0;
    term_state.cursor_y = 0;
    term_state.scroll_offset = 0;
    term_state.input_index = 0;
    term_state.escape_mode = false;
    term_state.escape_param_count = 0;
    
    // Очищаем буферы
    memset(terminal_buffer, 0, sizeof(terminal_buffer));
    memset(command_history, 0, sizeof(command_history));
    
    // Создаем окно терминала, если оно еще не создано
    if (term_window_id == -1) {
        term_window_id = gui_create_window(50, 50, 600, 400, "Terminal", true);
    }
    
    // Устанавливаем размеры терминала внутри окна
    term_x = 10;
    term_y = 30;
    term_width = 580;
    term_height = 340;
    
    // Добавляем начальное сообщение
    terminal_print_banner();
    
    terminal_initialized = true;
    
    #ifdef DEBUG
    terminal_print_line("Terminal initialized");
    #endif
}

/**
 * Отрисовка терминала
 */
void terminal_draw(void) {
    if (!terminal_initialized || term_window_id == -1) return;
    
    gui_window_t* win = gui_get_window(term_window_id);
    if (win == NULL || !win->visible) return;
    
    // Рассчитываем абсолютные координаты
    uint16_t abs_x = win->x + term_x;
    uint16_t abs_y = win->y + term_y;
    
    // Очищаем область терминала
    framebuffer_draw_rect(abs_x, abs_y, term_width, term_height, TERM_BG_COLOR);
    
    // Выводим строки из буфера
    int line_count = term_state.cursor_y + 1;
    int start_line = 0;
    
    // Прокрутка, если слишком много строк
    if (line_count > MAX_TERMINAL_LINES) {
        start_line = line_count - MAX_TERMINAL_LINES;
    }
    
    for (int i = start_line; i < line_count && i < MAX_TERMINAL_LINES; i++) {
        uint16_t line_y = abs_y + (i - start_line) * 16;
        
        // Находим строку в буфере
        char* line_start = &terminal_buffer[i * TERMINAL_WIDTH];
        char line[TERMINAL_WIDTH + 1];
        strncpy(line, line_start, TERMINAL_WIDTH);
        line[TERMINAL_WIDTH] = '\0';
        
        // Отрисовываем строку
        framebuffer_draw_string(abs_x, line_y, line, TERM_TEXT_COLOR, TERM_BG_COLOR);
    }
    
    // Отрисовываем курсор ввода
    if (term_state.cursor_visible) {
        uint16_t cursor_abs_x = abs_x + term_state.cursor_x * 8;
        uint16_t cursor_abs_y = abs_y + (term_state.cursor_y - start_line) * 16;
        
        // Мигающий курсор
        static uint32_t last_blink = 0;
        uint32_t current_ticks = timer_get_ticks();
        
        if (current_ticks - last_blink > 500) { // 500ms
            term_state.cursor_visible = !term_state.cursor_visible;
            last_blink = current_ticks;
        }
        
        if (term_state.cursor_visible) {
            framebuffer_draw_rect(cursor_abs_x, cursor_abs_y + 14, 8, 2, TERM_TEXT_COLOR);
        }
    }
    
    // Отрисовываем приглашение, если это текущая строка
    if (term_state.show_prompt) {
        uint16_t prompt_y = abs_y + (term_state.cursor_y - start_line) * 16;
        framebuffer_draw_string(abs_x, prompt_y, term_state.prompt, TERM_PROMPT_COLOR, TERM_BG_COLOR);
    }
}

/**
 * Вывод символа в терминал
 * @param c Символ для вывода
 */
void terminal_putchar(char c) {
    if (!terminal_initialized) return;
    
    // Обработка специальных символов
    switch (c) {
        case '\n': // Новая строка
            term_state.cursor_x = 0;
            term_state.cursor_y++;
            break;
            
        case '\r': // Возврат каретки
            term_state.cursor_x = 0;
            break;
            
        case '\b': // Backspace
            if (term_state.cursor_x > 0) {
                term_state.cursor_x--;
                int index = term_state.cursor_y * TERMINAL_WIDTH + term_state.cursor_x;
                if (index >= 0 && index < TERMINAL_BUFFER_SIZE) {
                    terminal_buffer[index] = ' ';
                }
            }
            break;
            
        case '\t': // Табуляция
            term_state.cursor_x = (term_state.cursor_x + 8) & ~7;
            break;
            
        default: // Обычный символ
            if (c >= 32 && c <= 126) { // Печатные символы
                int index = term_state.cursor_y * TERMINAL_WIDTH + term_state.cursor_x;
                if (index >= 0 && index < TERMINAL_BUFFER_SIZE) {
                    terminal_buffer[index] = c;
                }
                term_state.cursor_x++;
            }
            break;
    }
    
    // Перенос строки при достижении границы
    if (term_state.cursor_x >= TERMINAL_WIDTH) {
        term_state.cursor_x = 0;
        term_state.cursor_y++;
    }
    
    // Прокрутка при заполнении экрана
    if (term_state.cursor_y >= MAX_TERMINAL_LINES) {
        // Сдвигаем строки вверх
        memmove(terminal_buffer, 
                &terminal_buffer[TERMINAL_WIDTH], 
                TERMINAL_BUFFER_SIZE - TERMINAL_WIDTH);
        
        // Очищаем последнюю строку
        memset(&terminal_buffer[(MAX_TERMINAL_LINES - 1) * TERMINAL_WIDTH], 
               ' ', TERMINAL_WIDTH);
        
        term_state.cursor_y = MAX_TERMINAL_LINES - 1;
        term_state.scroll_offset++;
    }
    
    // Обновляем отображение
    terminal_draw();
    framebuffer_swap();
}

/**
 * Вывод строки в терминал
 * @param str Строка для вывода
 */
void terminal_print(const char* str) {
    if (!terminal_initialized || str == NULL) return;
    
    while (*str) {
        terminal_putchar(*str);
        str++;
    }
}

/**
 * Вывод строки с переводом строки
 * @param str Строка для вывода
 */
void terminal_print_line(const char* str) {
    terminal_print(str);
    terminal_putchar('\n');
}

/**
 * Форматированный вывод в терминал
 * @param format Форматная строка
 * @param ... Аргументы
 */
void terminal_printf(const char* format, ...) {
    if (!terminal_initialized || format == NULL) return;
    
    char buffer[256];
    
    // Простая реализация форматирования
    // В реальной системе нужно использовать vsprintf
    va_list args;
    va_start(args, format);
    
    // Простейшая реализация - только строки и числа
    char* dst = buffer;
    const char* src = format;
    
    while (*src && dst - buffer < 255) {
        if (*src == '%') {
            src++;
            switch (*src) {
                case 's': {
                    char* str = va_arg(args, char*);
                    while (*str && dst - buffer < 255) {
                        *dst++ = *str++;
                    }
                    break;
                }
                case 'd': {
                    int num = va_arg(args, int);
                    char num_buf[32];
                    itoa(num, num_buf, 10);
                    char* num_str = num_buf;
                    while (*num_str && dst - buffer < 255) {
                        *dst++ = *num_str++;
                    }
                    break;
                }
                case 'x': {
                    int num = va_arg(args, int);
                    char num_buf[32];
                    itoa(num, num_buf, 16);
                    char* num_str = num_buf;
                    while (*num_str && dst - buffer < 255) {
                        *dst++ = *num_str++;
                    }
                    break;
                }
                default:
                    *dst++ = '%';
                    *dst++ = *src;
                    break;
            }
        } else {
            *dst++ = *src;
        }
        src++;
    }
    
    *dst = '\0';
    
    va_end(args);
    
    terminal_print(buffer);
}

/**
 * Вывод баннера при запуске терминала
 */
void terminal_print_banner(void) {
    terminal_print_line("========================================");
    terminal_print_line("      MyOS Terminal v1.0");
    terminal_print_line("========================================");
    terminal_print_line("");
}

/**
 * Отображение приглашения командной строки
 */
void terminal_print_prompt(void) {
    term_state.show_prompt = true;
    strcpy(term_state.prompt, "myos> ");
    terminal_print(term_state.prompt);
}

/**
 * Обработка ввода с клавиатуры
 */
void terminal_process_input(void) {
    if (!terminal_initialized) return;
    
    // Проверяем, есть ли символы в буфере клавиатуры
    char input_buffer[COMMAND_MAX_LENGTH];
    int len = keyboard_getline(input_buffer, sizeof(input_buffer));
    
    if (len > 0) {
        // Обрабатываем введенную строку
        terminal_process_command_input(input_buffer);
    }
}

/**
 * Обработка введенной команды
 * @param input Введенная строка
 */
void terminal_process_command_input(const char* input) {
    if (input == NULL || input[0] == '\0') return;
    
    // Добавляем команду в историю
    if (history_count < MAX_HISTORY) {
        strncpy(command_history[history_count], input, COMMAND_MAX_LENGTH - 1);
        command_history[history_count][COMMAND_MAX_LENGTH - 1] = '\0';
        history_count++;
    } else {
        // Сдвигаем историю
        for (int i = 0; i < MAX_HISTORY - 1; i++) {
            strcpy(command_history[i], command_history[i + 1]);
        }
        strncpy(command_history[MAX_HISTORY - 1], input, COMMAND_MAX_LENGTH - 1);
        command_history[MAX_HISTORY - 1][COMMAND_MAX_LENGTH - 1] = '\0';
    }
    
    history_index = history_count;
    
    // Выводим команду (если она не была выведена ранее)
    terminal_print_line(input);
    
    // Обрабатываем команду
    terminal_process_command(input);
    
    // Отображаем новое приглашение
    terminal_print_prompt();
}

/**
 * Обработка и выполнение команды
 * @param command Строка команды
 */
void terminal_process_command(const char* command) {
    if (command == NULL || command[0] == '\0') {
        return;
    }
    
    // Копируем команду для разбора
    char cmd_copy[COMMAND_MAX_LENGTH];
    strncpy(cmd_copy, command, COMMAND_MAX_LENGTH - 1);
    cmd_copy[COMMAND_MAX_LENGTH - 1] = '\0';
    
    // Разделяем команду на части
    char* argv[16];
    int argc = 0;
    
    char* token = strtok(cmd_copy, " ");
    while (token != NULL && argc < 16) {
        argv[argc++] = token;
        token = strtok(NULL, " ");
    }
    
    if (argc == 0) {
        return;
    }
    
    // Ищем и выполняем команду
    bool found = false;
    
    // Проверяем встроенные команды
    if (strcmp(argv[0], "help") == 0) {
        execute_help(argc, argv);
        found = true;
    }
    else if (strcmp(argv[0], "echo") == 0) {
        execute_echo(argc, argv);
        found = true;
    }
    else if (strcmp(argv[0], "about") == 0) {
        execute_about(argc, argv);
        found = true;
    }
    else if (strcmp(argv[0], "clear") == 0) {
        terminal_clear();
        found = true;
    }
    else if (strcmp(argv[0], "history") == 0) {
        terminal_show_history();
        found = true;
    }
    
    // Если команда не найдена
    if (!found) {
        terminal_printf("Command not found: %s\n", argv[0]);
        terminal_print_line("Type 'help' for available commands");
    }
}

/**
 * Очистка терминала
 */
void terminal_clear(void) {
    memset(terminal_buffer, ' ', TERMINAL_BUFFER_SIZE);
    term_state.cursor_x = 0;
    term_state.cursor_y = 0;
    term_state.scroll_offset = 0;
    
    terminal_draw();
    framebuffer_swap();
    
    // Перерисовываем баннер
    terminal_print_banner();
}

/**
 * Отображение истории команд
 */
void terminal_show_history(void) {
    if (history_count == 0) {
        terminal_print_line("No commands in history");
        return;
    }
    
    terminal_print_line("Command history:");
    for (int i = 0; i < history_count; i++) {
        terminal_printf("%3d: %s\n", i + 1, command_history[i]);
    }
}

/**
 * Навигация по истории команд (стрелками вверх/вниз)
 * @param direction Направление: 1 - вверх, -1 - вниз
 */
void terminal_history_navigate(int direction) {
    if (history_count == 0) return;
    
    if (direction > 0) { // Вверх
        if (history_index > 0) {
            history_index--;
        }
    } else { // Вниз
        if (history_index < history_count - 1) {
            history_index++;
        }
    }
    
    if (history_index >= 0 && history_index < history_count) {
        // Восстанавливаем команду из истории
        // (нужно очистить текущую строку и вставить команду из истории)
        // Для простоты пока не реализуем
    }
}

/**
 * Автодополнение команды (по нажатию Tab)
 */
void terminal_autocomplete(void) {
    // Простая реализация - дополнение по первым символам
    // В реальной системе нужно искать по всем командам
    
    const char* commands[] = {"help", "echo", "about", "clear", "history", NULL};
    
    // Находим совпадающие команды
    for (int i = 0; commands[i] != NULL; i++) {
        if (strncmp(term_state.input_buffer, commands[i], 
                   strlen(term_state.input_buffer)) == 0) {
            // Дополняем команду
            strcpy(term_state.input_buffer, commands[i]);
            term_state.input_index = strlen(commands[i]);
            break;
        }
    }
}

/**
 * Получение состояния терминала
 * @return Указатель на состояние терминала
 */
terminal_state_t* terminal_get_state(void) {
    return &term_state;
}

/**
 * Получение ID окна терминала
 * @return ID окна терминала
 */
int terminal_get_window_id(void) {
    return term_window_id;
}

/**
 * Тест терминала
 */
void terminal_test(void) {
    if (!terminal_initialized) {
        terminal_init();
    }
    
    #ifdef DEBUG
    terminal_print_line("=== Terminal Test ===");
    terminal_print_line("Testing terminal output...");
    
    for (int i = 0; i < 10; i++) {
        terminal_printf("Line %d: Hello from MyOS!\n", i + 1);
    }
    
    terminal_print_line("Terminal test completed");
    #endif
}

// Вспомогательные функции (уже должны быть где-то определены)
char* itoa(int value, char* str, int base) {
    static char digits[] = "0123456789abcdefghijklmnopqrstuvwxyz";
    char* ptr = str;
    char* ptr1 = str;
    char tmp_char;
    int tmp_value;
    int len = 0;
    
    // Проверяем основание
    if (base < 2 || base > 36) {
        *ptr = '\0';
        return str;
    }
    
    // Для отрицательных чисел
    if (value < 0 && base == 10) {
        *ptr++ = '-';
        value = -value;
    }
    
    // Преобразуем
    do {
        tmp_value = value;
        value /= base;
        *ptr++ = digits[tmp_value - value * base];
        len++;
    } while (value);
    
    *ptr-- = '\0';
    
    // Переворачиваем строку
    for (int i = 0; i < len / 2; i++) {
        tmp_char = *ptr;
        *ptr = *ptr1;
        *ptr1 = tmp_char;
        ptr--;
        ptr1++;
    }
    
    return str;
}