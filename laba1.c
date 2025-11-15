#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include <ctype.h>

#define MAX_WIDTH 100
#define MAX_HEIGHT 100
#define MIN_SIZE 10
#define MAX_LINE_LENGTH 256

typedef struct {
    // Все данные игры в одном месте:
    char field[MAX_HEIGHT][MAX_WIDTH];
    char color_field[MAX_HEIGHT][MAX_WIDTH];
    int width;
    int height;
    int dino_x;
    int dino_y;
    int size_set;
    int start_set;
    int line_number;
} GameState;

void clear_screen() { // Вызывает системную команду "cls" которая очищает экран консоли
    system("cls");
}

void wait_for_enter(const char* message) {
    if (message) {
        printf("%s", message);
    }
    else {
        printf("nazmite ENTER dlya prodolzeniya...");
    }
    getchar();
}

void display_field(GameState* state, const char* command) {
    clear_screen();
    printf("=== INTERPRETATOR MOVDINO ===\n");

    if (command && strlen(command) > 0) {
        printf("Komanda: %s\n", command);
    }
    printf("Pozitsiya: (%d, %d) | Pole: %dx%d | Stroka: %d\n",
        state->dino_x, state->dino_y, state->width, state->height, state->line_number);
    printf("\n");

    // Отображение координат
    printf("   ");
    for (int x = 0; x < state->width; x++) {
        printf("%d ", x % 10);
    }
    printf("\n");

    //Верхняя граница поля:
    printf("   ");
    for (int x = 0; x < state->width * 2; x++) {
        printf("-");
    }
    printf("\n");

    //Отрисовка самого поля построчно:
    for (int y = 0; y < state->height; y++) {
        printf("%2d|", y);
        for (int x = 0; x < state->width; x++) {
            char cell = state->field[y][x];
            char color = state->color_field[y][x];

            if (cell == '_' && color != '_') {
                printf("%c ", color);
            }
            else {
                printf("%c ", cell);
            }
        }
        printf("|\n");
    }
//Нижняя граница поля:
    printf("   ");
    for (int x = 0; x < state->width * 2; x++) {
        printf("-");
    }
    printf("\n\n");

    printf("Oboznacheniya: #-Dinozavr, %%-yama, ^-gora, &-derevo, @-kamen, a-z-zakrasheno\n");
    printf("==========================================\n");
}

void error_exit(GameState* state, const char* message) {
    printf("\nOSHIBKA v stroke %d: %s\n", state->line_number, message);
    printf("Programma zavershena.\n");
    wait_for_enter("Nazhmite Enter dlya vyhoda...");
    exit(1);
}

void save_game(GameState* state) {
    FILE* file = fopen("output.txt", "w");
    if (!file) {
        printf("Ошибка: Не удалось создать файл output.txt\n");
        return;
    }
    
    fprintf(file, "# Финальное состояние игры - MovDino Интерпретатор\n");
    
    // Сохраняем размер поля и позицию динозавра
    fprintf(file, "SIZE %d %d\n", state->width, state->height);
    fprintf(file, "START %d %d\n\n", state->dino_x, state->dino_y);
    
    // Сохраняем объекты
    fprintf(file, "# Объекты на поле:\n");
    int objects_count = 0;
    for (int y = 0; y < state->height; y++) {
        for (int x = 0; x < state->width; x++) {
            char cell = state->field[y][x];
            if (cell != '_' && cell != '#') {  // Все объекты кроме пустых и динозавра
                fprintf(file, "CELL %d %d %c\n", x, y, cell);
                objects_count++;
            }
        }
    }
    
    fprintf(file, "\n");
    
    // Сохраняем цвета
    fprintf(file, "# Окрашенные клетки:\n");
    int colors_count = 0;
    for (int y = 0; y < state->height; y++) {
        for (int x = 0; x < state->width; x++) {
            char color = state->color_field[y][x];
            if (color != '_') {  // Только окрашенные клетки
                fprintf(file, "COLOR %d %d %c\n", x, y, color);
                colors_count++;
            }
        }
    }
    
    fprintf(file, "\n# Итого: %d объектов, %d окрашенных клеток\n", objects_count, colors_count);
    
    fclose(file);
    printf("Finalnoe sostoyanie sohraneno v output.txt\n");
}

