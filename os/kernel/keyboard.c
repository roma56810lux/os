/**
 * kernel/keyboard.c - Драйвер клавиатуры PS/2
 */

#include "keyboard.h"
#include "idt.h"
#include "terminal.h"
#include "gui.h"
#include <stdbool.h>
#include <string.h>

// Порты клавиатуры
#define KEYBOARD_DATA_PORT 0x60
#define KEYBOARD_STATUS_PORT 0x64
#define KEYBOARD_COMMAND_PORT 0x64

// Команды клавиатуры
#define KEYBOARD_CMD_LED 0xED
#define KEYBOARD_CMD_SET_SCANCODE 0xF0

// Буфер ввода клавиатуры
#define KEYBOARD_BUFFER_SIZE 256

static char keyboard_buffer[KEYBOARD_BUFFER_SIZE];
static uint32_t keyboard_buffer_start = 0;
static uint32_t keyboard_buffer_end = 0;
static bool keyboard_caps_lock = false;
static bool keyboard_shift_pressed = false;
static bool keyboard_ctrl_pressed = false;
static bool keyboard_alt_pressed = false;

// Таблица скан-кодов (set 2, без расширенных)
static const char keyboard_scancode_table[] = {
    0, 0, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', 0,
    0, 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', 0,
    0, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',
    0, '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0,
    '*', 0, ' '
};

// Таблица скан-кодов с Shift
static const char keyboard_shift_table[] = {
    0, 0, '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', 0,
    0, 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', 0,
    0, 'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"', '~',
    0, '|', 'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?', 0,
    '*', 0, ' '
};

// Состояния клавиш
static bool key_states[128] = {0};

/**
 * Обработчик прерывания клавиатуры (IRQ1)
 * @param regs Регистры на момент прерывания (не используется)
 */
void keyboard_handler(struct registers* regs) {
    (void)regs;
    
    // Читаем скан-код из порта данных
    uint8_t scancode = inb(KEYBOARD_DATA_PORT);
    
    // Проверяем, является ли это нажатием (0x80 - отпускание)
    bool pressed = !(scancode & 0x80);
    uint8_t keycode = scancode & 0x7F;
    
    // Обновляем состояние клавиши
    key_states[keycode] = pressed;
    
    // Обрабатываем специальные клавиши
    switch (keycode) {
        case 0x2A: // Левый Shift
        case 0x36: // Правый Shift
            keyboard_shift_pressed = pressed;
            break;
        case 0x1D: // Ctrl
            keyboard_ctrl_pressed = pressed;
            break;
        case 0x38: // Alt
            keyboard_alt_pressed = pressed;
            break;
        case 0x3A: // Caps Lock
            if (pressed) {
                keyboard_caps_lock = !keyboard_caps_lock;
                keyboard_update_leds();
            }
            break;
        case 0x0E: // Backspace
            if (pressed) {
                keyboard_backspace();
            }
            break;
        case 0x1C: // Enter
            if (pressed) {
                keyboard_enter();
            }
            break;
        case 0x0F: // Tab
            if (pressed) {
                keyboard_tab();
            }
            break;
        case 0x39: // Space
            if (pressed) {
                keyboard_add_char(' ');
            }
            break;
        default:
            // Обработка обычных клавиш
            if (pressed && keycode < sizeof(keyboard_scancode_table)) {
                char ch = 0;
                
                // Выбираем символ в зависимости от Shift и Caps Lock
                if (keyboard_shift_pressed) {
                    ch = keyboard_shift_table[keycode];
                } else {
                    ch = keyboard_scancode_table[keycode];
                    
                    // Caps Lock влияет только на буквы
                    if (keyboard_caps_lock && ch >= 'a' && ch <= 'z') {
                        ch = ch - 'a' + 'A';
                    }
                }
                
                if (ch != 0) {
                    keyboard_add_char(ch);
                }
            }
            break;
    }
    
    // Обработка Ctrl+Alt+Del (перезагрузка)
    if (keyboard_ctrl_pressed && keyboard_alt_pressed && keycode == 0x53 && pressed) {
        keyboard_reboot();
    }
}

/**
 * Добавление символа в буфер клавиатуры
 * @param ch Символ для добавления
 */
void keyboard_add_char(char ch) {
    uint32_t next_end = (keyboard_buffer_end + 1) % KEYBOARD_BUFFER_SIZE;
    
    // Проверяем, не переполнен ли буфер
    if (next_end != keyboard_buffer_start) {
        keyboard_buffer[keyboard_buffer_end] = ch;
        keyboard_buffer_end = next_end;
        
        // Отображаем символ в терминале
        terminal_putchar(ch);
    }
}

/**
 * Удаление последнего символа (Backspace)
 */
void keyboard_backspace(void) {
    if (keyboard_buffer_start != keyboard_buffer_end) {
        // Удаляем из буфера
        keyboard_buffer_end = (keyboard_buffer_end - 1 + KEYBOARD_BUFFER_SIZE) % KEYBOARD_BUFFER_SIZE;
        
        // Удаляем из терминала
        terminal_backspace();
    }
}

/**
 * Обработка нажатия Enter
 */
