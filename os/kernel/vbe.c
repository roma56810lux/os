/**
 * kernel/vbe.c - Управление VESA BIOS Extensions (VBE)
 */

#include "vbe.h"
#include "framebuffer.h"
#include "terminal.h"
#include <stdbool.h>
#include <string.h>

// Глобальные переменные VBE
static vbe_mode_info_t* vbe_mode_info = NULL;
static uint32_t* framebuffer = NULL;
static uint32_t framebuffer_size = 0;
static uint16_t screen_width = 0;
static uint16_t screen_height = 0;
static uint8_t screen_bpp = 0;
static uint16_t screen_pitch = 0;
static bool vbe_initialized = false;

// Внешние переменные из загрузчика
extern uint32_t framebuffer_addr;
extern uint16_t screen_width_boot;
extern uint16_t screen_height_boot;
extern uint8_t screen_bpp_boot;

/**
 * Инициализация VBE графического режима
 * @param fb_addr Адрес framebuffer
 * @param width Ширина экрана
 * @param height Высота экрана
 * @param bpp Бит на пиксель
 */
void vbe_init(uint32_t fb_addr, uint16_t width, uint16_t height, uint8_t bpp) {
    // Сохраняем параметры из загрузчика
    framebuffer = (uint32_t*)fb_addr;
    screen_width = width;
    screen_height = height;
    screen_bpp = bpp;
    
    // Рассчитываем pitch (байт на строку)
    screen_pitch = screen_width * (screen_bpp / 8);
    
    // Рассчитываем размер framebuffer
    framebuffer_size = screen_width * screen_height * (screen_bpp / 8);
    
    // Выделяем память для информации о режиме
    vbe_mode_info = (vbe_mode_info_t*)malloc(sizeof(vbe_mode_info_t));
    if (vbe_mode_info == NULL) {
        #ifdef DEBUG
        terminal_printf("Failed to allocate VBE mode info\n");
        #endif
        return;
    }
    
    // Заполняем информацию о режиме
    memset(vbe_mode_info, 0, sizeof(vbe_mode_info_t));
    vbe_mode_info->width = screen_width;
    vbe_mode_info->height = screen_height;
    vbe_mode_info->bpp = screen_bpp;
    vbe_mode_info->pitch = screen_pitch;
    vbe_mode_info->framebuffer = fb_addr;
    
    vbe_initialized = true;
    
    #ifdef DEBUG
    terminal_printf("VBE initialized: %dx%d, %dbpp\n", 
                   screen_width, screen_height, screen_bpp);
    #endif
}

/**
 * Получение информации о текущем графическом режиме
 * @param info Указатель на структуру для информации
 * @return true, если успешно
 */
bool vbe_get_mode_info(vbe_mode_info_t* info) {
    if (!vbe_initialized || info == NULL) {
        return false;
    }
    
    memcpy(info, vbe_mode_info, sizeof(vbe_mode_info_t));
    return true;
}

/**
 * Получение ширины экрана
 * @return Ширина экрана в пикселях
 */
uint16_t vbe_get_width(void) {
    return screen_width;
}

/**
 * Получение высоты экрана
 * @return Высота экрана в пикселях
 */
uint16_t vbe_get_height(void) {
    return screen_height;
}

/**
 * Получение глубины цвета
 * @return Бит на пиксель
 */
uint8_t vbe_get_bpp(void) {
    return screen_bpp;
}

/**
 * Получение адреса framebuffer
 * @return Адрес framebuffer
 */
uint32_t* vbe_get_framebuffer(void) {
    return framebuffer;
}

/**
 * Получение pitch (байт на строку)
 * @return Pitch в байтах
 */
uint16_t vbe_get_pitch(void) {
    return screen_pitch;
}

/**
 * Проверка, инициализирован ли VBE
 * @return true, если VBE инициализирован
 */
bool vbe_is_initialized(void) {
    return vbe_initialized;
}

/**
 * Установка пикселя на экране
 * @param x Координата X
 * @param y Координата Y
 * @param color Цвет пикселя (формат 0xRRGGBB)
 */
void vbe_put_pixel(uint16_t x, uint16_t y, uint32_t color) {
    if (!vbe_initialized || x >= screen_width || y >= screen_height) {
        return;
    }
    
    uint32_t offset = y * screen_width + x;
    framebuffer[offset] = color;
}

/**
 * Получение цвета пикселя
 * @param x Координата X
 * @param y Координата Y
 * @return Цвет пикселя
 */
uint32_t vbe_get_pixel(uint16_t x, uint16_t y) {
    if (!vbe_initialized || x >= screen_width || y >= screen_height) {
        return 0;
    }
    
    uint32_t offset = y * screen_width + x;
    return framebuffer[offset];
}

/**
 * Очистка экрана указанным цветом
 * @param color Цвет заливки
 */
void vbe_clear_screen(uint32_t color) {
    if (!vbe_initialized) {
        return;
    }
    
    for (uint32_t i = 0; i < screen_width * screen_height; i++) {
        framebuffer[i] = color;
    }
}

/**
 * Рисование прямоугольника
 * @param x Координата X верхнего левого угла
 * @param y Координата Y верхнего левого угла
 * @param width Ширина прямоугольника
 * @param height Высота прямоугольника
 * @param color Цвет заливки
 */
void vbe_draw_rect(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint32_t color) {
    if (!vbe_initialized) {
        return;
    }
    
    // Ограничиваем координаты
    if (x >= screen_width || y >= screen_height) {
        return;
    }
    
    uint16_t end_x = x + width;
    uint16_t end_y = y + height;
    
    if (end_x > screen_width) end_x = screen_width;
    if (end_y > screen_height) end_y = screen_height;
    
    // Рисуем прямоугольник
    for (uint16_t py = y; py < end_y; py++) {
        for (uint16_t px = x; px < end_x; px++) {
            uint32_t offset = py * screen_width + px;
            framebuffer[offset] = color;
        }
    }
}

