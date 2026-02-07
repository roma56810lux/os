/**
 * kernel/gui.c - Графический интерфейс пользователя (GUI)
 */

#include "gui.h"
#include "framebuffer.h"
#include "mouse.h"
#include "keyboard.h"
#include "terminal.h"
#include <stdbool.h>
#include <string.h>

// Состояние GUI
static gui_state_t gui_state;
static bool gui_initialized = false;

// Элементы GUI
static gui_window_t windows[MAX_WINDOWS];
static gui_button_t buttons[MAX_BUTTONS];
static gui_label_t labels[MAX_LABELS];
static gui_menu_t menus[MAX_MENUS];

// Курсор мыши
static int cursor_old_x = -1;
static int cursor_old_y = -1;
static bool cursor_visible = true;

// Цвета GUI
static const uint32_t COLOR_WINDOW_BG = 0x333333;
static const uint32_t COLOR_WINDOW_TITLE = 0x444444;
static const uint32_t COLOR_WINDOW_BORDER = 0x222222;
static const uint32_t COLOR_BUTTON_NORMAL = 0x555555;
static const uint32_t COLOR_BUTTON_HOVER = 0x666666;
static const uint32_t COLOR_BUTTON_PRESSED = 0x444444;
static const uint32_t COLOR_TEXT = 0xFFFFFF;
static const uint32_t COLOR_DESKTOP_BG = 0x224488;

/**
 * Инициализация графического интерфейса
 */
void gui_init(void) {
    if (gui_initialized) return;
    
    // Инициализируем состояние GUI
    memset(&gui_state, 0, sizeof(gui_state_t));
    gui_state.active_window = -1;
    gui_state.desktop_color = COLOR_DESKTOP_BG;
    
    // Инициализируем массивы элементов
    memset(windows, 0, sizeof(windows));
    memset(buttons, 0, sizeof(buttons));
    memset(labels, 0, sizeof(labels));
    memset(menus, 0, sizeof(menus));
    
    // Создаем окно терминала
    gui_create_window(50, 50, 600, 400, "Terminal", true);
    
    // Создаем меню
    gui_create_menu(0, 0, framebuffer_get_width(), 24, "System Menu");
    
    // Добавляем кнопки в меню
    gui_create_button(10, 2, 60, 20, "File", 0);
    gui_create_button(80, 2, 60, 20, "Edit", 0);
    gui_create_button(150, 2, 60, 20, "View", 0);
    gui_create_button(220, 2, 60, 20, "Help", 0);
    
    gui_initialized = true;
    
    #ifdef DEBUG
    terminal_printf("GUI initialized\n");
    #endif
}

/**
 * Отрисовка рабочего стола
 */
void gui_draw_desktop(void) {
    if (!gui_initialized) return;
    
    // Очищаем экран цветом рабочего стола
    framebuffer_clear(gui_state.desktop_color);
    
    // Рисуем иконки на рабочем столе (заглушки)
    framebuffer_draw_rect(20, 40, 32, 32, 0x666666);
    framebuffer_draw_string(15, 75, "Term", 0xFFFFFF, 0x00000000);
    
    framebuffer_draw_rect(70, 40, 32, 32, 0x666666);
    framebuffer_draw_string(60, 75, "Files", 0xFFFFFF, 0x00000000);
    
    // Рисуем все окна
    for (int i = 0; i < MAX_WINDOWS; i++) {
        if (windows[i].visible) {
            gui_draw_window(i);
        }
    }
    
    // Рисуем все кнопки
    for (int i = 0; i < MAX_BUTTONS; i++) {
        if (buttons[i].visible) {
            gui_draw_button(i);
        }
    }
    
    // Рисуем все метки
    for (int i = 0; i < MAX_LABELS; i++) {
        if (labels[i].visible) {
            gui_draw_label(i);
        }
    }
    
    // Рисуем все меню
    for (int i = 0; i < MAX_MENUS; i++) {
        if (menus[i].visible) {
            gui_draw_menu(i);
        }
    }
    
    // Рисуем курсор мыши
    mouse_draw_cursor();
}

/**
 * Обновление курсора мыши (стирание старого и отрисовка нового)
 */
