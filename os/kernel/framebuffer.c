/**
 * kernel/framebuffer.c - Абстракция над VBE framebuffer
 */

#include "framebuffer.h"
#include "vbe.h"
#include "terminal.h"
#include <stdbool.h>
#include <string.h>

// Глобальные переменные framebuffer
static uint32_t* back_buffer = NULL;
static uint32_t* front_buffer = NULL;
static uint32_t framebuffer_size = 0;
static uint16_t screen_width = 0;
static uint16_t screen_height = 0;
static uint8_t screen_bpp = 0;
static bool double_buffering = false;
static bool initialized = false;

// Шрифт 8x16 (простейший bitmap шрифт)
static const uint8_t font_8x16[256][16] = {
    // ... (шрифт будет сокращен для примера)
    // 'A'
    {0x00, 0x00, 0x18, 0x18, 0x24, 0x24, 0x42, 0x42, 
     0x7E, 0x42, 0x42, 0x42, 0x42, 0x42, 0x00, 0x00},
    // 'B'
    {0x00, 0x00, 0x7C, 0x42, 0x42, 0x42, 0x7C, 0x42,
     0x42, 0x42, 0x42, 0x42, 0x7C, 0x00, 0x00, 0x00},
    // ... остальные символы
};

// Цветовая палитра (16 цветов для простоты)
static const uint32_t color_palette[16] = {
    0x000000, // Черный
    0x0000AA, // Синий
    0x00AA00, // Зеленый
    0x00AAAA, // Голубой
    0xAA0000, // Красный
    0xAA00AA, // Пурпурный
    0xAA5500, // Коричневый
    0xAAAAAA, // Серый
    0x555555, // Темно-серый
    0x5555FF, // Ярко-синий
    0x55FF55, // Ярко-зеленый
    0x55FFFF, // Ярко-голубой
    0xFF5555, // Ярко-красный
    0xFF55FF, // Ярко-пурпурный
    0xFFFF55, // Желтый
    0xFFFFFF  // Белый
};

/**
 * Инициализация framebuffer
 */
void framebuffer_init(void) {
    // Получаем параметры экрана от VBE
    screen_width = vbe_get_width();
    screen_height = vbe_get_height();
    screen_bpp = vbe_get_bpp();
    
    if (screen_width == 0 || screen_height == 0) {
        #ifdef DEBUG
        terminal_printf("Framebuffer: Invalid screen dimensions\n");
        #endif
        return;
    }
    
    // Получаем front buffer от VBE
    front_buffer = vbe_get_framebuffer();
    if (front_buffer == NULL) {
        #ifdef DEBUG
        terminal_printf("Framebuffer: No front buffer\n");
        #endif
        return;
    }
    
    // Рассчитываем размер буфера
    framebuffer_size = screen_width * screen_height * (screen_bpp / 8);
    
    // Пытаемся включить двойную буферизацию
    back_buffer = (uint32_t*)malloc(framebuffer_size);
    if (back_buffer != NULL) {
        double_buffering = true;
        memset(back_buffer, 0, framebuffer_size);
        #ifdef DEBUG
        terminal_printf("Framebuffer: Double buffering enabled\n");
        #endif
    } else {
        double_buffering = false;
        #ifdef DEBUG
        terminal_printf("Framebuffer: Double buffering disabled\n");
        #endif
    }
    
    initialized = true;
    
    #ifdef DEBUG
    terminal_printf("Framebuffer initialized: %dx%d, %dbpp\n", 
                   screen_width, screen_height, screen_bpp);
    #endif
}

/**
 * Получение текущего буфера для рисования
 * @return Указатель на активный буфер
 */
static uint32_t* get_draw_buffer(void) {
    return double_buffering ? back_buffer : front_buffer;
}

/**
 * Обмен буферов (отображение back buffer на экран)
 */
