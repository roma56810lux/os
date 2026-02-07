/**
 * kernel/commands.c - Встроенные команды терминала
 */

#include "commands.h"
#include "terminal.h"
#include "framebuffer.h"
#include "timer.h"
#include "gui.h"
#include "mouse.h"
#include <string.h>

// Структура для хранения информации о команде
typedef struct {
    const char* name;
    const char* description;
    void (*execute)(int argc, char** argv);
} command_t;

// Прототипы функций команд
static void cmd_help(int argc, char** argv);
static void cmd_echo(int argc, char** argv);
static void cmd_about(int argc, char** argv);
static void cmd_clear(int argc, char** argv);
static void cmd_time(int argc, char** argv);
static void cmd_date(int argc, char** argv);
static void cmd_reboot(int argc, char** argv);
static void cmd_shutdown(int argc, char** argv);
static void cmd_gui(int argc, char** argv);
static void cmd_mouse(int argc, char** argv);
static void cmd_color(int argc, char** argv);
static void cmd_draw(int argc, char** argv);
static void cmd_mem(int argc, char** argv);

// Таблица встроенных команд
static const command_t builtin_commands[] = {
    {"help",     "Show this help message", cmd_help},
    {"echo",     "Print arguments to the terminal", cmd_echo},
    {"about",    "Show information about the OS", cmd_about},
    {"clear",    "Clear the terminal screen", cmd_clear},
    {"time",     "Show current system time", cmd_time},
    {"date",     "Show current system date", cmd_date},
    {"reboot",   "Reboot the system", cmd_reboot},
    {"shutdown", "Shutdown the system", cmd_shutdown},
    {"gui",      "GUI control commands", cmd_gui},
    {"mouse",    "Show mouse status", cmd_mouse},
    {"color",    "Change terminal colors", cmd_color},
    {"draw",     "Draw graphics in terminal", cmd_draw},
    {"mem",      "Show memory information", cmd_mem},
    {NULL, NULL, NULL} // Конец таблицы
};

// Глобальные переменные
static bool commands_initialized = false;
static uint32_t terminal_bg_color = 0x000000;
static uint32_t terminal_text_color = 0xFFFFFF;

/**
 * Инициализация системы команд
 */
void commands_init(void) {
    if (commands_initialized) return;
    
    commands_initialized = true;
    
    #ifdef DEBUG
    terminal_print_line("Command system initialized");
    #endif
}

/**
 * Выполнение команды help
 */
void execute_help(int argc, char** argv) {
    cmd_help(argc, argv);
}

/**
 * Выполнение команды echo
 */
void execute_echo(int argc, char** argv) {
    cmd_echo(argc, argv);
}

/**
 * Выполнение команды about
 */
void execute_about(int argc, char** argv) {
    cmd_about(argc, argv);
}

/**
 * Поиск команды по имени
 * @param name Имя команды
 * @return Указатель на структуру команды или NULL
 */
static const command_t* find_command(const char* name) {
    if (name == NULL) return NULL;
    
    for (int i = 0; builtin_commands[i].name != NULL; i++) {
        if (strcmp(builtin_commands[i].name, name) == 0) {
            return &builtin_commands[i];
        }
    }
    
    return NULL;
}

/**
 * Выполнение команды
 * @param name Имя команды
 * @param argc Количество аргументов
 * @param argv Массив аргументов
 * @return true, если команда найдена и выполнена
 */
bool execute_command(const char* name, int argc, char** argv) {
    const command_t* cmd = find_command(name);
    
    if (cmd == NULL) {
        return false;
    }
    
    cmd->execute(argc, argv);
    return true;
}

/**
 * Получение списка доступных команд
 */
void get_command_list(char* buffer, int size) {
    if (buffer == NULL || size <= 0) return;
    
    int pos = 0;
    
    for (int i = 0; builtin_commands[i].name != NULL; i++) {
        int len = snprintf(buffer + pos, size - pos, "  %-10s - %s\n", 
                          builtin_commands[i].name, builtin_commands[i].description);
        pos += len;
        
        if (pos >= size - 1) break;
    }
    
    if (pos < size) {
        buffer[pos] = '\0';
    } else {
        buffer[size - 1] = '\0';
    }
}