int is_valid_direction(const char* dir) {
    return strcmp(dir, "UP") == 0 || strcmp(dir, "DOWN") == 0 ||
        strcmp(dir, "LEFT") == 0 || strcmp(dir, "RIGHT") == 0;
}

int is_valid_letter(const char* letter) {
    return strlen(letter) == 1 && islower(letter[0]) && isalpha(letter[0]);
}

void get_target_cell(GameState* state, const char* direction, int distance, int* target_x, int* target_y) {
    *target_x = state->dino_x;
    *target_y = state->dino_y;

    if (strcmp(direction, "UP") == 0) {
        *target_y = state->dino_y - distance;
    }
    else if (strcmp(direction, "DOWN") == 0) {
        *target_y = state->dino_y + distance;
    }
    else if (strcmp(direction, "LEFT") == 0) {
        *target_x = state->dino_x - distance;
    }
    else if (strcmp(direction, "RIGHT") == 0) {
        *target_x = state->dino_x + distance;
    }
}


// Функция прыжка динозавра с тороидальной топологией и правильной обработкой препятствий
void jump_dinosaur(GameState* state, const char* direction, int distance) {
    if (distance <= 0) {
        display_field(state, "JUMP - NEKORREKTNAYA DISTANCIYA");
        printf("Preduprezhdenie: Distantsiya pryzhka dolzhna byt polozhitelnoj. Komanda propushchena.\n");
        wait_for_enter("Nazhmite Enter dlya prodolzheniya...");
        return;
    }

    int current_x = state->dino_x;
    int current_y = state->dino_y;
    int final_x = current_x;
    int final_y = current_y;
    int jump_interrupted = 0;

    // Проверяем путь прыжка с учётом тороидальной топологии
    for (int step = 1; step <= distance; step++) {
        int check_x, check_y;
        get_target_cell(state, direction, step, &check_x, &check_y);
        int wrapped_x = (check_x % state->width + state->width) % state->width;
        int wrapped_y = (check_y % state->height + state->height) % state->height;
        char cell = state->field[wrapped_y][wrapped_x];

        //  Дерево (&) и камень (@) — прыжок ЗАПРЕЩЁН полностью
        if (cell == '&' || cell == '@') {
            display_field(state, "JUMP - ZAPRESHCHENO");
            printf("Preduprezhdenie: Nelzya prygat cherez neprohodimoe prepyatstvie '%c'. Pryzhok polnostyu otmenen.\n", cell);
            wait_for_enter("Nazhmite Enter dlya prodolzheniya...");
            return;
        }

        // Гора (^) — можно остановиться перед ней
        if (cell == '^') {
            if (step == 1) {
                // Нельзя прыгнуть даже на 1 клетку
                display_field(state, "JUMP - BLOKIROVANO");
                printf("Preduprezhdenie: Gora '^' na pervom shage. Pryzhok otmenen.\n");
                wait_for_enter("Nazhmite Enter dlya prodolzheniya...");
                return;
            }
            // Останавливаемся на предыдущей клетке (ПЕРЕД горой)
            get_target_cell(state, direction, step - 1, &final_x, &final_y);
            final_x = (final_x % state->width + state->width) % state->width;
            final_y = (final_y % state->height + state->height) % state->height;
            // Обновляем состояние ДО отображения
            state->field[current_y][current_x] = '_';
            state->dino_x = final_x;
            state->dino_y = final_y;
            state->field[final_y][final_x] = '#';
            // Показываем результат прерывания
            display_field(state, "JUMP - PRERIVAN GOROJ");
            printf("Preduprezhdenie: Pryzhok prervan goroy '^' na shage %d. Prizemlenie v (%d,%d)\n",
                   step, final_x, final_y);
            wait_for_enter("Nazhmite Enter dlya prodolzheniya...");
            return; // Важно: не продолжать функцию
        }

        // Яма (%) — ошибка ТОЛЬКО при приземлении (последний шаг)
        if (step == distance && cell == '%') {
            error_exit(state, "Dinozavr upal v yamu vo vremya pryzhka! Igra okonchena.");
        }

        // Если дошли до конца без прерывания — фиксируем позицию
        if (step == distance) {
            final_x = wrapped_x;
            final_y = wrapped_y;
        }
    }

    // === УСПЕШНЫЙ ПРЫЖОК (без препятствий) ===
    state->field[current_y][current_x] = '_';
    state->dino_x = final_x;
    state->dino_y = final_y;
    state->field[final_y][final_x] = '#';

    printf("Prygnul na %d kletok v napravlenii %s. Prizemlenie v (%d,%d)\n",
           distance, direction, final_x, final_y);
    display_field(state, "JUMP - USPESHNO");
    wait_for_enter("Nazhmite Enter dlya prodolzheniya...");
}

