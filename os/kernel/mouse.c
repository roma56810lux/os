/**
 * kernel/mouse.c - Драйвер мыши PS/2
 */

#include "mouse.h"
#include "idt.h"
#include "framebuffer.h"
#include "gui.h"
#include <stdbool.h>

// Порты мыши
#define MOUSE_DATA_PORT    0x60
#define MOUSE_STATUS_PORT  0x64
#define MOUSE_COMMAND_PORT 0x64

// Команды мыши
#define MOUSE_CMD_RESET          0xFF
#define MOUSE_CMD_ENABLE         0xF4
#define MOUSE_CMD_DISABLE        0xF5
#define MOUSE_CMD_SET_SAMPLE     0xF3
#define MOUSE_CMD_GET_DEVICE_ID  0xF2
#define MOUSE_CMD_SET_STREAM_MODE 0xEA
#define MOUSE_CMD_SET_REMOTE_MODE 0xF0

// Битовая маска для пакета мыши
#define MOUSE_LEFT_BUTTON   0x01
#define MOUSE_RIGHT_BUTTON  0x02
#define MOUSE_MIDDLE_BUTTON 0x04
#define MOUSE_X_SIGN        0x10
#define MOUSE_Y_SIGN        0x20
#define MOUSE_X_OVERFLOW    0x40
#define MOUSE_Y_OVERFLOW    0x80

// Глобальные переменные мыши
static int mouse_x = 400;  // Начальная позиция X (центр экрана)
static int mouse_y = 300;  // Начальная позиция Y (центр экрана)
static int mouse_z = 0;    // Положение колеса
static uint8_t mouse_buttons = 0;
static uint8_t mouse_packet[4];
static uint8_t mouse_packet_index = 0;
static bool mouse_waiting_for_ack = false;
static bool mouse_initialized = false;
static bool mouse_is_wheel = false;

// Курсор мыши (16x16 пикселей)
static const uint32_t mouse_cursor[16][16] = {
    {0xFF000000,0xFF000000,0xFF000000,0xFF000000,0xFF000000,0xFF000000,0xFF000000,0xFF000000,0xFF000000,0xFF000000,0xFF000000,0xFF000000,0xFF000000,0xFF000000,0xFF000000,0xFF000000},
    {0xFF000000,0xFFFFFFFF,0xFFFFFFFF,0xFF000000,0xFF000000,0xFF000000,0xFF000000,0xFF000000,0xFF000000,0xFF000000,0xFF000000,0xFF000000,0xFF000000,0xFF000000,0xFF000000,0xFF000000},
    {0xFF000000,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFF000000,0xFF000000,0xFF000000,0xFF000000,0xFF000000,0xFF000000,0xFF000000,0xFF000000,0xFF000000,0xFF000000,0xFF000000,0xFF000000},
    {0xFF000000,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFF000000,0xFF000000,0xFF000000,0xFF000000,0xFF000000,0xFF000000,0xFF000000,0xFF000000,0xFF000000,0xFF000000,0xFF000000},
    {0xFF000000,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFF000000,0xFF000000,0xFF000000,0xFF000000,0xFF000000,0xFF000000,0xFF000000,0xFF000000,0xFF000000,0xFF000000},
    {0xFF000000,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFF000000,0xFF000000,0xFF000000,0xFF000000,0xFF000000,0xFF000000,0xFF000000,0xFF000000,0xFF000000},
    {0xFF000000,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFF000000,0xFF000000,0xFF000000,0xFF000000,0xFF000000,0xFF000000,0xFF000000,0xFF000000},
    {0xFF000000,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFF000000,0xFF000000,0xFF000000,0xFF000000,0xFF000000,0xFF000000,0xFF000000},
    {0xFF000000,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFF000000,0xFF000000,0xFF000000,0xFF000000,0xFF000000,0xFF000000},
    {0xFF000000,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFF000000,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFF000000,0xFF000000,0xFF000000,0xFF000000,0xFF000000},
    {0xFF000000,0xFFFFFFFF,0xFFFFFFFF,0xFF000000,0xFFFFFFFF,0xFFFFFFFF,0xFF000000,0xFF000000,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFF000000,0xFF000000,0xFF000000,0xFF000000},
    {0xFF000000,0xFFFFFFFF,0xFF000000,0xFF000000,0xFF000000,0xFFFFFFFF,0xFFFFFFFF,0xFF000000,0xFF000000,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFF000000,0xFF000000,0xFF000000},
    {0xFF000000,0xFF000000,0xFF000000,0xFF000000,0xFF000000,0xFF000000,0xFFFFFFFF,0xFFFFFFFF,0xFF000000,0xFF000000,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFF000000,0xFF000000},
    {0xFF000000,0xFF000000,0xFF000000,0xFF000000,0xFF000000,0xFF000000,0xFF000000,0xFFFFFFFF,0xFFFFFFFF,0xFF000000,0xFF000000,0xFFFFFFFF,0xFFFFFFFF,0xFF000000,0xFF000000,0xFF000000},
    {0xFF000000,0xFF000000,0xFF000000,0xFF000000,0xFF000000,0xFF000000,0xFF000000,0xFF000000,0xFFFFFFFF,0xFFFFFFFF,0xFF000000,0xFF000000,0xFF000000,0xFF000000,0xFF000000,0xFF000000},
    {0xFF000000,0xFF000000,0xFF000000,0xFF000000,0xFF000000,0xFF000000,0xFF000000,0xFF000000,0xFF000000,0xFF000000,0xFF000000,0xFF000000,0xFF000000,0xFF000000,0xFF000000,0xFF000000}
};

