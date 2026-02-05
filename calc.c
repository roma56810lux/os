#include "sys.h"

typedef struct {
    int x, y;
    int width, height;
    char* title;
} Window;

void draw_window(Window* win) {
    // Отрисовка окна
    for(int i = win->y; i < win->y + win->height; i++) {
        for(int j = win->x; j < win->x + win->width; j++) {
            draw_pixel(j, i, 0xFFFFFF); // Белый фон
        }
    }
}

void calc_app() {
    Window calc_win = {100, 100, 200, 300, "Calculator"};
    draw_window(&calc_win);
    
    // Логика калькулятора
    while(1) {
        // Обработка ввода
        // Отрисовка кнопок
        // Вычисления
    }
}