// Функция для создания дерева
void grow_tree(GameState* state, const char* direction) {
    int target_x, target_y;
    get_target_cell(state, direction, 1, &target_x, &target_y);

    // Применяем тороидальную топологию
    target_x = (target_x % state->width + state->width) % state->width;
    target_y = (target_y % state->height + state->height) % state->height;

    // Проверяем, что не выращиваем на динозавра
    if (target_x == state->dino_x && target_y == state->dino_y) {
        printf("Preduprezhdenie: Nelzya vyrashchivat derevo na dinozavre. Komanda propushchena.\n");
        return;
    }

    char current_cell = state->field[target_y][target_x];

    // Можно выращивать только на пустой клетке
    if (current_cell != '_') {
        printf("Preduprezhdenie: Nelzya vyrashchivat derevo na nepustoj kletke v (%d,%d). Komanda propushchena.\n", target_x, target_y);
        return;
    }

    state->field[target_y][target_x] = '&';
    printf("Vyroslo derevo v (%d,%d)\n", target_x, target_y);
}

// Функция для рубки дерева
void cut_tree(GameState* state, const char* direction) {
    int target_x, target_y;
    get_target_cell(state, direction, 1, &target_x, &target_y);

    // Применяем тороидальную топологию
    target_x = (target_x % state->width + state->width) % state->width;
    target_y = (target_y % state->height + state->height) % state->height;

    // Проверяем, что в целевой клетке есть дерево
    if (state->field[target_y][target_x] != '&') {
        printf("Preduprezhdenie: Net dereva dlya rubki v (%d,%d). Komanda propushchena.\n", target_x, target_y);
        return;
    }

    // Рубка дерева - клетка становится пустой, цвет сохраняется
    state->field[target_y][target_x] = '_';
    // Цвет сохраняется автоматически в color_field

    printf("Srubleno derevo v (%d,%d). Kletka teper pusta.\n", target_x, target_y);
}

// Функция для создания камня
void make_stone(GameState* state, const char* direction) {
    int target_x, target_y;
    get_target_cell(state, direction, 1, &target_x, &target_y);

    // Применяем тороидальную топологией
    target_x = (target_x % state->width + state->width) % state->width;
    target_y = (target_y % state->height + state->height) % state->height;

    // Проверяем, что не создаем камень на динозавра
    if (target_x == state->dino_x && target_y == state->dino_y) {
        printf("Preduprezhdenie: Nelzya sozdavat kamen na dinozavre. Komanda propushchena.\n");
        return;
    }

    char current_cell = state->field[target_y][target_x];

    // Можно создавать только на пустой клетке
    if (current_cell != '_') {
        printf("Preduprezhdenie: Nelzya sozdavat kamen na nepustoj kletke v (%d,%d). Komanda propushchena.\n", target_x, target_y);
        return;
    }

    state->field[target_y][target_x] = '@';
    printf("Sozdan kamen v (%d,%d)\n", target_x, target_y);
}

