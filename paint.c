#include "sys.h"

typedef struct {
    uint32_t color;
    int brush_size;
    int tool;
} PaintState;

void paint_app() {
    PaintState state = {0x000000, 3, 0}; // Черный цвет, размер 3px
    
    while(1) {
        // Обработка мыши
        // Отрисовка кистью
        // Выбор цвета
        // Сохранение изображения
    }
}

void draw_brush(int x, int y, PaintState* state) {
    for(int i = y - state->brush_size; i <= y + state->brush_size; i++) {
        for(int j = x - state->brush_size; j <= x + state->brush_size; j++) {
            draw_pixel(j, i, state->color);
        }
    }
}