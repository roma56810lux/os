/**
 * kernel/timer.c - Драйвер программируемого интервального таймера (PIT)
 */

#include "timer.h"
#include "idt.h"
#include "terminal.h"
#include <stdbool.h>

// Частота таймера в Гц
#define TIMER_FREQUENCY 1000

// Порт команд PIT
#define PIT_CMD_PORT 0x43
// Порт данных канала 0 PIT
#define PIT_CH0_PORT 0x40

// Значение делителя для нужной частоты
#define PIT_DIVIDER 1193180 / TIMER_FREQUENCY

// Глобальные переменные таймера
static volatile uint32_t timer_ticks = 0;
static volatile uint32_t timer_seconds = 0;
static volatile bool timer_initialized = false;

// Список таймерных обратных вызовов
typedef struct timer_callback {
    void (*func)(void*);
    void* data;
    uint32_t interval_ticks;
    uint32_t next_tick;
    struct timer_callback* next;
} timer_callback_t;

static timer_callback_t* callback_list = NULL;

/**
 * Обработчик прерывания таймера (IRQ0)
 * @param regs Регистры на момент прерывания (не используется)
 */
void timer_handler(struct registers* regs) {
    (void)regs; // Не используется
    
    // Увеличиваем счетчик тиков
    timer_ticks++;
    
    // Каждую секунду увеличиваем счетчик секунд
    if (timer_ticks % TIMER_FREQUENCY == 0) {
        timer_seconds++;
    }
    
    // Обрабатываем зарегистрированные обратные вызовы
    timer_callback_t* cb = callback_list;
    while (cb != NULL) {
        if (timer_ticks >= cb->next_tick) {
            // Вызываем функцию обратного вызова
            cb->func(cb->data);
            // Планируем следующий вызов
            cb->next_tick = timer_ticks + cb->interval_ticks;
        }
        cb = cb->next;
    }
}

/**
 * Инициализация таймера
 * @param frequency Желаемая частота таймера в Гц (по умолчанию 1000)
 */
void timer_init(uint32_t frequency) {
    // Регистрируем обработчик прерывания таймера
    irq_register_handler(0, timer_handler);
    
    // Отправляем команду настройки в PIT
    // Бит 0-1: канал 0
    // Бит 2-3: доступ к регистрам low и high
    // Бит 4-5: режим 3 (генератор прямоугольных импульсов)
    // Бит 6-7: двоичный режим (не BCD)
    outb(PIT_CMD_PORT, 0x36);
    
    // Устанавливаем делитель частоты (младший и старший байты)
    uint16_t divider = (uint16_t)(1193180 / frequency);
    outb(PIT_CH0_PORT, divider & 0xFF);        // Младший байт
    outb(PIT_CH0_PORT, (divider >> 8) & 0xFF); // Старший байт
    
    timer_initialized = true;
    
    // Отладочное сообщение
    #ifdef DEBUG
    terminal_printf("Timer initialized at %d Hz\n", frequency);
    #endif
}

/**
 * Получить текущее количество тиков таймера
 * @return Количество тиков с момента инициализации
 */
uint32_t timer_get_ticks(void) {
    return timer_ticks;
}

/**
 * Получить текущее количество секунд
 * @return Количество секунд с момента инициализации
 */
uint32_t timer_get_seconds(void) {
    return timer_seconds;
}

/**
 * Задержка на указанное количество миллисекунд
 * @param ms Количество миллисекунд для задержки
 */
void timer_sleep(uint32_t ms) {
    if (!timer_initialized) return;
    
    uint32_t start_ticks = timer_ticks;
    uint32_t ticks_to_wait = (ms * TIMER_FREQUENCY) / 1000;
    
    // Ждем, пока не пройдет нужное количество тиков
    while (timer_ticks - start_ticks < ticks_to_wait) {
        // Отдаем управление (в будущем здесь будет переключение задач)
        asm volatile("hlt");
    }
}

/**
 * Задержка на указанное количество микросекунд (более точная)
 * @param us Количество микросекунд для задержки
 */