// Функция для толкания камня
void push_stone(GameState* state, const char* direction) {
    int stone_x, stone_y;
    get_target_cell(state, direction, 1, &stone_x, &stone_y);

    // Применяем тороидальную топологию
    stone_x = (stone_x % state->width + state->width) % state->width;
    stone_y = (stone_y % state->height + state->height) % state->height;

    // Проверяем, что в целевой клетке есть камень
    if (state->field[stone_y][stone_x] != '@') {
        printf("Preduprezhdenie: Net kamnya dlya tolkaniya v (%d,%d). Komanda propushchena.\n", stone_x, stone_y);
        return;
    }

    // Определяем направление движения камня (противоположное позиции динозавра)
    int push_direction_x, push_direction_y;

    if (strcmp(direction, "UP") == 0) {
        push_direction_x = 0;
        push_direction_y = -1; // Камень движется ВНИЗ (от динозавра)
    }
    else if (strcmp(direction, "DOWN") == 0) {
        push_direction_x = 0;
        push_direction_y = 1;  // Камень движется ВВЕРХ (от динозавра)
    }
    else if (strcmp(direction, "LEFT") == 0) {
        push_direction_x = -1; // Камень движется ВПРАВО (от динозавра)
        push_direction_y = 0;
    }
    else if (strcmp(direction, "RIGHT") == 0) {
        push_direction_x = 1;  // Камень движется ВЛЕВО (от динозавра)
        push_direction_y = 0;
    }

    // Вычисляем новую позицию камня с тороидальной топологией
    int new_stone_x = (stone_x + push_direction_x + state->width) % state->width;
    int new_stone_y = (stone_y + push_direction_y + state->height) % state->height;

    // Проверяем, можно ли переместить камень
    char target_cell = state->field[new_stone_y][new_stone_x];

    if (target_cell == '^' || target_cell == '&' || target_cell == '@') {
        printf("Preduprezhdenie: Nelzya tolknut kamen - prepyatstvie na puti v (%d,%d). Komanda propushchena.\n", new_stone_x, new_stone_y);
        return;
    }

    // Сохраняем цвет исходной позиции камня
    char stone_color = state->color_field[stone_y][stone_x];

    // Перемещаем камень
    if (target_cell == '%') {
        // Камень падает в яму - засыпает её
        state->field[new_stone_y][new_stone_x] = '_';
        state->field[stone_y][stone_x] = '_';
        // Сохраняем цвет из исходной позиции камня
        state->color_field[new_stone_y][new_stone_x] = stone_color;
        printf("Kamen upal v yamu v (%d,%d). Yama zapolnena.\n", new_stone_x, new_stone_y);
    }
    else {
        // Обычное перемещение камня
        state->field[new_stone_y][new_stone_x] = '@';
        state->field[stone_y][stone_x] = '_';
        // Переносим цвет на новую позицию
        state->color_field[new_stone_y][new_stone_x] = stone_color;
        state->color_field[stone_y][stone_x] = '_';
        printf("Tolknuli kamen s (%d,%d) na (%d,%d)\n", stone_x, stone_y, new_stone_x, new_stone_y);
    }
}

// Функция для создания демо-файла с прыжками через границы
void create_demo_program() {
    FILE* file = fopen("input.txt", "r");
    if (file) {
        fclose(file);
        return;
    }

    file = fopen("input.txt", "w");
    if (!file) {
        printf("Oshibka: Ne udalos sozdat fail input.txt\n");
        return;
    }
    // Демо-программа
    fprintf(file, "// MovDino Demo\n");
    fprintf(file, "SIZE 10 10\n");
    fprintf(file, "START 1 1\n");
    fprintf(file, "\n");
    
    // Базовые команды
    fprintf(file, "MOVE RIGHT\n");
    fprintf(file, "PAINT a\n");
    fprintf(file, "MOVE DOWN\n");
    fprintf(file, "PAINT b\n");
    fprintf(file, "\n");
    
    // Создание объектов
    fprintf(file, "GROW UP\n");
    fprintf(file, "MAKE LEFT\n");
    fprintf(file, "DIG RIGHT\n");
    fprintf(file, "\n");
    
    // Прыжки
    fprintf(file, "JUMP UP 2\n");
    fprintf(file, "PAINT c\n");
    fprintf(file, "JUMP RIGHT 5\n");
    fprintf(file, "PAINT d\n");
    fprintf(file, "\n");
    
    // Взаимодействие
    fprintf(file, "MOVE UP\n");
    fprintf(file, "JUMP LEFT 4\n");
    fprintf(file, "CUT LEFT\n");
    fprintf(file, "JUMP LEFT 2\n");
    fprintf(file, "PUSH DOWN\n");
    

    fclose(file);
    printf("Demo file input.txt with jump commands created\n");
}