void gui_update_cursor(void) {
    if (!gui_initialized) return;
    
    int mouse_x = mouse_get_x();
    int mouse_y = mouse_get_y();
    
    // Если позиция не изменилась, ничего не делаем
    if (mouse_x == cursor_old_x && mouse_y == cursor_old_y) {
        return;
    }
    
    // Стираем старый курсор
    if (cursor_old_x >= 0 && cursor_old_y >= 0) {
        // Нужно перерисовать область под курсором
        // Для простоты перерисовываем весь экран
        // В реальной системе нужно перерисовывать только область курсора
        gui_draw_desktop();
    }
    
    // Сохраняем новую позицию
    cursor_old_x = mouse_x;
    cursor_old_y = mouse_y;
    
    // Рисуем курсор в новой позиции
    mouse_draw_cursor();
    
    // Обновляем экран
    framebuffer_swap();
}

/**
 * Создание окна
 * @param x Координата X
 * @param y Координата Y
 * @param width Ширина
 * @param height Высота
 * @param title Заголовок окна
 * @param resizable Возможность изменения размера
 * @return ID окна или -1 при ошибке
 */
int gui_create_window(uint16_t x, uint16_t y, uint16_t width, uint16_t height, 
                     const char* title, bool resizable) {
    for (int i = 0; i < MAX_WINDOWS; i++) {
        if (!windows[i].visible) {
            windows[i].x = x;
            windows[i].y = y;
            windows[i].width = width;
            windows[i].height = height;
            windows[i].visible = true;
            windows[i].active = false;
            windows[i].resizable = resizable;
            windows[i].title_color = COLOR_WINDOW_TITLE;
            windows[i].bg_color = COLOR_WINDOW_BG;
            windows[i].border_color = COLOR_WINDOW_BORDER;
            
            if (title != NULL) {
                strncpy(windows[i].title, title, MAX_TITLE_LENGTH - 1);
                windows[i].title[MAX_TITLE_LENGTH - 1] = '\0';
            } else {
                windows[i].title[0] = '\0';
            }
            
            return i;
        }
    }
    
    return -1;
}

/**
 * Отрисовка окна
 * @param window_id ID окна
 */
void gui_draw_window(int window_id) {
    if (window_id < 0 || window_id >= MAX_WINDOWS || !windows[window_id].visible) {
        return;
    }
    
    gui_window_t* win = &windows[window_id];
    
    // Рисуем фон окна
    framebuffer_draw_rect(win->x, win->y, win->width, win->height, win->bg_color);
    
    // Рисуем рамку окна
    framebuffer_draw_rect_outline(win->x, win->y, win->width, win->height, 2, win->border_color);
    
    // Рисуем заголовок окна
    framebuffer_draw_rect(win->x + 2, win->y + 2, win->width - 4, 20, win->title_color);
    
    // Рисуем текст заголовка
    framebuffer_draw_string(win->x + 10, win->y + 6, win->title, COLOR_TEXT, win->title_color);
    
    // Рисуем кнопки управления окном (закрыть, свернуть)
    framebuffer_draw_rect(win->x + win->width - 40, win->y + 4, 16, 16, 0xFF5555); // Закрыть
    framebuffer_draw_rect(win->x + win->width - 60, win->y + 4, 16, 16, 0xFFFF55); // Свернуть
}

/**
 * Создание кнопки
 * @param x Координата X
 * @param y Координата Y
 * @param width Ширина
 * @param height Высота
 * @param text Текст кнопки
 * @param parent_window ID родительского окна (-1 для экрана)
 * @return ID кнопки или -1 при ошибке
 */
int gui_create_button(uint16_t x, uint16_t y, uint16_t width, uint16_t height, 
                     const char* text, int parent_window) {
    for (int i = 0; i < MAX_BUTTONS; i++) {
        if (!buttons[i].visible) {
            buttons[i].x = x;
            buttons[i].y = y;
            buttons[i].width = width;
            buttons[i].height = height;
            buttons[i].visible = true;
            buttons[i].parent_window = parent_window;
            buttons[i].state = BUTTON_NORMAL;
            
            if (text != NULL) {
                strncpy(buttons[i].text, text, MAX_BUTTON_TEXT - 1);
                buttons[i].text[MAX_BUTTON_TEXT - 1] = '\0';
            } else {
                buttons[i].text[0] = '\0';
            }
            
            return i;
        }
    }
    
    return -1;
}

