#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#define THREAD_COUNT 32 // Количество потоков

// Структура для передачи данных потокам
typedef struct {
    unsigned char *image;       // Исходное изображение
    unsigned char *result_image; // Обработанное изображение
    int width;                  // Ширина изображения
    int height;                 // Высота изображения
    int begin;                  // Начальная строка для обработки
    int end;                    // Конечная строка для обработки
    int thread_id;              // Идентификатор потока
} ThreadData;

// Функция фильтра Собеля для потоков
void *sobel_filter(void *arg) {
    ThreadData *data = (ThreadData *)arg; // Преобразование аргумента в структуру ThreadData
    struct timespec thread_start, thread_end;
    clock_gettime(CLOCK_MONOTONIC, &thread_start); // Запуск таймера для потока

    printf("Поток %d начал обработку строк с %d по %d\n", data->thread_id, data->begin, data->end);

    for (int y = data->begin; y < data->end; y++) { // Перебор строк изображения
        for (int x = 1; x < data->width - 1; x++) { // Перебор пикселей в строке, кроме границ
            // Вычисление градиентов по осям x и y
            int gx = (data->image[(y - 1) * data->width + (x + 1)] + 2 * data->image[y * data->width + (x + 1)] + data->image[(y + 1) * data->width + (x + 1)]) -
                     (data->image[(y - 1) * data->width + (x - 1)] + 2 * data->image[y * data->width + (x - 1)] + data->image[(y + 1) * data->width + (x - 1)]);
            int gy = (data->image[(y + 1) * data->width + (x - 1)] + 2 * data->image[(y + 1) * data->width + x] + data->image[(y + 1) * data->width + (x + 1)]) -
                     (data->image[(y - 1) * data->width + (x - 1)] + 2 * data->image[(y - 1) * data->width + x] + data->image[(y - 1) * data->width + (x + 1)]);
            // Вычисление модуля градиента
            int sum = sqrt(gx * gx + gy * gy); 
            if (sum > 255) sum = 255; // Ограничение максимального значения градиента
            data->result_image[y * data->width + x] = sum; // Запись результата в выходное изображение
        }
    }

    clock_gettime(CLOCK_MONOTONIC, &thread_end); // Остановка таймера для потока
    double thread_seconds = (thread_end.tv_sec - thread_start.tv_sec) + (thread_end.tv_nsec - thread_start.tv_nsec) / 1e9; // Вычисление времени выполнения потока
    printf("Поток %d завершил обработку за %.3f мс\n", data->thread_id, thread_seconds * 1000);

    pthread_exit(NULL); // Завершение потока
}

// Функция для обработки изображения с использованием фильтра Собеля и многопоточности
void process_image(const char *input_path, const char *output_path) {
    int width, height, channels;
    // Загрузка изображения
    unsigned char *image = stbi_load(input_path, &width, &height, &channels, 1); 
    if (image == NULL) {
        fprintf(stderr, "Ошибка загрузки изображения: %s\n", stbi_failure_reason());
        exit(1);
    }

    // Вывод информации об изображении
    printf("Информация об изображении:\n");
    printf("  Ширина: %d пикселей\n", width);
    printf("  Высота: %d пикселей\n", height);
    printf("  Количество каналов: %d\n", channels);

    // Выделение памяти для обработанного изображения
    unsigned char *result_image = (unsigned char *)malloc(width * height * sizeof(unsigned char));
    if (result_image == NULL) {
        fprintf(stderr, "Ошибка выделения памяти для обработанного изображения\n");
        stbi_image_free(image);
        exit(1);
    }

    // Разделение работы между потоками
    int delta_y = (height - 1) / THREAD_COUNT;
    int y1 = 1;
    int y2 = delta_y;

    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start); // Запуск таймера

    // Создание потоков
    pthread_t threads[THREAD_COUNT];
    ThreadData thread_data[THREAD_COUNT];
    for (int i = 0; i < THREAD_COUNT; i++) {
        thread_data[i].image = image;
        thread_data[i].result_image = result_image;
        thread_data[i].width = width;
        thread_data[i].height = height;
        thread_data[i].begin = y1;
        thread_data[i].end = y2;
        thread_data[i].thread_id = i; // Присвоение идентификатора потока

        // Создание потока
        if (pthread_create(&threads[i], NULL, sobel_filter, (void *)&thread_data[i]) != 0) {
            fprintf(stderr, "Ошибка создания потока %d\n", i);
            free(result_image);
            stbi_image_free(image);
            exit(1);
        }

        y1 = y2; // Обновление начальной строки для следующего потока
        y2 += delta_y; // Обновление конечной строки для следующего потока
        if (y2 > height - 1) {
            y2 = height - 1; // Обработка оставшихся строк в последнем потоке
        }
    }

    // Ожидание завершения потоков
    for (int i = 0; i < THREAD_COUNT; i++) {
        pthread_join(threads[i], NULL);
    }

    clock_gettime(CLOCK_MONOTONIC, &end); // Остановка таймера
    double seconds = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9; // Вычисление времени выполнения

    printf("Общее время выполнения: %d потоков  --  %.3f мс\n", THREAD_COUNT, seconds * 1000); // Вывод времени выполнения
    
    // Сохранение обработанного изображения
    stbi_write_png(output_path, width, height, 1, result_image, width);

    // Освобождение памяти
    stbi_image_free(image);
    free(result_image);
}

int main() {
    const char *image_path = "img.jpg"; // Путь к исходному изображению
    const char *output_path = "result.png"; // Путь для сохранения обработанного изображения

    FILE *fout = fopen("test_records.txt", "a"); // Открытие файла для записи времени выполнения
    if (fout == NULL) {
        fprintf(stderr, "Ошибка открытия файла для записи\n");
        exit(1);
    }

    process_image(image_path, output_path); // Обработка изображения

    fclose(fout); // Закрытие файла

    return 0; // Завершение программы
}