void keyboard_enter(void) {
    // Добавляем символ новой строки
    keyboard_add_char('\n');
    
    // Завершаем ввод команды
    terminal_process_command();
}

/**
 * Обработка нажатия Tab
 */
void keyboard_tab(void) {
    // Автодополнение команды (в будущем)
    keyboard_add_char('\t');
}

/**
 * Обновление светодиодов клавиатуры (Caps Lock, Num Lock, Scroll Lock)
 */
void keyboard_update_leds(void) {
    // Ждем, пока клавиатура будет готова принять команду
    keyboard_wait();
    
    // Отправляем команду установки светодиодов
    outb(KEYBOARD_DATA_PORT, KEYBOARD_CMD_LED);
    keyboard_wait();
    
    // Отправляем байт состояния светодиодов
    uint8_t leds = 0;
    if (keyboard_caps_lock) leds |= 0x04;
    // Можно добавить Num Lock и Scroll Lock при необходимости
    
    outb(KEYBOARD_DATA_PORT, leds);
}

/**
 * Ожидание готовности клавиатуры
 */
void keyboard_wait(void) {
    // Ждем, пока бит 1 (буфер ввода) не станет 0
    while (inb(KEYBOARD_STATUS_PORT) & 0x02) {
        // Пауза
        asm volatile("pause");
    }
}

/**
 * Перезагрузка системы (Ctrl+Alt+Del)
 */
void keyboard_reboot(void) {
    // Отправляем команду сброса через порт 0x64
    outb(KEYBOARD_COMMAND_PORT, 0xFE);
    
    // Вечный цикл (на случай, если перезагрузка не сработала)
    while (1) {
        asm volatile("hlt");
    }
}

/**
 * Получение строки из буфера клавиатуры
 * @param buffer Буфер для строки
 * @param size Размер буфера
 * @return Длина строки
 */
int keyboard_getline(char* buffer, int size) {
    if (size <= 0) return 0;
    
    int i = 0;
    while (i < size - 1) {
        // Ждем, пока появятся данные в буфере
        while (keyboard_buffer_start == keyboard_buffer_end) {
            asm volatile("hlt");
        }
        
        // Читаем символ из буфера
        char ch = keyboard_buffer[keyboard_buffer_start];
        keyboard_buffer_start = (keyboard_buffer_start + 1) % KEYBOARD_BUFFER_SIZE;
        
        // Если это конец строки, завершаем
        if (ch == '\n') {
            buffer[i] = '\0';
            return i;
        }
        
        // Добавляем символ в буфер
        buffer[i++] = ch;
    }
    
    buffer[size - 1] = '\0';
    return size - 1;
}

/**
 * Проверка, нажата ли определенная клавиша
 * @param keycode Код клавиши
 * @return true, если клавиша нажата
 */
bool keyboard_is_key_pressed(uint8_t keycode) {
    if (keycode >= 128) return false;
    return key_states[keycode];
}

/**
 * Получение состояния модификаторов
 */
bool keyboard_is_shift_pressed(void) { return keyboard_shift_pressed; }
bool keyboard_is_ctrl_pressed(void) { return keyboard_ctrl_pressed; }
bool keyboard_is_alt_pressed(void) { return keyboard_alt_pressed; }
bool keyboard_is_caps_lock(void) { return keyboard_caps_lock; }

/**
 * Инициализация клавиатуры
 */
void keyboard_init(void) {
    // Регистрируем обработчик прерывания клавиатуры
    irq_register_handler(1, keyboard_handler);
    
    // Очищаем буфер
    keyboard_buffer_start = 0;
    keyboard_buffer_end = 0;
    memset(keyboard_buffer, 0, sizeof(keyboard_buffer));
    
    // Инициализируем состояния клавиш
    memset(key_states, 0, sizeof(key_states));
    
    // Сбрасываем состояния модификаторов
    keyboard_shift_pressed = false;
    keyboard_ctrl_pressed = false;
    keyboard_alt_pressed = false;
    keyboard_caps_lock = false;
    
    // Включаем клавиатуру (сбрасываем бит отключения)
    keyboard_wait();
    outb(KEYBOARD_COMMAND_PORT, 0xAE);
    
    // Устанавливаем скан-код set 2
    keyboard_wait();
    outb(KEYBOARD_DATA_PORT, KEYBOARD_CMD_SET_SCANCODE);
    keyboard_wait();
    outb(KEYBOARD_DATA_PORT, 0x02);
    
    #ifdef DEBUG
    terminal_printf("Keyboard initialized\n");
    #endif
}

/**
 * Тест клавиатуры
 */
void keyboard_test(void) {
    #ifdef DEBUG
    terminal_printf("Keyboard test - press keys (ESC to exit)...\n");
    
    while (1) {
        // Проверяем нажатие ESC
        if (keyboard_is_key_pressed(0x01)) {
            break;
        }
        
        // Выводим состояние модификаторов
        if (keyboard_is_shift_pressed()) {
            terminal_printf("Shift pressed\n");
        }
        
        // Небольшая задержка
        timer_sleep(100);
    }
    
    terminal_printf("Keyboard test completed.\n");
    #endif
}