/**
 * Отрисовка кнопки
 * @param button_id ID кнопки
 */
void gui_draw_button(int button_id) {
    if (button_id < 0 || button_id >= MAX_BUTTONS || !buttons[button_id].visible) {
        return;
    }
    
    gui_button_t* btn = &buttons[button_id];
    
    // Определяем цвет кнопки в зависимости от состояния
    uint32_t color;
    switch (btn->state) {
        case BUTTON_HOVER:
            color = COLOR_BUTTON_HOVER;
            break;
        case BUTTON_PRESSED:
            color = COLOR_BUTTON_PRESSED;
            break;
        default:
            color = COLOR_BUTTON_NORMAL;
            break;
    }
    
    // Рассчитываем абсолютные координаты
    uint16_t abs_x = btn->x;
    uint16_t abs_y = btn->y;
    
    if (btn->parent_window >= 0 && btn->parent_window < MAX_WINDOWS) {
        abs_x += windows[btn->parent_window].x;
        abs_y += windows[btn->parent_window].y + 24; // Ниже заголовка
    }
    
    // Рисуем кнопку
    framebuffer_draw_rect(abs_x, abs_y, btn->width, btn->height, color);
    framebuffer_draw_rect_outline(abs_x, abs_y, btn->width, btn->height, 1, COLOR_WINDOW_BORDER);
    
    // Рисуем текст кнопки (центрированный)
    if (btn->text[0] != '\0') {
        uint16_t text_x = abs_x + (btn->width - strlen(btn->text) * 8) / 2;
        uint16_t text_y = abs_y + (btn->height - 16) / 2;
        framebuffer_draw_string(text_x, text_y, btn->text, COLOR_TEXT, color);
    }
}

/**
 * Создание метки (текста)
 * @param x Координата X
 * @param y Координата Y
 * @param text Текст метки
 * @param parent_window ID родительского окна (-1 для экрана)
 * @return ID метки или -1 при ошибке
 */
int gui_create_label(uint16_t x, uint16_t y, const char* text, int parent_window) {
    for (int i = 0; i < MAX_LABELS; i++) {
        if (!labels[i].visible) {
            labels[i].x = x;
            labels[i].y = y;
            labels[i].visible = true;
            labels[i].parent_window = parent_window;
            
            if (text != NULL) {
                strncpy(labels[i].text, text, MAX_LABEL_TEXT - 1);
                labels[i].text[MAX_LABEL_TEXT - 1] = '\0';
            } else {
                labels[i].text[0] = '\0';
            }
            
            return i;
        }
    }
    
    return -1;
}

/**
 * Отрисовка метки
 * @param label_id ID метки
 */
void gui_draw_label(int label_id) {
    if (label_id < 0 || label_id >= MAX_LABELS || !labels[label_id].visible) {
        return;
    }
    
    gui_label_t* label = &labels[label_id];
    
    // Рассчитываем абсолютные координаты
    uint16_t abs_x = label->x;
    uint16_t abs_y = label->y;
    
    if (label->parent_window >= 0 && label->parent_window < MAX_WINDOWS) {
        abs_x += windows[label->parent_window].x;
        abs_y += windows[label->parent_window].y + 24;
    }
    
    // Рисуем текст
    framebuffer_draw_string(abs_x, abs_y, label->text, COLOR_TEXT, 0x00000000);
}

/**
 * Создание меню
 * @param x Координата X
 * @param y Координата Y
 * @param width Ширина
 * @param height Высота
 * @param title Заголовок меню
 * @return ID меню или -1 при ошибке
 */
int gui_create_menu(uint16_t x, uint16_t y, uint16_t width, uint16_t height, const char* title) {
    for (int i = 0; i < MAX_MENUS; i++) {
        if (!menus[i].visible) {
            menus[i].x = x;
            menus[i].y = y;
            menus[i].width = width;
            menus[i].height = height;
            menus[i].visible = true;
            
            if (title != NULL) {
                strncpy(menus[i].title, title, MAX_TITLE_LENGTH - 1);
                menus[i].title[MAX_TITLE_LENGTH - 1] = '\0';
            } else {
                menus[i].title[0] = '\0';
            }
            
            return i;
        }
    }
    
    return -1;
}

/**
 * Отрисовка меню
 * @param menu_id ID меню
 */