/**
 * Обработчик прерывания мыши (IRQ12)
 * @param regs Регистры на момент прерывания (не используется)
 */
void mouse_handler(struct registers* regs) {
    (void)regs;
    
    // Читаем данные из порта мыши
    uint8_t data = inb(MOUSE_DATA_PORT);
    
    // Если мы ждем подтверждение
    if (mouse_waiting_for_ack) {
        if (data == 0xFA) { // ACK
            mouse_waiting_for_ack = false;
        }
        return;
    }
    
    // Собираем пакет мыши
    if (mouse_is_wheel) {
        // 4-байтовый пакет для мыши с колесом
        mouse_packet[mouse_packet_index++] = data;
        
        if (mouse_packet_index == 4) {
            mouse_process_packet();
            mouse_packet_index = 0;
        }
    } else {
        // 3-байтовый пакет для обычной мыши
        mouse_packet[mouse_packet_index++] = data;
        
        if (mouse_packet_index == 3) {
            mouse_process_packet();
            mouse_packet_index = 0;
        }
    }
}

/**
 * Обработка собранного пакета мыши
 */
void mouse_process_packet(void) {
    uint8_t flags = mouse_packet[0];
    
    // Проверяем, что это валидный пакет (бит 3 всегда 1)
    if (!(flags & 0x08)) {
        return;
    }
    
    // Обновляем состояние кнопок
    mouse_buttons = flags & 0x07;
    
    // Получаем перемещение по X
    int32_t delta_x = mouse_packet[1];
    if (flags & MOUSE_X_SIGN) {
        delta_x |= 0xFFFFFF00;
    }
    
    // Получаем перемещение по Y
    int32_t delta_y = mouse_packet[2];
    if (flags & MOUSE_Y_SIGN) {
        delta_y |= 0xFFFFFF00;
    }
    
    // Инвертируем Y (мышь обычно дает обратное направление)
    delta_y = -delta_y;
    
    // Получаем перемещение колеса (если есть)
    if (mouse_is_wheel) {
        int8_t delta_z = (int8_t)mouse_packet[3];
        mouse_z += delta_z;
    }
    
    // Обновляем позицию мыши
    mouse_x += delta_x;
    mouse_y += delta_y;
    
    // Ограничиваем позицию границами экрана
    if (mouse_x < 0) mouse_x = 0;
    if (mouse_y < 0) mouse_y = 0;
    if (mouse_x >= SCREEN_WIDTH - 16) mouse_x = SCREEN_WIDTH - 17;
    if (mouse_y >= SCREEN_HEIGHT - 16) mouse_y = SCREEN_HEIGHT - 17;
    
    // Обновляем курсор
    gui_update_cursor();
}

/**
 * Отправка команды мыши
 * @param command Команда для отправки
 * @return Ответ мыши
 */
uint8_t mouse_send_command(uint8_t command) {
    mouse_waiting_for_ack = true;
    
    // Ждем, пока контроллер будет готов принять команду
    mouse_wait_for_write();
    
    // Отправляем команду на порт мыши
    outb(MOUSE_COMMAND_PORT, 0xD4);
    mouse_wait_for_write();
    outb(MOUSE_DATA_PORT, command);
    
    // Ждем подтверждения
    uint32_t timeout = 100000;
    while (mouse_waiting_for_ack && timeout > 0) {
        timeout--;
        asm volatile("pause");
    }
    
    if (timeout == 0) {
        return 0xFF; // Таймаут
    }
    
    // Читаем ответ
    mouse_wait_for_read();
    return inb(MOUSE_DATA_PORT);
}

/**
 * Ожидание возможности чтения из порта мыши
 */
void mouse_wait_for_read(void) {
    uint32_t timeout = 100000;
    while (timeout > 0) {
        if (inb(MOUSE_STATUS_PORT) & 0x01) {
            return;
        }
        timeout--;
        asm volatile("pause");
    }
}

/**
 * Ожидание возможности записи в порт мыши
 */
void mouse_wait_for_write(void) {
    uint32_t timeout = 100000;
    while (timeout > 0) {
        if (!(inb(MOUSE_STATUS_PORT) & 0x02)) {
            return;
        }
        timeout--;
        asm volatile("pause");
    }
}

/**
 * Получение текущей позиции мыши по X
 * @return Позиция X
 */
int mouse_get_x(void) {
    return mouse_x;
}

/**
 * Получение текущей позиции мыши по Y
 * @return Позиция Y
 */
int mouse_get_y(void) {
    return mouse_y;
}