/**
 * Команда: help - вывод списка команд
 */
static void cmd_help(int argc, char** argv) {
    (void)argc; // Не используется
    (void)argv; // Не используется
    
    terminal_print_line("Available commands:");
    terminal_print_line("");
    
    for (int i = 0; builtin_commands[i].name != NULL; i++) {
        terminal_printf("  %-10s - %s\n", 
                       builtin_commands[i].name, 
                       builtin_commands[i].description);
    }
    
    terminal_print_line("");
    terminal_print_line("Type 'help <command>' for more information");
}

/**
 * Команда: echo - вывод текста
 */
static void cmd_echo(int argc, char** argv) {
    // Проверяем флаги
    bool no_newline = false;
    bool enable_escapes = false;
    
    int start_arg = 1;
    
    // Обрабатываем опции
    for (int i = 1; i < argc; i++) {
        if (argv[i][0] == '-') {
            if (strcmp(argv[i], "-n") == 0) {
                no_newline = true;
                start_arg++;
            } else if (strcmp(argv[i], "-e") == 0) {
                enable_escapes = true;
                start_arg++;
            }
        }
    }
    
    // Выводим аргументы
    for (int i = start_arg; i < argc; i++) {
        if (enable_escapes) {
            // Простая обработка escape-последовательностей
            char* str = argv[i];
            while (*str) {
                if (*str == '\\') {
                    str++;
                    switch (*str) {
                        case 'n': terminal_putchar('\n'); break;
                        case 't': terminal_putchar('\t'); break;
                        case 'r': terminal_putchar('\r'); break;
                        case 'b': terminal_putchar('\b'); break;
                        case '\\': terminal_putchar('\\'); break;
                        default: terminal_putchar(*str); break;
                    }
                } else {
                    terminal_putchar(*str);
                }
                str++;
            }
        } else {
            terminal_print(argv[i]);
        }
        
        if (i < argc - 1) {
            terminal_putchar(' ');
        }
    }
    
    if (!no_newline) {
        terminal_putchar('\n');
    }
}

/**
 * Команда: about - информация об ОС
 */
static void cmd_about(int argc, char** argv) {
    (void)argc; // Не используется
    (void)argv; // Не используется
    
    terminal_print_line("========================================");
    terminal_print_line("           MyOS v1.0");
    terminal_print_line("========================================");
    terminal_print_line("A simple operating system for learning");
    terminal_print_line("");
    terminal_print_line("Features:");
    terminal_print_line("  • 32-bit protected mode");
    terminal_print_line("  • VESA graphics (1024x768x32)");
    terminal_print_line("  • GUI with windows and mouse");
    terminal_print_line("  • Terminal with command line");
    terminal_print_line("  • PS/2 keyboard and mouse support");
    terminal_print_line("  • Basic command set");
    terminal_print_line("");
    terminal_print_line("Author: Student Developer");
    terminal_print_line("License: Educational Use Only");
    terminal_print_line("========================================");
}

/**
 * Команда: clear - очистка экрана
 */
static void cmd_clear(int argc, char** argv) {
    (void)argc; // Не используется
    (void)argv; // Не используется
    
    terminal_clear();
}

/**
 * Команда: time - вывод текущего времени
 */
static void cmd_time(int argc, char** argv) {
    (void)argc; // Не используется
    (void)argv; // Не используется
    
    char time_str[16];
    timer_get_time_string(time_str);
    
    terminal_printf("Current time: %s\n", time_str);
    terminal_printf("System uptime: %d seconds\n", timer_get_seconds());
    terminal_printf("Timer ticks: %d\n", timer_get_ticks());
}

/**
 * Команда: date - вывод даты (заглушка)
 */
static void cmd_date(int argc, char** argv) {
    (void)argc; // Не используется
    (void)argv; // Не используется
    
    // В реальной системе здесь будет чтение времени из RTC
    terminal_print_line("System date: 2024-01-01");
    terminal_print_line("(RTC not implemented)");
}

/**
 * Команда: reboot - перезагрузка системы
 */