/**
 * Рисование прямоугольной рамки
 * @param x Координата X верхнего левого угла
 * @param y Координата Y верхнего левого угла
 * @param width Ширина прямоугольника
 * @param height Высота прямоугольника
 * @param thickness Толщина линии
 * @param color Цвет рамки
 */
void vbe_draw_rect_outline(uint16_t x, uint16_t y, uint16_t width, uint16_t height, 
                          uint8_t thickness, uint32_t color) {
    if (!vbe_initialized) {
        return;
    }
    
    // Верхняя линия
    vbe_draw_rect(x, y, width, thickness, color);
    
    // Нижняя линия
    vbe_draw_rect(x, y + height - thickness, width, thickness, color);
    
    // Левая линия
    vbe_draw_rect(x, y, thickness, height, color);
    
    // Правая линия
    vbe_draw_rect(x + width - thickness, y, thickness, height, color);
}

/**
 * Рисование линии
 * @param x0 Начальная координата X
 * @param y0 Начальная координата Y
 * @param x1 Конечная координата X
 * @param y1 Конечная координата Y
 * @param color Цвет линии
 */
void vbe_draw_line(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint32_t color) {
    if (!vbe_initialized) {
        return;
    }
    
    // Алгоритм Брезенхэма
    int dx = abs(x1 - x0);
    int dy = abs(y1 - y0);
    int sx = (x0 < x1) ? 1 : -1;
    int sy = (y0 < y1) ? 1 : -1;
    int err = dx - dy;
    
    while (1) {
        vbe_put_pixel(x0, y0, color);
        
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
 * Рисование окружности
 * @param cx Координата X центра
 * @param cy Координата Y центра
 * @param radius Радиус
 * @param color Цвет окружности
 */
void vbe_draw_circle(uint16_t cx, uint16_t cy, uint16_t radius, uint32_t color) {
    if (!vbe_initialized) {
        return;
    }
    
    int x = radius;
    int y = 0;
    int err = 0;
    
    while (x >= y) {
        vbe_put_pixel(cx + x, cy + y, color);
        vbe_put_pixel(cx + y, cy + x, color);
        vbe_put_pixel(cx - y, cy + x, color);
        vbe_put_pixel(cx - x, cy + y, color);
        vbe_put_pixel(cx - x, cy - y, color);
        vbe_put_pixel(cx - y, cy - x, color);
        vbe_put_pixel(cx + y, cy - x, color);
        vbe_put_pixel(cx + x, cy - y, color);
        
        y++;
        err += 1 + 2 * y;
        if (2 * (err - x) + 1 > 0) {
            x--;
            err += 1 - 2 * x;
        }
    }
}

/**
 * Тест графических возможностей VBE
 */
void vbe_test(void) {
    if (!vbe_initialized) {
        terminal_printf("VBE not initialized for test\n");
        return;
    }
    
    #ifdef DEBUG
    terminal_printf("Starting VBE graphics test...\n");
    
    // Очищаем экран черным цветом
    vbe_clear_screen(0x000000);
    
    // Рисуем градиентный фон
    for (uint16_t y = 0; y < screen_height; y++) {
        uint8_t blue = (y * 255) / screen_height;
        for (uint16_t x = 0; x < screen_width; x++) {
            uint8_t red = (x * 255) / screen_width;
            uint32_t color = (red << 16) | blue;
            vbe_put_pixel(x, y, color);
        }
    }
    
    // Рисуем разноцветные прямоугольники
    vbe_draw_rect(50, 50, 200, 150, 0xFF0000);      // Красный
    vbe_draw_rect(70, 70, 160, 110, 0x00FF00);      // Зеленый
    vbe_draw_rect(90, 90, 120, 70, 0x0000FF);       // Синий
    
    // Рисуем рамки
    vbe_draw_rect_outline(300, 50, 200, 150, 3, 0xFFFF00); // Желтая рамка
    
    // Рисуем линии
    vbe_draw_line(550, 50, 750, 200, 0xFF00FF);     // Пурпурная линия
    vbe_draw_line(550, 200, 750, 50, 0x00FFFF);     // Голубая линия
    
    // Рисуем окружности
    vbe_draw_circle(200, 400, 50, 0xFFFFFF);        // Белый круг
    vbe_draw_circle(200, 400, 40, 0xFF0000);        // Красный круг
    vbe_draw_circle(200, 400, 30, 0x00FF00);        // Зеленый круг
    vbe_draw_circle(200, 400, 20, 0x0000FF);        // Синий круг
    
    // Рисуем сетку
    for (uint16_t x = 400; x < 800; x += 20) {
        vbe_draw_line(x, 300, x, 500, 0x444444);
    }
    for (uint16_t y = 300; y < 500; y += 20) {
        vbe_draw_line(400, y, 800, y, 0x444444);
    }
    
    terminal_printf("VBE graphics test completed\n");
    #endif
}

/**
 * Простая реализация malloc/free для VBE
 */
static uint8_t vbe_heap[2048];
static size_t vbe_heap_ptr = 0;

void* malloc(size_t size) {
    if (vbe_heap_ptr + size > sizeof(vbe_heap)) {
        return NULL;
    }
    void* ptr = &vbe_heap[vbe_heap_ptr];
    vbe_heap_ptr += size;
    return ptr;
}

void free(void* ptr) {
    (void)ptr; // В этой простой реализации не освобождаем память
}