void gui_draw_menu(int menu_id) {
    if (menu_id < 0 || menu_id >= MAX_MENUS || !menus[menu_id].visible) {
        return;
    }
    
    gui_menu_t* menu = &menus[menu_id];
    
    // Рисуем фон меню
    framebuffer_draw_rect(menu->x, menu->y, menu->width, menu->height, COLOR_WINDOW_TITLE);
    
    // Рисуем текст меню
    framebuffer_draw_string(menu->x + 10, menu->y + 4, menu->title, COLOR_TEXT, COLOR_WINDOW_TITLE);
}

/**
 * Обработка событий мыши (вызывается из основного цикла)
 */
void gui_handle_mouse(void) {
    if (!gui_initialized) return;
    
    int mouse_x = mouse_get_x();
    int mouse_y = mouse_get_y();
    bool left_pressed = mouse_is_left_pressed();
    
    // Проверяем клики по кнопкам
    for (int i = 0; i < MAX_BUTTONS; i++) {
        if (!buttons[i].visible) continue;
        
        gui_button_t* btn = &buttons[i];
        
        // Рассчитываем абсолютные координаты кнопки
        uint16_t abs_x = btn->x;
        uint16_t abs_y = btn->y;
        
        if (btn->parent_window >= 0 && btn->parent_window < MAX_WINDOWS) {
            abs_x += windows[btn->parent_window].x;
            abs_y += windows[btn->parent_window].y + 24;
        }
        
        // Проверяем, находится ли мышь над кнопкой
        bool is_over = (mouse_x >= abs_x && mouse_x <= abs_x + btn->width &&
                       mouse_y >= abs_y && mouse_y <= abs_y + btn->height);
        
        // Обновляем состояние кнопки
        if (is_over) {
            if (left_pressed) {
                btn->state = BUTTON_PRESSED;
            } else {
                btn->state = BUTTON_HOVER;
            }
        } else {
            btn->state = BUTTON_NORMAL;
        }
    }
    
    // Проверяем клики по окнам
    for (int i = 0; i < MAX_WINDOWS; i++) {
        if (!windows[i].visible) continue;
        
        gui_window_t* win = &windows[i];
        
        // Проверяем, находится ли мышь в области окна
        bool is_over = (mouse_x >= win->x && mouse_x <= win->x + win->width &&
                       mouse_y >= win->y && mouse_y <= win->y + win->height);
        
        if (is_over && left_pressed) {
            // Активируем окно
            gui_state.active_window = i;
            windows[i].active = true;
        }
    }
}

/**
 * Обработка событий клавиатуры (вызывается из основного цикла)
 */
void gui_handle_keyboard(void) {
    if (!gui_initialized) return;
    
    // Обработка горячих клавиш
    if (keyboard_is_ctrl_pressed() && keyboard_is_key_pressed(0x1E)) { // Ctrl+A
        // Действие по Ctrl+A
    }
}

/**
 * Обновление GUI (вызывается из основного цикла)
 */
void gui_update(void) {
    if (!gui_initialized) return;
    
    gui_handle_mouse();
    gui_handle_keyboard();
    
    // Перерисовываем GUI при необходимости
    static uint32_t last_redraw = 0;
    uint32_t current_ticks = timer_get_ticks();
    
    if (current_ticks - last_redraw > 30) { // ~30 FPS
        gui_draw_desktop();
        framebuffer_swap();
        last_redraw = current_ticks;
    }
}

/**
 * Тест GUI
 */
void gui_test(void) {
    if (!gui_initialized) {
        gui_init();
    }
    
    #ifdef DEBUG
    terminal_printf("Starting GUI test...\n");
    
    // Создаем тестовые элементы
    int win1 = gui_create_window(100, 100, 300, 200, "Test Window 1", true);
    int win2 = gui_create_window(150, 150, 250, 180, "Test Window 2", false);
    
    // Создаем кнопки в первом окне
    gui_create_button(20, 40, 80, 30, "Button 1", win1);
    gui_create_button(120, 40, 80, 30, "Button 2", win1);
    
    // Создаем метки
    gui_create_label(20, 90, "This is a label", win1);
    gui_create_label(20, 110, "Another label", win1);
    
    // Отрисовываем
    gui_draw_desktop();
    framebuffer_swap();
    
    terminal_printf("GUI test completed\n");
    #endif
}