void timer_sleep_us(uint32_t us) {
    // Используем инструкцию CPU для коротких задержек
    // Примерно 1-2 микросекунды на итерацию (зависит от CPU)
    uint32_t loops = us / 2;
    
    for (uint32_t i = 0; i < loops; i++) {
        asm volatile("pause");
    }
}

/**
 * Регистрация периодического обратного вызова
 * @param func Функция обратного вызова
 * @param data Данные для передачи в функцию
 * @param interval_ms Интервал в миллисекундах
 * @return Указатель на структуру обратного вызова или NULL при ошибке
 */
timer_callback_t* timer_register_callback(void (*func)(void*), void* data, uint32_t interval_ms) {
    if (func == NULL) return NULL;
    
    // Создаем новую структуру обратного вызова
    timer_callback_t* new_cb = (timer_callback_t*)malloc(sizeof(timer_callback_t));
    if (new_cb == NULL) return NULL;
    
    new_cb->func = func;
    new_cb->data = data;
    new_cb->interval_ticks = (interval_ms * TIMER_FREQUENCY) / 1000;
    new_cb->next_tick = timer_ticks + new_cb->interval_ticks;
    new_cb->next = NULL;
    
    // Добавляем в начало списка
    if (callback_list == NULL) {
        callback_list = new_cb;
    } else {
        new_cb->next = callback_list;
        callback_list = new_cb;
    }
    
    return new_cb;
}

/**
 * Удаление зарегистрированного обратного вызова
 * @param cb Указатель на структуру обратного вызова
 */
void timer_unregister_callback(timer_callback_t* cb) {
    if (cb == NULL || callback_list == NULL) return;
    
    // Если это первый элемент в списке
    if (callback_list == cb) {
        callback_list = cb->next;
        free(cb);
        return;
    }
    
    // Ищем элемент в списке
    timer_callback_t* current = callback_list;
    while (current->next != NULL && current->next != cb) {
        current = current->next;
    }
    
    // Если нашли, удаляем
    if (current->next == cb) {
        current->next = cb->next;
        free(cb);
    }
}

/**
 * Получить текущее время в формате HH:MM:SS
 * @param buffer Буфер для строки (минимум 9 символов)
 */
void timer_get_time_string(char* buffer) {
    if (buffer == NULL) return;
    
    uint32_t seconds = timer_seconds;
    uint32_t hours = seconds / 3600;
    uint32_t minutes = (seconds % 3600) / 60;
    uint32_t secs = seconds % 60;
    
    // Форматируем время
    buffer[0] = '0' + (hours / 10);
    buffer[1] = '0' + (hours % 10);
    buffer[2] = ':';
    buffer[3] = '0' + (minutes / 10);
    buffer[4] = '0' + (minutes % 10);
    buffer[5] = ':';
    buffer[6] = '0' + (secs / 10);
    buffer[7] = '0' + (secs % 10);
    buffer[8] = '\0';
}

/**
 * Тестовая функция для отладки таймера
 */
void timer_test(void) {
    #ifdef DEBUG
    terminal_printf("Timer test started...\n");
    terminal_printf("Sleeping for 1 second...\n");
    
    uint32_t start_ticks = timer_get_ticks();
    timer_sleep(1000); // 1 секунда
    uint32_t end_ticks = timer_get_ticks();
    
    terminal_printf("Slept for %d ticks\n", end_ticks - start_ticks);
    terminal_printf("Timer test completed.\n");
    #endif
}

// Простая реализация malloc/free для использования в таймере
// В реальной ОС будет использоваться менеджер памяти
static uint8_t heap[4096];
static size_t heap_ptr = 0;

void* malloc(size_t size) {
    if (heap_ptr + size > sizeof(heap)) {
        return NULL;
    }
    void* ptr = &heap[heap_ptr];
    heap_ptr += size;
    return ptr;
}

void free(void* ptr) {
    // В этой простой реализации мы не освобождаем память
    (void)ptr;
}