void show_command_result(GameState* state, const char* format, ...) {
    char display_cmd[100];
    va_list args;
    va_start(args, format);
    vsprintf(display_cmd, format, args);
    va_end(args);
    
    display_field(state, display_cmd);
    wait_for_enter(NULL);
}

void parse_and_execute(GameState* state, char* line) {
    char original_line[MAX_LINE_LENGTH];
    strcpy(original_line, line);

    // Убираем пробелы в начале и конце
    char* trimmed = line;
    while (*trimmed == ' ' || *trimmed == '\t') trimmed++;

    char* end = trimmed + strlen(trimmed) - 1;
    while (end > trimmed && (*end == ' ' || *end == '\t' || *end == '\r' || *end == '\n')) {
        *end = '\0';
        end--;
    }

    // Пропускаем пустые строки
    if (strlen(trimmed) == 0) return;

    // Проверяем отступы в начале оригинальной строки
    if (original_line[0] == ' ' || original_line[0] == '\t') {
        error_exit(state, "Otstupy v nachale stroki zapreshcheny");
    }

    // Убираем комментарии
    char* comment = strstr(trimmed, "//");
    if (comment) {
        *comment = '\0';
        // Обрезаем пробелы после удаления комментария
        end = trimmed + strlen(trimmed) - 1;
        while (end > trimmed && (*end == ' ' || *end == '\t')) {
            *end = '\0';
            end--;
        }
    }

    // Пропускаем строки, ставшие пустыми после удаления комментария
    if (strlen(trimmed) == 0) return;

    // Парсим команду
    char command[50], param1[50], param2[50], param3[50];
    int parsed = sscanf(trimmed, "%49s %49s %49s %49s", command, param1, param2, param3);

    if (parsed < 1) return;

    printf("Vypolnyaetsya: %s\n", trimmed);

    if (strcmp(command, "SIZE") != 0 && strcmp(command, "START") != 0) {
        if (!state->size_set || !state->start_set) {
            error_exit(state, "SIZE i START dolzhny byt ustanovleny pered komandami");
        }
    }

    // === ПРОВЕРКА КОМАНД ===

    if (strcmp(command, "SIZE") == 0) {
        if (state->size_set) {
            error_exit(state, "Komanda SIZE mozhet ispolzovatsya tolko odin raz");
        }
        if (parsed != 3) {
            error_exit(state, "Komanda SIZE trebuet shirinu i vysotu");
        }

        int width = atoi(param1);
        int height = atoi(param2);

        if (width < MIN_SIZE || width > MAX_WIDTH || height < MIN_SIZE || height > MAX_HEIGHT) {
            error_exit(state, "Razmery polya dolzhny byt mezhdu 10x10 i 100x100");
        }

        state->width = width;
        state->height = height;
        state->size_set = 1;

        // Инициализация поля
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                state->field[y][x] = '_';
                state->color_field[y][x] = '_';
            }
        }

        display_field(state, "SIZE - Inicializatsiya polya");
        wait_for_enter(NULL);
    }
    else if (strcmp(command, "START") == 0) {
        if (!state->size_set) {
            error_exit(state, "SIZE dolzhen byt ustanovlen pered START");
        }
        if (state->start_set) {
            error_exit(state, "Komanda START mozhet ispolzovatsya tolko odin raz");
        }
        if (parsed != 3) {
            error_exit(state, "Komanda START trebuet koordinaty x i y");
        }

        int x = atoi(param1);
        int y = atoi(param2);

        if (x < 0 || x >= state->width || y < 0 || y >= state->height) {
            error_exit(state, "Startovaya poziciya vne granic polya");
        }

        state->dino_x = x;
        state->dino_y = y;
        state->field[y][x] = '#';
        state->start_set = 1;

        display_field(state, "START - Razmeshenie dinozavra");
        wait_for_enter(NULL);
    }

    else if (strcmp(command, "EXEC") == 0) {
    if (parsed != 2) {
        error_exit(state, "Komanda EXEC trebuet imya fayla");
    }
    FILE* exec_file = fopen(param1, "r");
    if (!exec_file) {
        error_exit(state, "Ne udalos otkryt fayl dlya EXEC");
    }
    char exec_line[MAX_LINE_LENGTH];
    while (fgets(exec_line, sizeof(exec_line), exec_file)) {
        state->line_number++; // считаем каждую строку из вложенного файла
        parse_and_execute(state, exec_line);
    }
    fclose(exec_file);
}

    else if (strcmp(command, "JUMP") == 0) {
        if (parsed != 3) {
            error_exit(state, "Komanda JUMP trebuet napravlenie i distanciyu");
        }
        if (!is_valid_direction(param1)) {
            error_exit(state, "Nevernoe napravlenie. Ispolzujte: UP, DOWN, LEFT, RIGHT");
        }

        int distance = atoi(param2);

        if (distance <= 0) {
            error_exit(state, "Distanciya JUMP dolzhna byt polozhitelnym celym chislom");
        }
        
        jump_dinosaur(state, param1, distance);
    }

    else if (strcmp(command, "MOVE") == 0) {
        if (parsed != 2) {
            error_exit(state, "Komanda MOVE trebuet napravlenie");
        }
        if (!is_valid_direction(param1)) {
            error_exit(state, "Nevernoe napravlenie. Ispolzujte: UP, DOWN, LEFT, RIGHT");
        }

        int target_x, target_y;
        get_target_cell(state, param1, 1, &target_x, &target_y);

        // Применяем тороидальную топологию
        target_x = (target_x % state->width + state->width) % state->width;
        target_y = (target_y % state->height + state->height) % state->height;

        char target_cell = state->field[target_y][target_x];

        if (target_cell == '%') {
            error_exit(state, "Dinozavr upal v yamu! Igra okonchena.");
        }

        if (target_cell == '^' || target_cell == '&' || target_cell == '@') {
            display_field(state, "MOVE - BLOKIROVANO");
            printf("Preduprezhdenie: Nelzya peremestitsya na prepyatstvie. Komanda propushchena.\n");
            wait_for_enter(NULL);
            return;
        }

        // Выполняем перемещение
        state->field[state->dino_y][state->dino_x] = '_';
        state->dino_x = target_x;
        state->dino_y = target_y;
        state->field[target_y][target_x] = '#';

        show_command_result(state, "MOVE %s", param1);
    }

    else if (strcmp(command, "GROW") == 0) {
        if (parsed != 2) {
            error_exit(state, "Komanda GROW trebuet napravlenie");
        }
        if (!is_valid_direction(param1)) {
            error_exit(state, "Nevernoe napravlenie. Ispolzujte: UP, DOWN, LEFT, RIGHT");
        }

        grow_tree(state, param1);

        show_command_result(state, "GROW %s", param1);
    }

    else if (strcmp(command, "CUT") == 0) {
        if (parsed != 2) {
            error_exit(state, "Komanda CUT trebuet napravlenie");
        }
        if (!is_valid_direction(param1)) {
            error_exit(state, "Nevernoe napravlenie. Ispolzujte: UP, DOWN, LEFT, RIGHT");
        }

        cut_tree(state, param1);

        show_command_result(state, "CUT %s", param1);
    }

    else if (strcmp(command, "MAKE") == 0) {
        if (parsed != 2) {
            error_exit(state, "Komanda MAKE trebuet napravlenie");
        }
        if (!is_valid_direction(param1)) {
            error_exit(state, "Nevernoe napravlenie. Ispolzujte: UP, DOWN, LEFT, RIGHT");
        }

        make_stone(state, param1);
        show_command_result(state, "MAKE %s", param1);
    }

    else if (strcmp(command, "PUSH") == 0) {
        if (parsed != 2) {
            error_exit(state, "Komanda PUSH trebuet napravlenie");
        }
        if (!is_valid_direction(param1)) {
            error_exit(state, "Nevernoe napravlenie. Ispolzujte: UP, DOWN, LEFT, RIGHT");
        }

        push_stone(state, param1);
        show_command_result(state, "PUSH %s", param1);
    }

    else if (strcmp(command, "PAINT") == 0) {
        if (parsed != 2) {
            error_exit(state, "Komanda PAINT trebuet bukvu");
        }
        if (!is_valid_letter(param1)) {
            error_exit(state, "PAINT trebuet odnu strochnuyu bukvu (a-z)");
        }

        state->color_field[state->dino_y][state->dino_x] = param1[0];
        show_command_result(state, "PAINT %s", param1);

    }

    else if (strcmp(command, "DIG") == 0) {
        if (parsed != 2) {
            error_exit(state, "Komanda DIG trebuet napravlenie");
        }
        if (!is_valid_direction(param1)) {
            error_exit(state, "Nevernoe napravlenie. Ispolzujte: UP, DOWN, LEFT, RIGHT");
        }

        int target_x, target_y;
        get_target_cell(state, param1, 1, &target_x, &target_y);

        // Применяем тороидальную топологию
        target_x = (target_x % state->width + state->width) % state->width;
        target_y = (target_y % state->height + state->height) % state->height;

        // Проверяем, что не копаем под динозавра
        if (target_x == state->dino_x && target_y == state->dino_y) {
            printf("Preduprezhdenie: Nelzya kopat pod dinozavrom. Komanda propushchena.\n");
            display_field(state, "DIG - БЛОКИРОВАНО");
            wait_for_enter(NULL);
            return;
        }

        char current_cell = state->field[target_y][target_x];

        if (current_cell == '^') {
            state->field[target_y][target_x] = '_';
            printf("Gora zapolnena yamoj. Kletka teper pusta.\n");
        }
        else if (current_cell == '_') {
            state->field[target_y][target_x] = '%';
        }
        else {
            printf("Preduprezhdenie: Nelzya kopat na nepustoj kletke. Komanda propushchena.\n");
        }

        show_command_result(state, "DIG %s", param1);

    }

    else if (strcmp(command, "MOUND") == 0) {
        if (parsed != 2) {
            error_exit(state, "Komanda MOUND trebuet napravlenie");
        }
        if (!is_valid_direction(param1)) {
            error_exit(state, "Nevernoe napravlenie. Ispolzujte: UP, DOWN, LEFT, RIGHT");
        }

        int target_x, target_y;
        get_target_cell(state, param1, 1, &target_x, &target_y);

        // Применяем тороидальную топологию
        target_x = (target_x % state->width + state->width) % state->width;
        target_y = (target_y % state->height + state->height) % state->height;

        // Проверяем, что не насыпаем на динозавра
        if (target_x == state->dino_x && target_y == state->dino_y) {
            printf("Preduprezhdenie: Nelzya sozdavat nasyip na dinozavre. Komanda propushchena.\n");
            display_field(state, "MOUND - БЛОКИРОВАНО");
            wait_for_enter(NULL);
            return;
        }

        char current_cell = state->field[target_y][target_x];

        if (current_cell == '%') {
            state->field[target_y][target_x] = '_';
            printf("Yama zapolnena nasyipyuu. Kletka teper pusta.\n");
        }
        else if (current_cell == '_') {
            state->field[target_y][target_x] = '^';
        }
        else {
            printf("Preduprezhdenie: Nelzya sozdavat nasyip na nepustoj kletke. Komanda propushchena.\n");
        }

        show_command_result(state, "MOUND %s", param1);
    }

    else {
        error_exit(state, "Neizvestnaya komanda");
    }
}

int main() {
    printf("Poisk faila input.txt...\n");

    create_demo_program();

    FILE* file = fopen("input.txt", "r");
    if (!file) {
        printf("Ошибка: Не удалось открыть файл input.txt\n");
        printf("Убедитесь, что input.txt находится в той же папке\n");
        wait_for_enter("Нажмите Enter для выхода...");
        return 1;
    }

    printf("Fail input.txt najden! Nazhmite Enter dlya nachala vypolneniya...");
    getchar();

    GameState state;
    memset(&state, 0, sizeof(state));

    char line[MAX_LINE_LENGTH];
    state.line_number = 0;

    while (fgets(line, sizeof(line), file)) {
        state.line_number++;
        parse_and_execute(&state, line);
    }

    fclose(file);

    save_game(&state);

    printf("\n=== PROGRAMMA USPESHNO ZAVERSHENA ===\n");
    printf("Finalnaya pozitsiya: (%d, %d)\n", state.dino_x, state.dino_y);
    wait_for_enter("Nazhmite Enter dlya vyhoda...");

    return 0;
}