static void cmd_reboot(int argc, char** argv) {
    (void)argc; // Не используется
    (void)argv; // Не используется
    
    terminal_print_line("Rebooting system...");
    
    // Задержка для отображения сообщения
    timer_sleep(1000);
    
    // Перезагрузка через клавиатурный контроллер
    outb(0x64, 0xFE);
    
    // Если перезагрузка не сработала
    terminal_print_line("Reboot failed. Halting.");
    while (1) {
        asm volatile("hlt");
    }
}

/**
 * Команда: shutdown - выключение системы (заглушка)
 */
static void cmd_shutdown(int argc, char** argv) {
    (void)argc; // Не используется
    (void)argv; // Не используется
    
    terminal_print_line("Shutdown not implemented in QEMU.");
    terminal_print_line("Use Ctrl+Alt+Del to reboot.");
}

/**
 * Команда: gui - управление графическим интерфейсом
 */
static void cmd_gui(int argc, char** argv) {
    if (argc < 2) {
        terminal_print_line("Usage: gui <command>");
        terminal_print_line("Commands: test, hide, show, color");
        return;
    }
    
    if (strcmp(argv[1], "test") == 0) {
        terminal_print_line("Starting GUI test...");
        gui_test();
        terminal_print_line("GUI test completed");
    }
    else if (strcmp(argv[1], "hide") == 0) {
        terminal_print_line("Hiding GUI...");
        // Здесь должна быть реализация скрытия GUI
        terminal_print_line("(Not implemented)");
    }
    else if (strcmp(argv[1], "show") == 0) {
        terminal_print_line("Showing GUI...");
        gui_draw_desktop();
        framebuffer_swap();
        terminal_print_line("GUI shown");
    }
    else if (strcmp(argv[1], "color") == 0) {
        if (argc < 3) {
            terminal_print_line("Usage: gui color <RRGGBB>");
            return;
        }
        
        // Парсим цвет
        uint32_t color = 0;
        sscanf(argv[2], "%x", &color);
        
        terminal_printf("Setting desktop color to 0x%06X\n", color);
        // Здесь должна быть установка цвета
        terminal_print_line("(Not implemented)");
    }
    else {
        terminal_printf("Unknown GUI command: %s\n", argv[1]);
    }
}

/**
 * Команда: mouse - информация о мыши
 */
static void cmd_mouse(int argc, char** argv) {
    (void)argc; // Не используется
    (void)argv; // Не используется
    
    int mouse_x = mouse_get_x();
    int mouse_y = mouse_get_y();
    int mouse_z = mouse_get_z();
    
    terminal_printf("Mouse position: X=%d, Y=%d, Z=%d\n", mouse_x, mouse_y, mouse_z);
    terminal_printf("Mouse buttons: L=%d, R=%d, M=%d\n",
                   mouse_is_left_pressed(),
                   mouse_is_right_pressed(),
                   mouse_is_middle_pressed());
    
    // Проверяем, доступна ли мышь
    if (mouse_x == 0 && mouse_y == 0) {
        terminal_print_line("Warning: Mouse may not be initialized");
    }
}

/**
 * Команда: color - изменение цветов терминала
 */
static void cmd_color(int argc, char** argv) {
    if (argc < 2) {
        terminal_printf("Current colors: text=0x%06X, bg=0x%06X\n",
                       terminal_text_color, terminal_bg_color);
        terminal_print_line("Usage: color <text> <bg>");
        terminal_print_line("Example: color FFFFFF 000000");
        return;
    }
    
    if (argc >= 2) {
        uint32_t text_color = 0;
        sscanf(argv[1], "%x", &text_color);
        terminal_text_color = text_color;
        terminal_printf("Text color set to 0x%06X\n", text_color);
    }
    
    if (argc >= 3) {
        uint32_t bg_color = 0;
        sscanf(argv[2], "%x", &bg_color);
        terminal_bg_color = bg_color;
        terminal_printf("Background color set to 0x%06X\n", bg_color);
    }
    
    // Здесь должен быть код для применения цветов
    terminal_print_line("(Color change requires terminal redraw)");
}

/**
 * Команда: draw - рисование графики через терминал
 */