void framebuffer_swap(void) {
    if (!initialized || !double_buffering) {
        return;
    }
    
    // Копируем back buffer в front buffer
    memcpy(front_buffer, back_buffer, framebuffer_size);
}

/**
 * Очистка буфера указанным цветом
 * @param color Цвет заливки
 */
void framebuffer_clear(uint32_t color) {
    if (!initialized) return;
    
    uint32_t* buffer = get_draw_buffer();
    
    for (uint32_t i = 0; i < screen_width * screen_height; i++) {
        buffer[i] = color;
    }
}

/**
 * Установка пикселя в буфере
 * @param x Координата X
 * @param y Координата Y
 * @param color Цвет пикселя
 */
void framebuffer_put_pixel(uint16_t x, uint16_t y, uint32_t color) {
    if (!initialized || x >= screen_width || y >= screen_height) {
        return;
    }
    
    uint32_t* buffer = get_draw_buffer();
    uint32_t offset = y * screen_width + x;
    buffer[offset] = color;
}

/**
 * Получение цвета пикселя из буфера
 * @param x Координата X
 * @param y Координата Y
 * @return Цвет пикселя
 */
uint32_t framebuffer_get_pixel(uint16_t x, uint16_t y) {
    if (!initialized || x >= screen_width || y >= screen_height) {
        return 0;
    }
    
    uint32_t* buffer = get_draw_buffer();
    uint32_t offset = y * screen_width + x;
    return buffer[offset];
}

/**
 * Рисование прямоугольника
 * @param x Координата X
 * @param y Координата Y
 * @param width Ширина
 * @param height Высота
 * @param color Цвет
 */
void framebuffer_draw_rect(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint32_t color) {
    if (!initialized) return;
    
    // Ограничиваем координаты
    if (x >= screen_width || y >= screen_height) return;
    
    uint16_t end_x = x + width;
    uint16_t end_y = y + height;
    
    if (end_x > screen_width) end_x = screen_width;
    if (end_y > screen_height) end_y = screen_height;
    
    uint32_t* buffer = get_draw_buffer();
    
    for (uint16_t py = y; py < end_y; py++) {
        uint32_t row_offset = py * screen_width;
        for (uint16_t px = x; px < end_x; px++) {
            buffer[row_offset + px] = color;
        }
    }
}

/**
 * Рисование прямоугольной рамки
 * @param x Координата X
 * @param y Координата Y
 * @param width Ширина
 * @param height Высота
 * @param thickness Толщина линии
 * @param color Цвет
 */
void framebuffer_draw_rect_outline(uint16_t x, uint16_t y, uint16_t width, uint16_t height, 
                                   uint8_t thickness, uint32_t color) {
    if (!initialized) return;
    
    // Верхняя линия
    framebuffer_draw_rect(x, y, width, thickness, color);
    
    // Нижняя линия
    framebuffer_draw_rect(x, y + height - thickness, width, thickness, color);
    
    // Левая линия
    framebuffer_draw_rect(x, y, thickness, height, color);
    
    // Правая линия
    framebuffer_draw_rect(x + width - thickness, y, thickness, height, color);
}

/**
 * Рисование линии
 * @param x0 Начало X
 * @param y0 Начало Y
 * @param x1 Конец X
 * @param y1 Конец Y
 * @param color Цвет
 */
void framebuffer_draw_line(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint32_t color) {
    if (!initialized) return;
    
    // Алгоритм Брезенхэма
    int dx = abs(x1 - x0);
    int dy = abs(y1 - y0);
    int sx = (x0 < x1) ? 1 : -1;
    int sy = (y0 < y1) ? 1 : -1;
    int err = dx - dy;
    
    while (1) {
        framebuffer_put_pixel(x0, y0, color);
        
        if (x0 == x1 && y0 == y1) break;
        
        int e2 = 2 * err;
        if (e2 > -dy) {
            err -= dy;
            x0 += sx;
        }
        if (e2 < dx) {
            err += dx;
            y0 += sy;
        }
    }
}

