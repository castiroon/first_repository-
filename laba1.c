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

void clear_screen() {
    system("cls");
}

void wait_for_enter(const char* message) {
    if (message) {
        printf("%s", message);
    }
    else {
        printf("Нажмите ENTER для продолжения...");
    }
    getchar();
}

void display_field(GameState* state, const char* command) {
    clear_screen();
    printf("=== ИНТЕРПРЕТАТОР MOVDINO ===\n");

    if (command && strlen(command) > 0) {
        printf("Команда: %s\n", command);
    }
    printf("Позиция: (%d, %d) | Поле: %dx%d | Строка: %d\n",
        state->dino_x, state->dino_y, state->width, state->height, state->line_number);
    printf("\n");

    // Отображение координат
    printf("   ");
    for (int x = 0; x < state->width; x++) {
        printf("%d ", x % 10);
    }
    printf("\n");

    printf("   ");
    for (int x = 0; x < state->width * 2; x++) {
        printf("-");
    }
    printf("\n");

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

    printf("   ");
    for (int x = 0; x < state->width * 2; x++) {
        printf("-");
    }
    printf("\n\n");

    printf("Обозначения: #-Динозавр, %%-Яма, ^-Гора, &-Дерево, @-Камень, a-z-Закрашено\n");
    printf("==========================================\n");
}

void error_exit(GameState* state, const char* message) {
    printf("\nОШИБКА в строке %d: %s\n", state->line_number, message);
    printf("Программа завершена.\n");
    wait_for_enter("Нажмите Enter для выхода...");
    exit(1);
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

int is_obstacle(char cell) {
    return cell == '^' || cell == '&' || cell == '@';
}

// Функция прыжка динозавра с тороидальной топологией
void jump_dinosaur(GameState* state, const char* direction, int distance) {
    if (distance <= 0) {
        printf("Предупреждение: Дистанция прыжка должна быть положительной. Команда пропущена.\n");
        return;
    }

    int current_x = state->dino_x;
    int current_y = state->dino_y;
    int final_x = current_x;
    int final_y = current_y;
    int jump_interrupted = 0;

    // Проверяем путь прыжка с учетом тороидальной топологии
    for (int step = 1; step <= distance; step++) {
        int check_x, check_y;
        get_target_cell(state, direction, step, &check_x, &check_y);

        // Применяем тороидальную топологию
        int wrapped_x = (check_x % state->width + state->width) % state->width;
        int wrapped_y = (check_y % state->height + state->height) % state->height;

        char cell_at_step = state->field[wrapped_y][wrapped_x];

        // Если встречаем гору - останавливаемся перед ней
        if (cell_at_step == '^') {
            if (step == 1) {
                // Не можем прыгнуть вообще
                printf("Предупреждение: Гора блокирует путь прыжка на первом шаге. Команда пропущена.\n");
                return;
            }

            // Останавливаемся на предыдущей клетке
            get_target_cell(state, direction, step - 1, &final_x, &final_y);
            final_x = (final_x % state->width + state->width) % state->width;
            final_y = (final_y % state->height + state->height) % state->height;

            printf("Предупреждение: Прыжок прерван горой на шаге %d. Приземлился в (%d,%d)\n",
                step, final_x, final_y);

            jump_interrupted = 1;
            break;
        }

        // Если приземляемся на яму - ошибка
        if (step == distance && cell_at_step == '%') {
            error_exit(state, "Динозавр упал в яму во время прыжка! Игра окончена.");
        }

        // Если встречаем другие препятствия (дерево, камень) - перепрыгиваем их
        if (step < distance && (cell_at_step == '&' || cell_at_step == '@')) {
            printf("Перепрыгиваем препятствие в (%d,%d)\n", wrapped_x, wrapped_y);
        }

        // Если это последний шаг и не было прерывания, устанавливаем финальные координаты
        if (step == distance && !jump_interrupted) {
            final_x = wrapped_x;
            final_y = wrapped_y;
        }
    }

    // Проверяем финальную клетку на яму (если не было прерывания горой)
    if (!jump_interrupted) {
        char final_cell = state->field[final_y][final_x];
        if (final_cell == '%') {
            error_exit(state, "Динозавр упал в яму во время прыжка! Игра окончена.");
        }
    }

    // Выполняем прыжок
    state->field[current_y][current_x] = '_';
    state->dino_x = final_x;
    state->dino_y = final_y;
    state->field[final_y][final_x] = '#';

    printf("Прыгнул с (%d,%d) на (%d,%d) на расстояние %d\n",
        current_x, current_y, final_x, final_y, distance);
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
        printf("Предупреждение: Нельзя выращивать дерево на динозавре. Команда пропущена.\n");
        return;
    }

    char current_cell = state->field[target_y][target_x];

    // Можно выращивать только на пустой клетке
    if (current_cell != '_') {
        printf("Предупреждение: Нельзя выращивать дерево на непустой клетке в (%d,%d). Команда пропущена.\n", target_x, target_y);
        return;
    }

    state->field[target_y][target_x] = '&';
    printf("Выросло дерево в (%d,%d)\n", target_x, target_y);
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
        printf("Предупреждение: Нет дерева для рубки в (%d,%d). Команда пропущена.\n", target_x, target_y);
        return;
    }

    // Рубка дерева - клетка становится пустой, цвет сохраняется
    state->field[target_y][target_x] = '_';
    // Цвет сохраняется автоматически в color_field

    printf("Срублено дерево в (%d,%d). Клетка теперь пуста.\n", target_x, target_y);
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
        printf("Предупреждение: Нельзя создавать камень на динозавре. Команда пропущена.\n");
        return;
    }

    char current_cell = state->field[target_y][target_x];

    // Можно создавать только на пустой клетке
    if (current_cell != '_') {
        printf("Предупреждение: Нельзя создавать камень на непустой клетке в (%d,%d). Команда пропущена.\n", target_x, target_y);
        return;
    }

    state->field[target_y][target_x] = '@';
    printf("Создан камень в (%d,%d)\n", target_x, target_y);
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
        printf("Предупреждение: Нет камня для толкания в (%d,%d). Команда пропущена.\n", stone_x, stone_y);
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

    if (is_obstacle(target_cell) || target_cell == '@') {
        printf("Предупреждение: Нельзя толкнуть камень - препятствие на пути в (%d,%d). Команда пропущена.\n", new_stone_x, new_stone_y);
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
        printf("Камень упал в яму в (%d,%d). Яма заполнена.\n", new_stone_x, new_stone_y);
    }
    else {
        // Обычное перемещение камня
        state->field[new_stone_y][new_stone_x] = '@';
        state->field[stone_y][stone_x] = '_';
        // Переносим цвет на новую позицию
        state->color_field[new_stone_y][new_stone_x] = stone_color;
        state->color_field[stone_y][stone_x] = '_';
        printf("Толкнули камень с (%d,%d) на (%d,%d)\n", stone_x, stone_y, new_stone_x, new_stone_y);
    }
}