/**
 * Получение текущего положения колеса мыши
 * @return Положение колеса
 */
int mouse_get_z(void) {
    return mouse_z;
}

/**
 * Проверка, нажата ли левая кнопка мыши
 * @return true, если нажата
 */
bool mouse_is_left_pressed(void) {
    return mouse_buttons & MOUSE_LEFT_BUTTON;
}

/**
 * Проверка, нажата ли правая кнопка мыши
 * @return true, если нажата
 */
bool mouse_is_right_pressed(void) {
    return mouse_buttons & MOUSE_RIGHT_BUTTON;
}

/**
 * Проверка, нажата ли средняя кнопка мыши
 * @return true, если нажата
 */
bool mouse_is_middle_pressed(void) {
    return mouse_buttons & MOUSE_MIDDLE_BUTTON;
}

/**
 * Установка позиции мыши
 * @param x Новая позиция X
 * @param y Новая позиция Y
 */
void mouse_set_position(int x, int y) {
    mouse_x = x;
    mouse_y = y;
    
    // Ограничиваем границами
    if (mouse_x < 0) mouse_x = 0;
    if (mouse_y < 0) mouse_y = 0;
    if (mouse_x >= SCREEN_WIDTH - 16) mouse_x = SCREEN_WIDTH - 17;
    if (mouse_y >= SCREEN_HEIGHT - 16) mouse_y = SCREEN_HEIGHT - 17;
}

/**
 * Рисование курсора мыши
 */
void mouse_draw_cursor(void) {
    for (int y = 0; y < 16; y++) {
        for (int x = 0; x < 16; x++) {
            uint32_t color = mouse_cursor[y][x];
            if (color != 0xFF000000) { // Пропускаем полностью прозрачные пиксели
                framebuffer_put_pixel(mouse_x + x, mouse_y + y, color);
            }
        }
    }
}

/**
 * Инициализация мыши
 */
void mouse_init(void) {
    // Регистрируем обработчик прерывания мыши
    irq_register_handler(12, mouse_handler);
    
    // Включаем мышь
    mouse_wait_for_write();
    outb(MOUSE_COMMAND_PORT, 0xA8); // Включаем второй порт PS/2
    
    // Сбрасываем мышь
    mouse_send_command(MOUSE_CMD_RESET);
    
    // Устанавливаем стримовый режим
    mouse_send_command(MOUSE_CMD_SET_STREAM_MODE);
    mouse_send_command(MOUSE_CMD_ENABLE);
    
    // Пробуем включить колесо (Intellimouse)
    mouse_send_command(0xF3);
    mouse_send_command(200); // 200 отсчетов в секунду
    mouse_send_command(0xF3);
    mouse_send_command(100);
    mouse_send_command(0xF3);
    mouse_send_command(80);
    
    // Пробуем получить ID устройства
    mouse_send_command(MOUSE_CMD_GET_DEVICE_ID);
    mouse_wait_for_read();
    uint8_t device_id = inb(MOUSE_DATA_PORT);
    
    if (device_id == 0x00) {
        // Обычная мышь без колеса
        mouse_is_wheel = false;
        #ifdef DEBUG
        terminal_printf("Standard PS/2 mouse detected\n");
        #endif
    } else if (device_id == 0x03) {
        // Мышь с колесом (Intellimouse)
        mouse_is_wheel = true;
        #ifdef DEBUG
        terminal_printf("Intellimouse (wheel) detected\n");
        #endif
    }
    
    // Включаем прерывания мыши
    mouse_wait_for_write();
    outb(MOUSE_COMMAND_PORT, 0x20);
    mouse_wait_for_read();
    uint8_t status = inb(MOUSE_DATA_PORT);
    status |= 0x02; // Включаем прерывания мыши
    status &= 0xDF; // Очищаем бит 5
    mouse_wait_for_write();
    outb(MOUSE_COMMAND_PORT, 0x60);
    mouse_wait_for_write();
    outb(MOUSE_DATA_PORT, status);
    
    // Включаем стримовый режим
    mouse_send_command(MOUSE_CMD_SET_STREAM_MODE);
    mouse_send_command(MOUSE_CMD_ENABLE);
    
    mouse_initialized = true;
    
    #ifdef DEBUG
    terminal_printf("Mouse initialized\n");
    #endif
}

/**
 * Тест мыши
 */
void mouse_test(void) {
    #ifdef DEBUG
    terminal_printf("Mouse test - move mouse around...\n");
    
    int last_x = mouse_x;
    int last_y = mouse_y;
    
    for (int i = 0; i < 100; i++) {
        if (mouse_x != last_x || mouse_y != last_y) {
            terminal_printf("Mouse: X=%d, Y=%d, L=%d, R=%d, M=%d\n",
                          mouse_x, mouse_y,
                          mouse_is_left_pressed(),
                          mouse_is_right_pressed(),
                          mouse_is_middle_pressed());
            last_x = mouse_x;
            last_y = mouse_y;
        }
        timer_sleep(100);
    }
    
    terminal_printf("Mouse test completed.\n");
    #endif
}