/**
 * Рисование символа
 * @param x Координата X
 * @param y Координата Y
 * @param c Символ
 * @param color Цвет
 * @param bg_color Цвет фона
 */
void framebuffer_draw_char(uint16_t x, uint16_t y, char c, uint32_t color, uint32_t bg_color) {
    if (!initialized || c < 0 || c > 255) return;
    
    uint8_t char_index = (uint8_t)c;
    
    // Рисуем каждый пиксель символа
    for (uint8_t row = 0; row < 16; row++) {
        uint8_t row_data = font_8x16[char_index][row];
        
        for (uint8_t col = 0; col < 8; col++) {
            uint16_t px = x + col;
            uint16_t py = y + row;
            
            if (px >= screen_width || py >= screen_height) continue;
            
            if (row_data & (1 << (7 - col))) {
                framebuffer_put_pixel(px, py, color);
            } else if (bg_color != 0xFFFFFFFF) { // 0xFFFFFFFF = прозрачный
                framebuffer_put_pixel(px, py, bg_color);
            }
        }
    }
}

/**
 * Рисование строки
 * @param x Координата X
 * @param y Координата Y
 * @param str Строка
 * @param color Цвет текста
 * @param bg_color Цвет фона (0xFFFFFFFF для прозрачного)
 */
void framebuffer_draw_string(uint16_t x, uint16_t y, const char* str, uint32_t color, uint32_t bg_color) {
    if (!initialized || str == NULL) return;
    
    uint16_t current_x = x;
    uint16_t current_y = y;
    
    while (*str) {
        if (*str == '\n') {
            current_x = x;
            current_y += 16;
        } else {
            framebuffer_draw_char(current_x, current_y, *str, color, bg_color);
            current_x += 8;
            
            // Перенос строки при достижении границы экрана
            if (current_x + 8 >= screen_width) {
                current_x = x;
                current_y += 16;
            }
        }
        
        str++;
    }
}

/**
 * Рисование строки с форматированием
 * @param x Координата X
 * @param y Координата Y
 * @param color Цвет текста
 * @param bg_color Цвет фона
 * @param format Форматная строка
 * @param ... Аргументы
 */
void framebuffer_printf(uint16_t x, uint16_t y, uint32_t color, uint32_t bg_color, const char* format, ...) {
    if (!initialized || format == NULL) return;
    
    char buffer[256];
    
    // Простая реализация форматирования
    va_list args;
    va_start(args, format);
    
    // В реальной реализации здесь должен быть vsprintf
    // Для простоты - только строки
    char* ptr = buffer;
    while (*format) {
        if (*format == '%') {
            format++;
            if (*format == 's') {
                char* str = va_arg(args, char*);
                while (*str) {
                    *ptr++ = *str++;
                }
            }
        } else {
            *ptr++ = *format;
        }
        format++;
    }
    *ptr = '\0';
    
    va_end(args);
    
    framebuffer_draw_string(x, y, buffer, color, bg_color);
}

/**
 * Рисование изображения (raw данные)
 * @param x Координата X
 * @param y Координата Y
 * @param width Ширина изображения
 * @param height Высота изображения
 * @param data Данные изображения (формат 32-bit)
 */
void framebuffer_draw_image(uint16_t x, uint16_t y, uint16_t width, uint16_t height, const uint32_t* data) {
    if (!initialized || data == NULL) return;
    
    for (uint16_t py = 0; py < height; py++) {
        uint16_t screen_y = y + py;
        if (screen_y >= screen_height) break;
        
        for (uint16_t px = 0; px < width; px++) {
            uint16_t screen_x = x + px;
            if (screen_x >= screen_width) break;
            
            uint32_t color = data[py * width + px];
            if ((color & 0xFF000000) != 0xFF000000) { // Пропускаем прозрачные пиксели
                framebuffer_put_pixel(screen_x, screen_y, color);
            }
        }
    }
}