static void cmd_draw(int argc, char** argv) {
    if (argc < 2) {
        terminal_print_line("Usage: draw <command>");
        terminal_print_line("Commands: line, rect, circle, test");
        return;
    }
    
    if (strcmp(argv[1], "test") == 0) {
        terminal_print_line("Drawing test pattern...");
        
        // Рисуем тестовую графику
        framebuffer_draw_rect(100, 100, 200, 150, 0xFF0000);
        framebuffer_draw_rect(150, 125, 100, 100, 0x00FF00);
        framebuffer_draw_line(100, 300, 300, 400, 0x0000FF);
        framebuffer_draw_circle(400, 300, 50, 0xFFFF00);
        
        framebuffer_swap();
        terminal_print_line("Test pattern drawn");
    }
    else if (strcmp(argv[1], "rect") == 0) {
        if (argc < 7) {
            terminal_print_line("Usage: draw rect <x> <y> <w> <h> <color>");
            return;
        }
        
        int x = atoi(argv[2]);
        int y = atoi(argv[3]);
        int w = atoi(argv[4]);
        int h = atoi(argv[5]);
        uint32_t color = 0;
        sscanf(argv[6], "%x", &color);
        
        framebuffer_draw_rect(x, y, w, h, color);
        framebuffer_swap();
        terminal_printf("Rectangle drawn at (%d,%d) size %dx%d color 0x%06X\n",
                       x, y, w, h, color);
    }
    else if (strcmp(argv[1], "line") == 0) {
        if (argc < 7) {
            terminal_print_line("Usage: draw line <x1> <y1> <x2> <y2> <color>");
            return;
        }
        
        int x1 = atoi(argv[2]);
        int y1 = atoi(argv[3]);
        int x2 = atoi(argv[4]);
        int y2 = atoi(argv[5]);
        uint32_t color = 0;
        sscanf(argv[6], "%x", &color);
        
        framebuffer_draw_line(x1, y1, x2, y2, color);
        framebuffer_swap();
        terminal_printf("Line drawn from (%d,%d) to (%d,%d) color 0x%06X\n",
                       x1, y1, x2, y2, color);
    }
    else {
        terminal_printf("Unknown draw command: %s\n", argv[1]);
    }
}

/**
 * Команда: mem - информация о памяти (заглушка)
 */
static void cmd_mem(int argc, char** argv) {
    (void)argc; // Не используется
    (void)argv; // Не используется
    
    terminal_print_line("Memory Information:");
    terminal_print_line("  Total: 64 MB (simulated)");
    terminal_print_line("  Used: ~4 MB");
    terminal_print_line("  Free: ~60 MB");
    terminal_print_line("");
    terminal_print_line("Memory map not implemented");
}

/**
 * Вспомогательная функция: преобразование строки в число
 */
static int atoi(const char* str) {
    int result = 0;
    int sign = 1;
    
    if (*str == '-') {
        sign = -1;
        str++;
    }
    
    while (*str >= '0' && *str <= '9') {
        result = result * 10 + (*str - '0');
        str++;
    }
    
    return result * sign;
}

/**
 * Вспомогательная функция: форматированный вывод
 */
static int snprintf(char* str, size_t size, const char* format, ...) {
    // Простейшая реализация для компиляции
    // В реальной системе должна быть полная реализация
    (void)size;
    
    va_list args;
    va_start(args, format);
    
    // Очень простая реализация - только строки
    char* dst = str;
    const char* src = format;
    
    while (*src && size > 1) {
        if (*src == '%') {
            src++;
            if (*src == 's') {
                char* arg = va_arg(args, char*);
                while (*arg && size > 1) {
                    *dst++ = *arg++;
                    size--;
                }
            }
        } else {
            *dst++ = *src;
            size--;
        }
        src++;
    }
    
    *dst = '\0';
    va_end(args);
    
    return dst - str;
}

/**
 * Тест системы команд
 */
void commands_test(void) {
    terminal_print_line("=== Command System Test ===");
    
    // Тестируем команду help
    char* test_args[] = {"help"};
    cmd_help(1, test_args);
    
    // Тестируем команду echo
    char* echo_args[] = {"echo", "Hello,", "World!"};
    cmd_echo(3, echo_args);
    
    terminal_print_line("Command system test completed");
}