// Функция для создания демо-файла с прыжками через границы
void create_demo_program() {
    FILE* file = fopen("program.txt", "r");
    if (file) {
        fclose(file);
        return;
    }

    file = fopen("program.txt", "w");
    if (!file) {
        printf("Ошибка: Не удалось создать файл program.txt\n");
        return;
    }

    // Демо-программа с прыжками через границы
    fprintf(file, "// ДЕМО ПРОГРАММА С ПРЫЖКАМИ ЧЕРЕЗ ГРАНИЦЫ\n");
    fprintf(file, "SIZE 8 6\n");
    fprintf(file, "START 0 0\n");
    fprintf(file, "\n");
    fprintf(file, "// Прыжок через правую границу\n");
    fprintf(file, "JUMP RIGHT 10\n");
    fprintf(file, "PAINT jumped_right\n");
    fprintf(file, "\n");
    fprintf(file, "// Прыжок через нижнюю границу\n");
    fprintf(file, "JUMP DOWN 8\n");
    fprintf(file, "PAINT jumped_down\n");
    fprintf(file, "\n");
    fprintf(file, "// Прыжок через левую границу\n");
    fprintf(file, "JUMP LEFT 5\n");
    fprintf(file, "PAINT jumped_left\n");
    fprintf(file, "\n");
    fprintf(file, "// Прыжок через верхнюю границу\n");
    fprintf(file, "JUMP UP 3\n");
    fprintf(file, "PAINT jumped_up\n");
    fprintf(file, "\n");
    fprintf(file, "// Создаем гору для демонстрации прерывания прыжка\n");
    fprintf(file, "MOUND RIGHT\n");
    fprintf(file, "MOUND RIGHT 2\n");
    fprintf(file, "JUMP RIGHT 3\n");
    fprintf(file, "PAINT mountain_block\n");
    fprintf(file, "\n");
    fprintf(file, "// Большой прыжок по диагонали через углы\n");
    fprintf(file, "JUMP RIGHT 15\n");
    fprintf(file, "JUMP DOWN 10\n");
    fprintf(file, "PAINT big_jump\n");
    fprintf(file, "\n");
    fprintf(file, "// Создаем яму и пытаемся прыгнуть в нее\n");
    fprintf(file, "DIG DOWN\n");
    fprintf(file, "MOVE UP\n");
    fprintf(file, "// JUMP DOWN 2  // Раскомментируйте для теста ошибки\n");
    fprintf(file, "PAINT safe\n");
    fprintf(file, "\n");
    fprintf(file, "// Финальная позиция\n");
    fprintf(file, "PAINT end\n");

    fclose(file);
    printf("Создан демо-файл program.txt с командами прыжков\n");
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
        error_exit(state, "Отступы в начале строки запрещены");
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

    printf("Выполняется: %s\n", trimmed);

    // === ПРОВЕРКА КОМАНД ===

    if (strcmp(command, "SIZE") == 0) {
        if (state->size_set) {
            error_exit(state, "Команда SIZE может использоваться только один раз");
        }
        if (parsed != 3) {
            error_exit(state, "Команда SIZE требует ширину и высоту");
        }

        int width = atoi(param1);
        int height = atoi(param2);

        if (width < MIN_SIZE || width > MAX_WIDTH || height < MIN_SIZE || height > MAX_HEIGHT) {
            error_exit(state, "Размеры поля должны быть между 10x10 и 100x100");
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

        display_field(state, "SIZE - Инициализация поля");
        wait_for_enter(NULL);
    }
    else if (strcmp(command, "START") == 0) {
        if (!state->size_set) {
            error_exit(state, "SIZE должен быть установлен перед START");
        }
        if (state->start_set) {
            error_exit(state, "Команда START может использоваться только один раз");
        }
        if (parsed != 3) {
            error_exit(state, "Команда START требует координаты x и y");
        }

        int x = atoi(param1);
        int y = atoi(param2);

        if (x < 0 || x >= state->width || y < 0 || y >= state->height) {
            error_exit(state, "Стартовая позиция вне границ поля");
        }

        state->dino_x = x;
        state->dino_y = y;
        state->field[y][x] = '#';
        state->start_set = 1;

        display_field(state, "START - Размещение динозавра");
        wait_for_enter(NULL);
    }
    else if (strcmp(command, "JUMP") == 0) {
        if (!state->size_set || !state->start_set) {
            error_exit(state, "SIZE и START должны быть установлены перед JUMP");
        }
        if (parsed != 3) {
            error_exit(state, "Команда JUMP требует направление и дистанцию");
        }
        if (!is_valid_direction(param1)) {
            error_exit(state, "Неверное направление. Используйте: UP, DOWN, LEFT, RIGHT");
        }

        int distance = atoi(param2);
        if (distance <= 0) {
            error_exit(state, "Дистанция JUMP должна быть положительным целым числом");
        }

        jump_dinosaur(state, param1, distance);

        char display_cmd[100];
        sprintf(display_cmd, "JUMP %s %d", param1, distance);
        display_field(state, display_cmd);
        wait_for_enter(NULL);
    }
    else if (strcmp(command, "MOVE") == 0) {
        if (!state->size_set || !state->start_set) {
            error_exit(state, "SIZE и START должны быть установлены перед командами движения");
        }
        if (parsed != 2) {
            error_exit(state, "Команда MOVE требует направление");
        }
        if (!is_valid_direction(param1)) {
            error_exit(state, "Неверное направление. Используйте: UP, DOWN, LEFT, RIGHT");
        }

        int target_x, target_y;
        get_target_cell(state, param1, 1, &target_x, &target_y);

        // Применяем тороидальную топологию
        target_x = (target_x % state->width + state->width) % state->width;
        target_y = (target_y % state->height + state->height) % state->height;

        char target_cell = state->field[target_y][target_x];

        if (target_cell == '%') {
            error_exit(state, "Динозавр упал в яму! Игра окончена.");
        }

        if (is_obstacle(target_cell)) {
            printf("Предупреждение: Нельзя переместиться на препятствие. Команда пропущена.\n");
            display_field(state, "MOVE - БЛОКИРОВАНО");
            wait_for_enter(NULL);
            return;
        }

        // Выполняем перемещение
        state->field[state->dino_y][state->dino_x] = '_';
        state->dino_x = target_x;
        state->dino_y = target_y;
        state->field[target_y][target_x] = '#';

        char display_cmd[100];
        sprintf(display_cmd, "MOVE %s", param1);
        display_field(state, display_cmd);
        wait_for_enter(NULL);
    }
    else if (strcmp(command, "GROW") == 0) {
        if (!state->size_set || !state->start_set) {
            error_exit(state, "SIZE и START должны быть установлены перед GROW");
        }
        if (parsed != 2) {
            error_exit(state, "Команда GROW требует направление");
        }
        if (!is_valid_direction(param1)) {
            error_exit(state, "Неверное направление. Используйте: UP, DOWN, LEFT, RIGHT");
        }

        grow_tree(state, param1);

        char display_cmd[100];
        sprintf(display_cmd, "GROW %s", param1);
        display_field(state, display_cmd);
        wait_for_enter(NULL);
    }
    else if (strcmp(command, "CUT") == 0) {
        if (!state->size_set || !state->start_set) {
            error_exit(state, "SIZE и START должны быть установлены перед CUT");
        }
        if (parsed != 2) {
            error_exit(state, "Команда CUT требует направление");
        }
        if (!is_valid_direction(param1)) {
            error_exit(state, "Неверное направление. Используйте: UP, DOWN, LEFT, RIGHT");
        }

        cut_tree(state, param1);

        char display_cmd[100];
        sprintf(display_cmd, "CUT %s", param1);
        display_field(state, display_cmd);
        wait_for_enter(NULL);
    }
    else if (strcmp(command, "MAKE") == 0) {
        if (!state->size_set || !state->start_set) {
            error_exit(state, "SIZE и START должны быть установлены перед MAKE");
        }
        if (parsed != 2) {
            error_exit(state, "Команда MAKE требует направление");
        }
        if (!is_valid_direction(param1)) {
            error_exit(state, "Неверное направление. Используйте: UP, DOWN, LEFT, RIGHT");
        }

        make_stone(state, param1);

        char display_cmd[100];
        sprintf(display_cmd, "MAKE %s", param1);
        display_field(state, display_cmd);
        wait_for_enter(NULL);
    }
    else if (strcmp(command, "PUSH") == 0) {
        if (!state->size_set || !state->start_set) {
            error_exit(state, "SIZE и START должны быть установлены перед PUSH");
        }
        if (parsed != 2) {
            error_exit(state, "Команда PUSH требует направление");
        }
        if (!is_valid_direction(param1)) {
            error_exit(state, "Неверное направление. Используйте: UP, DOWN, LEFT, RIGHT");
        }

        push_stone(state, param1);

        char display_cmd[100];
        sprintf(display_cmd, "PUSH %s", param1);
        display_field(state, display_cmd);
        wait_for_enter(NULL);
    }
    else if (strcmp(command, "PAINT") == 0) {
        if (!state->size_set || !state->start_set) {
            error_exit(state, "SIZE и START должны быть установлены перед PAINT");
        }
        if (parsed != 2) {
            error_exit(state, "Команда PAINT требует букву");
        }
        if (!is_valid_letter(param1)) {
            error_exit(state, "PAINT требует одну строчную букву (a-z)");
        }

        state->color_field[state->dino_y][state->dino_x] = param1[0];

        char display_cmd[100];
        sprintf(display_cmd, "PAINT %s", param1);
        display_field(state, display_cmd);
        wait_for_enter(NULL);
    }
    else if (strcmp(command, "DIG") == 0) {
        if (!state->size_set || !state->start_set) {
            error_exit(state, "SIZE и START должны быть установлены перед DIG");
        }
        if (parsed != 2) {
            error_exit(state, "Команда DIG требует направление");
        }
        if (!is_valid_direction(param1)) {
            error_exit(state, "Неверное направление. Используйте: UP, DOWN, LEFT, RIGHT");
        }

        int target_x, target_y;
        get_target_cell(state, param1, 1, &target_x, &target_y);

        // Применяем тороидальную топологию
        target_x = (target_x % state->width + state->width) % state->width;
        target_y = (target_y % state->height + state->height) % state->height;

        // Проверяем, что не копаем под динозавра
        if (target_x == state->dino_x && target_y == state->dino_y) {
            printf("Предупреждение: Нельзя копать под динозавром. Команда пропущена.\n");
            display_field(state, "DIG - БЛОКИРОВАНО");
            wait_for_enter(NULL);
            return;
        }

        char current_cell = state->field[target_y][target_x];

        if (current_cell == '^') {
            state->field[target_y][target_x] = '_';
            printf("Гора заполнена ямой. Клетка теперь пуста.\n");
        }
        else if (current_cell == '_') {
            state->field[target_y][target_x] = '%';
        }
        else {
            printf("Предупреждение: Нельзя копать на непустой клетке. Команда пропущена.\n");
        }

        char display_cmd[100];
        sprintf(display_cmd, "DIG %s", param1);
        display_field(state, display_cmd);
        wait_for_enter(NULL);
    }
    else if (strcmp(command, "MOUND") == 0) {
        if (!state->size_set || !state->start_set) {
            error_exit(state, "SIZE и START должны быть установлены перед MOUND");
        }
        if (parsed != 2) {
            error_exit(state, "Команда MOUND требует направление");
        }
        if (!is_valid_direction(param1)) {
            error_exit(state, "Неверное направление. Используйте: UP, DOWN, LEFT, RIGHT");
        }

        int target_x, target_y;
        get_target_cell(state, param1, 1, &target_x, &target_y);

        // Применяем тороидальную топологию
        target_x = (target_x % state->width + state->width) % state->width;
        target_y = (target_y % state->height + state->height) % state->height;

        // Проверяем, что не насыпаем на динозавра
        if (target_x == state->dino_x && target_y == state->dino_y) {
            printf("Предупреждение: Нельзя создавать насыпь на динозавре. Команда пропущена.\n");
            display_field(state, "MOUND - БЛОКИРОВАНО");
            wait_for_enter(NULL);
            return;
        }

        char current_cell = state->field[target_y][target_x];

        if (current_cell == '%') {
            state->field[target_y][target_x] = '_';
            printf("Яма заполнена насыпью. Клетка теперь пуста.\n");
        }
        else if (current_cell == '_') {
            state->field[target_y][target_x] = '^';
        }
        else {
            printf("Предупреждение: Нельзя создавать насыпь на непустой клетке. Команда пропущена.\n");
        }

        char display_cmd[100];
        sprintf(display_cmd, "MOUND %s", param1);
        display_field(state, display_cmd);
        wait_for_enter(NULL);
    }
    else {
        error_exit(state, "Неизвестная команда");
    }
}

int main() {
    printf("=== ИНТЕРПРЕТАТОР MOVDINO С ФУНКЦИЕЙ ПРЫЖКА ===\n");
    printf("Поиск файла program.txt...\n");

    create_demo_program();

    FILE* file = fopen("program.txt", "r");
    if (!file) {
        printf("Ошибка: Не удалось открыть файл program.txt\n");
        printf("Убедитесь, что program.txt находится в той же папке\n");
        wait_for_enter("Нажмите Enter для выхода...");
        return 1;
    }

    printf("Файл program.txt найден! Нажмите Enter для начала выполнения...");
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

    printf("\n=== ПРОГРАММА УСПЕШНО ЗАВЕРШЕНА ===\n");
    printf("Финальная позиция: (%d, %d)\n", state.dino_x, state.dino_y);
    printf("Размер поля: %dx%d\n", state.width, state.height);
    wait_for_enter("Нажмите Enter для выхода...");

    return 0;
}