/**
 * Копирование области экрана
 * @param src_x Исходная X
 * @param src_y Исходная Y
 * @param width Ширина
 * @param height Высота
 * @param dst_x Целевая X
 * @param dst_y Целевая Y
 */
void framebuffer_blit(uint16_t src_x, uint16_t src_y, uint16_t width, uint16_t height, 
                     uint16_t dst_x, uint16_t dst_y) {
    if (!initialized) return;
    
    // Проверяем границы
    if (src_x >= screen_width || src_y >= screen_height ||
        dst_x >= screen_width || dst_y >= screen_height) {
        return;
    }
    
    uint32_t* buffer = get_draw_buffer();
    
    // Копируем построчно
    for (uint16_t y = 0; y < height; y++) {
        uint16_t src_row = src_y + y;
        uint16_t dst_row = dst_y + y;
        
        if (src_row >= screen_height || dst_row >= screen_height) break;
        
        uint32_t* src_ptr = &buffer[src_row * screen_width + src_x];
        uint32_t* dst_ptr = &buffer[dst_row * screen_width + dst_x];
        
        uint16_t copy_width = width;
        if (src_x + copy_width > screen_width) copy_width = screen_width - src_x;
        if (dst_x + copy_width > screen_width) copy_width = screen_width - dst_x;
        
        memmove(dst_ptr, src_ptr, copy_width * sizeof(uint32_t));
    }
}

/**
 * Получение ширины экрана
 * @return Ширина экрана
 */
uint16_t framebuffer_get_width(void) {
    return screen_width;
}

/**
 * Получение высоты экрана
 * @return Высота экрана
 */
uint16_t framebuffer_get_height(void) {
    return screen_height;
}

/**
 * Проверка, инициализирован ли framebuffer
 * @return true, если инициализирован
 */
bool framebuffer_is_initialized(void) {
    return initialized;
}

/**
 * Тест framebuffer
 */
void framebuffer_test(void) {
    if (!initialized) {
        terminal_printf("Framebuffer not initialized\n");
        return;
    }
    
    #ifdef DEBUG
    terminal_printf("Starting framebuffer test...\n");
    
    // Очищаем экран
    framebuffer_clear(0x000033);
    
    // Рисуем тестовый текст
    framebuffer_draw_string(100, 50, "Framebuffer Test", 0xFFFFFF, 0x000033);
    framebuffer_draw_string(100, 70, "Double buffering: ", 0xCCCCCC, 0x000033);
    framebuffer_draw_string(300, 70, double_buffering ? "ENABLED" : "DISABLED", 
                           double_buffering ? 0x00FF00 : 0xFF0000, 0x000033);
    
    // Рисуем разноцветные блоки
    framebuffer_draw_rect(100, 100, 100, 100, 0xFF0000);
    framebuffer_draw_rect(220, 100, 100, 100, 0x00FF00);
    framebuffer_draw_rect(340, 100, 100, 100, 0x0000FF);
    
    // Рисуем рамки
    framebuffer_draw_rect_outline(100, 220, 340, 100, 3, 0xFFFF00);
    
    // Рисуем линии
    framebuffer_draw_line(100, 340, 440, 340, 0xFF00FF);
    framebuffer_draw_line(100, 340, 100, 440, 0x00FFFF);
    framebuffer_draw_line(440, 340, 440, 440, 0x00FFFF);
    framebuffer_draw_line(100, 440, 440, 440, 0xFF00FF);
    
    // Рисуем текст внутри рамок
    framebuffer_draw_string(120, 240, "This is a text inside", 0xFFFFFF, 0x000033);
    framebuffer_draw_string(120, 260, "a yellow outline box", 0xFFFFFF, 0x000033);
    
    // Обновляем экран
    if (double_buffering) {
        framebuffer_swap();
    }
    
    terminal_printf("Framebuffer test completed\n");
    #endif
}