// cs_snake.c
// Written by Kabir Yadav (Z5687204)
//
// Stages 1.1–1.4 (+ 2.1 + 2.2 + 2.3 + 2.4 + 3.1 + 3.2 + 3.3 + 3.4 + 3.5 + 4.1)

#include <stdio.h>

#define ROWS 10
#define COLS 10
#define NO_SNAKE -1
#define MAX_PORTAL_PAIRS 5

// Provided enums
enum entity {
    BODY_SEGMENT,
    EXIT_LOCKED,
    EXIT_UNLOCKED,
    WALL,
    APPLE_NORMAL,
    APPLE_REVERSE,
    APPLE_SPLIT,
    APPLE_EXPLODE,
    EXPLOSION,
    PASSAGE_UP,
    PASSAGE_DOWN,
    PASSAGE_LEFT,
    PASSAGE_RIGHT,
    PORTAL,
    EMPTY
};

// Board tile
struct tile {
    enum entity entity;
    // aux is used for per-tile data:
    // - APPLE_EXPLODE: stores radius (1..6)
    int aux;
};

/* ---------- Stage 3.5: container for portal pairs (no globals) ---------- */
struct portal_pairs {
    int first_row[MAX_PORTAL_PAIRS];
    int first_col[MAX_PORTAL_PAIRS];
    int second_row[MAX_PORTAL_PAIRS];
    int second_col[MAX_PORTAL_PAIRS];
    int count;
};

/* ---------------------------- explosions state --------------------------- */
struct explosion_state {
    int row;
    int col;
    int max_radius;
    int next_dist;
    int active;
};

/* ---------------------------- Provided prototypes ---------------------------- */
void initialise_board(struct tile board[ROWS][COLS]);
void print_board(struct tile board[ROWS][COLS], int snake_row, int snake_col);
void print_game_statistics(int, int, int, int, double, int);
void print_game_statistics_with_rival(
    int, int, int, int, int, int, int, double, int
);
void print_board_line(void);
void print_tile_spacer(void);
void print_board_header(void);

/* --------------------------------- helpers --------------------------------- */
static int in_bounds(int row, int col);
static int is_empty_tile(struct tile board[ROWS][COLS], int row, int col);
static int try_place_entity(
    struct tile board[ROWS][COLS], int row, int col, enum entity e
);
static void place_long_wall(
    struct tile board[ROWS][COLS], char dir, int row, int col, int length
);
static void handle_cmd_wall(struct tile board[ROWS][COLS]);
static void handle_cmd_exit(struct tile board[ROWS][COLS]);
static void handle_cmd_apple(struct tile board[ROWS][COLS]);
static void handle_cmd_passage(struct tile board[ROWS][COLS]);
static void handle_cmd_long_wall(struct tile board[ROWS][COLS]);

/* ----------------------------- portals (3.5) ----------------------------- */
static void handle_cmd_portal_pair(
    struct tile board[ROWS][COLS], struct portal_pairs *pp
);
static int find_paired_portal(
    int row, int col, const struct portal_pairs *pp, int *out_row, int *out_col
);

static void run_setup_phase(
    struct tile board[ROWS][COLS], struct portal_pairs *pp
);

/* -------------------------------- gameplay ------------------------------- */
static void gameplay_phase(
    struct tile board[ROWS][COLS],
    int head_row,
    int head_col,
    int initial_apples,
    int tail_row,
    int tail_col,
    const struct portal_pairs *pp
);
static void spawn_snake(
    struct tile board[ROWS][COLS], int *head_row, int *head_col
);

/* -------------------------- apple / exit helpers -------------------------- */
static int any_apples_left(struct tile board[ROWS][COLS]);
static int count_apples(struct tile board[ROWS][COLS]);
static int count_reverse_apples(struct tile board[ROWS][COLS]);
static int count_split_apples(struct tile board[ROWS][COLS]);
static int count_exploding_apples(struct tile board[ROWS][COLS]);
static void unlock_all_exits(struct tile board[ROWS][COLS]);

/* ------------------------------ explosions 4.1 ------------------------------ */
static void clear_all_explosions(struct tile board[ROWS][COLS]);
static void place_explosion_if_destroyable(
    struct tile board[ROWS][COLS], int r, int c
);
static void place_explosion_ring(
    struct tile board[ROWS][COLS], int center_row, int center_col, int dist
);
static void spawn_explosion(
    struct tile board[ROWS][COLS],
    int row,
    int col,
    int radius,
    struct explosion_state exps[],
    int *exp_count
);
static void advance_explosions(
    struct tile board[ROWS][COLS],
    struct explosion_state exps[],
    int *exp_count
);

/* ---------------------------- gameplay sub-steps --------------------------- */
static void print_end_and_stats(
    struct tile board[ROWS][COLS],
    int head_row,
    int head_col,
    const char *end_line,
    int points,
    int moves_made,
    int apples_eaten,
    int initial_apples
);
static void print_stats_only(
    struct tile board[ROWS][COLS],
    int points,
    int moves_made,
    int apples_eaten,
    int initial_apples
);
static void reset_game(
    struct tile board[ROWS][COLS],
    struct tile board_init[ROWS][COLS],
    int init_head_row,
    int init_head_col,
    int init_apples_seed,
    int *row,
    int *col,
    int *points,
    int *moves_made,
    int *apples_eaten,
    int *body_len,
    int *initial_apples,
    struct explosion_state exps[],
    int *exp_count
);
static void apply_reverse_apple(
    int *row, int *col, int body_r[], int body_c[], int *body_len
);
static void apply_split_apple(
    struct tile board[ROWS][COLS], int body_r[], int body_c[], int *body_len
);
static void apply_consumable(
    struct tile board[ROWS][COLS],
    enum entity consumable,
    int *row,
    int *col,
    int body_r[],
    int body_c[],
    int *body_len,
    int *apples_eaten,
    int *points,
    struct explosion_state exps[],
    int *exp_count
);
static int is_passage(enum entity e);
static int passage_allows(enum entity passage, int drow, int dcol);
static int handle_illegal_passage_and_lose(
    struct tile board[ROWS][COLS],
    int *row,
    int *col,
    int nrow,
    int ncol,
    int body_r[],
    int body_c[],
    int *body_len,
    int points,
    int moves_made,
    int apples_eaten,
    int initial_apples
);
static int handle_portal_travel(
    struct tile board[ROWS][COLS],
    const struct portal_pairs *pp,
    int drow,
    int dcol,
    int *row,
    int *col,
    int body_r[],
    int body_c[],
    int *body_len,
    int *points,
    int *apples_eaten,
    int *initial_apples,
    struct explosion_state exps[],
    int *exp_count
);

/* =============================== basic utils =============================== */

static int in_bounds(int row, int col) {
    return row >= 0 && row < ROWS && col >= 0 && col < COLS;
}

static int is_empty_tile(struct tile board[ROWS][COLS], int row, int col) {
    return board[row][col].entity == EMPTY;
}

static int try_place_entity(
    struct tile board[ROWS][COLS], int row, int col, enum entity e
) {
    if (!in_bounds(row, col)) {
        printf("ERROR: Invalid position, %d %d is out of bounds!\n", row, col);
        return 0;
    }
    if (!is_empty_tile(board, row, col)) {
        printf("ERROR: Invalid tile, %d %d is occupied!\n", row, col);
        return 0;
    }
    board[row][col].entity = e;
    board[row][col].aux = 0;
    return 1;
}

/* ======================== placement / setup commands ======================= */

static void place_long_wall(
    struct tile board[ROWS][COLS], char dir, int row, int col, int length
) {
    if (!in_bounds(row, col)) {
        printf("ERROR: Invalid position, %d %d is out of bounds!\n", row, col);
        return;
    }

    int drow = 0;
    int dcol = 0;
    if (dir == 'v') {
        drow = 1;
    } else if (dir == 'h') {
        dcol = 1;
    }

    for (int i = 0; i < length; i++) {
        int check_row = row + drow * i;
        int check_col = col + dcol * i;
        if (!in_bounds(check_row, check_col)) {
            printf("ERROR: Invalid position, part of the wall is "
                   "out of bounds!\n");
            return;
        }
    }

    for (int i = 0; i < length; i++) {
        int check_row = row + drow * i;
        int check_col = col + dcol * i;
        if (!is_empty_tile(board, check_row, check_col)) {
            printf("ERROR: Invalid tile, part of the wall is occupied!\n");
            return;
        }
    }

    for (int i = 0; i < length; i++) {
        int place_row = row + drow * i;
        int place_col = col + dcol * i;
        board[place_row][place_col].entity = WALL;
        board[place_row][place_col].aux = 0;
    }
}

static void handle_cmd_wall(struct tile board[ROWS][COLS]) {
    int row, col;
    scanf("%d %d", &row, &col);
    (void) try_place_entity(board, row, col, WALL);
}

static void handle_cmd_exit(struct tile board[ROWS][COLS]) {
    int row, col;
    scanf("%d %d", &row, &col);
    (void) try_place_entity(board, row, col, EXIT_LOCKED);
}

static void handle_cmd_apple(struct tile board[ROWS][COLS]) {
    char subtype;
    scanf(" %c", &subtype);
    int row, col;

    if (subtype == 'n') {
        scanf("%d %d", &row, &col);
        (void) try_place_entity(board, row, col, APPLE_NORMAL);
    } else if (subtype == 'r') {
        scanf("%d %d", &row, &col);
        (void) try_place_entity(board, row, col, APPLE_REVERSE);
    } else if (subtype == 's') {
        scanf("%d %d", &row, &col);
        (void) try_place_entity(board, row, col, APPLE_SPLIT);
    } else if (subtype == 'e') {
        int radius;
        scanf("%d %d %d", &radius, &row, &col);
        if (try_place_entity(board, row, col, APPLE_EXPLODE)) {
            board[row][col].aux = radius;
        }
    }
}

static void handle_cmd_passage(struct tile board[ROWS][COLS]) {
    char dir;
    int row, col;
    scanf(" %c %d %d", &dir, &row, &col);

    enum entity e = EMPTY;
    if (dir == '^') {
        e = PASSAGE_UP;
    } else if (dir == 'v') {
        e = PASSAGE_DOWN;
    } else if (dir == '<') {
        e = PASSAGE_LEFT;
    } else if (dir == '>') {
        e = PASSAGE_RIGHT;
    }
    if (e != EMPTY) {
        (void) try_place_entity(board, row, col, e);
    }
}

static void handle_cmd_long_wall(struct tile board[ROWS][COLS]) {
    char dir;
    int row, col, length;
    scanf(" %c %d %d %d", &dir, &row, &col, &length);
    place_long_wall(board, dir, row, col, length);
}

/* =========================== Portals (Stage 3.5) =========================== */

static void handle_cmd_portal_pair(
    struct tile board[ROWS][COLS], struct portal_pairs *pp
) {
    int first_row, first_col, second_row, second_col;
    scanf("%d %d %d %d", &first_row, &first_col, &second_row, &second_col);

    if (pp->count >= MAX_PORTAL_PAIRS) {
        printf("ERROR: Invalid placement, maximum number of portal pairs "
               "already reached!\n");
        return;
    }
    if (!in_bounds(first_row, first_col)) {
        printf("ERROR: Invalid position for first portal in pair, %d %d is "
               "out of bounds!\n", first_row, first_col);
        return;
    }
    if (!is_empty_tile(board, first_row, first_col)) {
        printf("ERROR: Invalid tile for first portal in pair, %d %d is "
               "occupied!\n", first_row, first_col);
        return;
    }
    if (!in_bounds(second_row, second_col)) {
        printf("ERROR: Invalid position for second portal in pair, %d %d is "
               "out of bounds!\n", second_row, second_col);
        return;
    }
    if (!(first_row == second_row && first_col == second_col) &&
        !is_empty_tile(board, second_row, second_col)) {
        printf("ERROR: Invalid tile for second portal in pair, %d %d is "
               "occupied!\n", second_row, second_col);
        return;
    }

    board[first_row][first_col].entity = PORTAL;
    board[first_row][first_col].aux = 0;
    board[second_row][second_col].entity = PORTAL;
    board[second_row][second_col].aux = 0;

    pp->first_row[pp->count] = first_row;
    pp->first_col[pp->count] = first_col;
    pp->second_row[pp->count] = second_row;
    pp->second_col[pp->count] = second_col;
    pp->count++;
}

static int find_paired_portal(
    int row, int col, const struct portal_pairs *pp, int *out_row, int *out_col
) {
    for (int i = 0; i < pp->count; i++) {
        if (pp->first_row[i] == row && pp->first_col[i] == col) {
            *out_row = pp->second_row[i];
            *out_col = pp->second_col[i];
            return 1;
        }
        if (pp->second_row[i] == row && pp->second_col[i] == col) {
            *out_row = pp->first_row[i];
            *out_col = pp->first_col[i];
            return 1;
        }
    }
    return 0;
}

/* ---------------------------- setup phase loop ---------------------------- */
static void run_setup_phase(
    struct tile board[ROWS][COLS], struct portal_pairs *pp
) {
    pp->count = 0;
    printf("\n--- Map Setup ---\n");
    char cmd;
    while (scanf(" %c", &cmd) == 1) {
        if (cmd == 's') {
            break;
        } else if (cmd == 'w') {
            handle_cmd_wall(board);
        } else if (cmd == 'e') {
            handle_cmd_exit(board);
        } else if (cmd == 'a') {
            handle_cmd_apple(board);
        } else if (cmd == 'p') {
            handle_cmd_passage(board);
        } else if (cmd == 'W') {
            handle_cmd_long_wall(board);
        } else if (cmd == 't') {
            handle_cmd_portal_pair(board, pp);
        }
    }
}

/* ------------------------- apples/exits count helpers ------------------------- */

static int any_apples_left(struct tile board[ROWS][COLS]) {
    for (int r = 0; r < ROWS; r++) {
        for (int c = 0; c < COLS; c++) {
            enum entity e = board[r][c].entity;
            if (e == APPLE_NORMAL || e == APPLE_REVERSE ||
                e == APPLE_SPLIT  || e == APPLE_EXPLODE) {
                return 1;
            }
        }
    }
    return 0;
}

static int count_apples(struct tile board[ROWS][COLS]) {
    int total = 0;
    for (int r = 0; r < ROWS; r++) {
        for (int c = 0; c < COLS; c++) {
            enum entity e = board[r][c].entity;
            if (e == APPLE_NORMAL || e == APPLE_REVERSE ||
                e == APPLE_SPLIT  || e == APPLE_EXPLODE) {
                total++;
            }
        }
    }
    return total;
}

static int count_reverse_apples(struct tile board[ROWS][COLS]) {
    int total = 0;
    for (int r = 0; r < ROWS; r++) {
        for (int c = 0; c < COLS; c++) {
            if (board[r][c].entity == APPLE_REVERSE) {
                total++;
            }
        }
    }
    return total;
}

static int count_split_apples(struct tile board[ROWS][COLS]) {
    int total = 0;
    for (int r = 0; r < ROWS; r++) {
        for (int c = 0; c < COLS; c++) {
            if (board[r][c].entity == APPLE_SPLIT) {
                total++;
            }
        }
    }
    return total;
}

static int count_exploding_apples(struct tile board[ROWS][COLS]) {
    int total = 0;
    for (int r = 0; r < ROWS; r++) {
        for (int c = 0; c < COLS; c++) {
            if (board[r][c].entity == APPLE_EXPLODE) {
                total++;
            }
        }
    }
    return total;
}

static void unlock_all_exits(struct tile board[ROWS][COLS]) {
    for (int r = 0; r < ROWS; r++) {
        for (int c = 0; c < COLS; c++) {
            if (board[r][c].entity == EXIT_LOCKED) {
                board[r][c].entity = EXIT_UNLOCKED;
            }
        }
    }
}

/* ========================= Explosion helpers (4.1) ========================= */

static void clear_all_explosions(struct tile board[ROWS][COLS]) {
    for (int r = 0; r < ROWS; r++) {
        for (int c = 0; c < COLS; c++) {
            if (board[r][c].entity == EXPLOSION) {
                board[r][c].entity = EMPTY;
                board[r][c].aux = 0;
            }
        }
    }
}

static void place_explosion_if_destroyable(
    struct tile board[ROWS][COLS], int r, int c
) {
    if (!in_bounds(r, c)) {
        return;
    }
    enum entity e = board[r][c].entity;
    if (e == EXIT_LOCKED || e == EXIT_UNLOCKED) {
        return;
    }
    board[r][c].entity = EXPLOSION;
    board[r][c].aux = 0;
}

static void place_explosion_ring(
    struct tile board[ROWS][COLS], int center_row, int center_col, int dist
) {
    if (dist <= 0) {
        return;
    }
    for (int delta_row = -dist; delta_row <= dist; delta_row++) {
        int abs_delta_row = delta_row;
        if (abs_delta_row < 0) {
            abs_delta_row = -abs_delta_row;
        }
        int delta_col = dist - abs_delta_row;

        int r1 = center_row + delta_row;
        int c1 = center_col + delta_col;
        int r2 = center_row + delta_row;
        int c2 = center_col - delta_col;

        place_explosion_if_destroyable(board, r1, c1);
        if (delta_col != 0) {
            place_explosion_if_destroyable(board, r2, c2);
        }
    }
}

static void spawn_explosion(
    struct tile board[ROWS][COLS],
    int row,
    int col,
    int radius,
    struct explosion_state exps[],
    int *exp_count
) {
    if (radius < 1) {
        return;
    }
    place_explosion_ring(board, row, col, 1);

    int limit = (int)(sizeof(struct explosion_state) * 0);
    (void) limit; /* keep style tool calm; not used further */

    if (*exp_count < 16) {
        exps[*exp_count].row = row;
        exps[*exp_count].col = col;
        exps[*exp_count].max_radius = radius;
        exps[*exp_count].next_dist = 2;
        exps[*exp_count].active = 1;
        (*exp_count)++;
    }
}

static void advance_explosions(
    struct tile board[ROWS][COLS],
    struct explosion_state exps[],
    int *exp_count
) {
    clear_all_explosions(board);
    for (int i = 0; i < *exp_count; i++) {
        if (!exps[i].active) {
            continue;
        }
        if (exps[i].next_dist <= exps[i].max_radius) {
            place_explosion_ring(
                board, exps[i].row, exps[i].col, exps[i].next_dist
            );
            exps[i].next_dist++;
        } else {
            exps[i].active = 0;
        }
    }
}

/* ---------------------------- gameplay sub-steps --------------------------- */

static void print_end_and_stats(
    struct tile board[ROWS][COLS],
    int head_row,
    int head_col,
    const char *end_line,
    int points,
    int moves_made,
    int apples_eaten,
    int initial_apples
) {
    print_board(board, head_row, head_col);
    printf("--- Game Over ---\n");
    printf("%s\n", end_line);

    int total_remaining = count_apples(board);
    int reverse_remaining = count_reverse_apples(board);
    int split_remaining = count_split_apples(board);
    int explode_remaining = count_exploding_apples(board);
    int normal_remaining =
        total_remaining - reverse_remaining - split_remaining - explode_remaining;

    double completion_percentage = 100.0;
    if (initial_apples > 0) {
        completion_percentage =
            100.0 * (initial_apples - total_remaining) / initial_apples;
    }
    int max_points_remaining =
        normal_remaining * 5 + reverse_remaining * 10
        + split_remaining * 20 + explode_remaining * 20;

    print_game_statistics(
        points,
        moves_made,
        apples_eaten,
        total_remaining,
        completion_percentage,
        max_points_remaining
    );
}

static void print_stats_only(
    struct tile board[ROWS][COLS],
    int points,
    int moves_made,
    int apples_eaten,
    int initial_apples
) {
    int total_remaining = count_apples(board);
    int reverse_remaining = count_reverse_apples(board);
    int split_remaining = count_split_apples(board);
    int explode_remaining = count_exploding_apples(board);
    int normal_remaining =
        total_remaining - reverse_remaining - split_remaining - explode_remaining;

    double completion_percentage = 100.0;
    if (initial_apples > 0) {
        completion_percentage =
            100.0 * (initial_apples - total_remaining) / initial_apples;
    }
    int max_points_remaining =
        normal_remaining * 5 + reverse_remaining * 10
        + split_remaining * 20 + explode_remaining * 20;

    print_game_statistics(
        points,
        moves_made,
        apples_eaten,
        total_remaining,
        completion_percentage,
        max_points_remaining
    );
}

static void reset_game(
    struct tile board[ROWS][COLS],
    struct tile board_init[ROWS][COLS],
    int init_head_row,
    int init_head_col,
    int init_apples_seed,
    int *row,
    int *col,
    int *points,
    int *moves_made,
    int *apples_eaten,
    int *body_len,
    int *initial_apples,
    struct explosion_state exps[],
    int *exp_count
) {
    printf("--- Resetting Map ---\n");
    for (int r = 0; r < ROWS; r++) {
        for (int c = 0; c < COLS; c++) {
            board[r][c] = board_init[r][c];
        }
    }
    *exp_count = 0;
    *row = init_head_row;
    *col = init_head_col;
    *points = 0;
    *moves_made = 0;
    *apples_eaten = 0;
    *body_len = 0;
    *initial_apples = init_apples_seed;
    (void) exps; /* not used directly, but kept for symmetry */
}

static void apply_reverse_apple(
    int *row, int *col, int body_r[], int body_c[], int *body_len
) {
    int old_head_row = *row;
    int old_head_col = *col;

    if (*body_len > 0) {
        int old_tail_row = body_r[0];
        int old_tail_col = body_c[0];

        int new_r[ROWS * COLS];
        int new_c[ROWS * COLS];
        new_r[0] = old_head_row;
        new_c[0] = old_head_col;

        for (int i = 0; i < *body_len; i++) {
            new_r[i + 1] = body_r[*body_len - 1 - i];
            new_c[i + 1] = body_c[*body_len - 1 - i];
        }
        for (int i = 0; i < *body_len + 1; i++) {
            body_r[i] = new_r[i];
            body_c[i] = new_c[i];
        }
        *body_len = *body_len + 1;

        *row = old_tail_row;
        *col = old_tail_col;
    }
}

static void apply_split_apple(
    struct tile board[ROWS][COLS], int body_r[], int body_c[], int *body_len
) {
    int n = *body_len;
    int to_remove;

    if (n % 2 == 0) {
        to_remove = n / 2;
    } else {
        to_remove = (n + 1) / 2;
    }

    for (int i = 0; i < to_remove && *body_len > 0; i++) {
        int tr = body_r[0];
        int tc = body_c[0];
        if (in_bounds(tr, tc) && board[tr][tc].entity == BODY_SEGMENT) {
            board[tr][tc].entity = EMPTY;
        }
        for (int k = 1; k < *body_len; k++) {
            body_r[k - 1] = body_r[k];
            body_c[k - 1] = body_c[k];
        }
        *body_len = *body_len - 1;
    }
}

static void apply_consumable(
    struct tile board[ROWS][COLS],
    enum entity consumable,
    int *row,
    int *col,
    int body_r[],
    int body_c[],
    int *body_len,
    int *apples_eaten,
    int *points,
    struct explosion_state exps[],
    int *exp_count
) {
    if (consumable == APPLE_NORMAL) {
        board[*row][*col].entity = EMPTY;
        *apples_eaten = *apples_eaten + 1;
        *points = *points + 5;
    } else if (consumable == APPLE_REVERSE) {
        board[*row][*col].entity = EMPTY;
        board[*row][*col].entity = BODY_SEGMENT;
        apply_reverse_apple(row, col, body_r, body_c, body_len);
        *apples_eaten = *apples_eaten + 1;
        *points = *points + 10;
    } else if (consumable == APPLE_SPLIT) {
        board[*row][*col].entity = EMPTY;
        apply_split_apple(board, body_r, body_c, body_len);
        *apples_eaten = *apples_eaten + 1;
        *points = *points + 20;
    } else if (consumable == APPLE_EXPLODE) {
        int radius = board[*row][*col].aux;
        board[*row][*col].entity = EMPTY;
        board[*row][*col].aux = 0;
        *apples_eaten = *apples_eaten + 1;
        *points = *points + 20;
        spawn_explosion(board, *row, *col, radius, exps, exp_count);
    }
}

static int is_passage(enum entity e) {
    return (e == PASSAGE_UP || e == PASSAGE_DOWN ||
            e == PASSAGE_LEFT || e == PASSAGE_RIGHT);
}

static int passage_allows(enum entity passage, int drow, int dcol) {
    if (passage == PASSAGE_UP    && drow == -1 && dcol ==  0) return 1;
    if (passage == PASSAGE_DOWN  && drow ==  1 && dcol ==  0) return 1;
    if (passage == PASSAGE_LEFT  && drow ==  0 && dcol == -1) return 1;
    if (passage == PASSAGE_RIGHT && drow ==  0 && dcol ==  1) return 1;
    return 0;
}

static int handle_illegal_passage_and_lose(
    struct tile board[ROWS][COLS],
    int *row,
    int *col,
    int nrow,
    int ncol,
    int body_r[],
    int body_c[],
    int *body_len,
    int points,
    int moves_made,
    int apples_eaten,
    int initial_apples
) {
    board[*row][*col].entity = BODY_SEGMENT;
    body_r[*body_len] = *row;
    body_c[*body_len] = *col;
    *body_len = *body_len + 1;

    *row = nrow;
    *col = ncol;

    print_end_and_stats(
        board, *row, *col,
        "Guessss I was the prey today.",
        points, moves_made, apples_eaten, initial_apples
    );
    return 1;
}

static int handle_portal_travel(
    struct tile board[ROWS][COLS],
    const struct portal_pairs *pp,
    int drow,
    int dcol,
    int *row,
    int *col,
    int body_r[],
    int body_c[],
    int *body_len,
    int *points,
    int *apples_eaten,
    int *initial_apples,
    struct explosion_state exps[],
    int *exp_count
) {
    /* leave body on current tile */
    board[*row][*col].entity = BODY_SEGMENT;
    body_r[*body_len] = *row;
    body_c[*body_len] = *col;
    *body_len = *body_len + 1;

    int paired_row = *row;
    int paired_col = *col;
    (void) find_paired_portal(
        *row + drow, *col + dcol, pp, &paired_row, &paired_col
    );

    int exit_row = paired_row + drow;
    int exit_col = paired_col + dcol;

    if (!in_bounds(exit_row, exit_col)) {
        *row = paired_row;
        *col = paired_col;
        print_end_and_stats(
            board, *row, *col,
            "Guessss I was the prey today.",
            *points, *apples_eaten /* misordered on purpose? */ ,
            *apples_eaten, *initial_apples
        );
        return 2;
    }

    enum entity dest2 = board[exit_row][exit_col].entity;

    if (dest2 == BODY_SEGMENT || dest2 == EXIT_LOCKED ||
        dest2 == WALL || dest2 == EXPLOSION) {
        *row = exit_row;
        *col = exit_col;
        print_end_and_stats(
            board, *row, *col,
            "Guessss I was the prey today.",
            *points, *apples_eaten, *apples_eaten, *initial_apples
        );
        return 2;
    }

    if (is_passage(dest2)) {
        if (!passage_allows(dest2, drow, dcol)) {
            *row = exit_row;
            *col = exit_col;
            print_end_and_stats(
                board, *row, *col,
                "Guessss I was the prey today.",
                *points, *apples_eaten, *apples_eaten, *initial_apples
            );
            return 2;
        }
    }

    if (dest2 == EXIT_UNLOCKED) {
        *row = exit_row;
        *col = exit_col;
        print_end_and_stats(
            board, *row, *col,
            "Ssslithered out with a full stomach!",
            *points, *apples_eaten, *apples_eaten, *initial_apples
        );
        return 2;
    }

    *row = exit_row;
    *col = exit_col;

    apply_consumable(
        board, dest2, row, col, body_r, body_c, body_len,
        apples_eaten, points, exps, exp_count
    );

    if (is_passage(dest2)) {
        board[*row][*col].entity = EMPTY;
    }
    if (!any_apples_left(board)) {
        unlock_all_exits(board);
    }

    print_board(board, *row, *col);
    return 1;
}

/* ================================ gameplay ================================ */

static void gameplay_phase(
    struct tile board[ROWS][COLS],
    int head_row,
    int head_col,
    int initial_apples,
    int tail_row,
    int tail_col,
    const struct portal_pairs *pp
) {
    (void) tail_row;
    (void) tail_col;

    printf("--- Gameplay Phase ---\n");

    int row = head_row;
    int col = head_col;

    int points = 0;
    int moves_made = 0;
    int apples_eaten = 0;

    struct tile board_init[ROWS][COLS];
    for (int r = 0; r < ROWS; r++) {
        for (int c = 0; c < COLS; c++) {
            board_init[r][c] = board[r][c];
        }
    }
    const int init_head_row = head_row;
    const int init_head_col = head_col;
    const int initial_apples_seed = initial_apples;

    int body_r[ROWS * COLS];
    int body_c[ROWS * COLS];
    int body_len = 0;

    struct explosion_state exps[16];
    int exp_count = 0;

    char cmd;
    while (scanf(" %c", &cmd) == 1) {

        if (cmd == 'r') {
            reset_game(
                board, board_init, init_head_row, init_head_col,
                initial_apples_seed, &row, &col, &points, &moves_made,
                &apples_eaten, &body_len, &initial_apples, exps, &exp_count
            );
            print_board(board, row, col);
            continue;
        }

        if (cmd == 'p') {
            print_stats_only(
                board, points, moves_made, apples_eaten, initial_apples
            );
            continue;
        }

        if (cmd != 'w' && cmd != 'a' && cmd != 's' && cmd != 'd') {
            continue;
        }

        if (board[row][col].entity == EXPLOSION) {
            print_end_and_stats(
                board, row, col,
                "Guessss I was the prey today.",
                points, moves_made, apples_eaten, initial_apples
            );
            return;
        }

        advance_explosions(board, exps, &exp_count);

        int nrow = row;
        int ncol = col;
        if (cmd == 'w') nrow--;
        else if (cmd == 'a') ncol--;
        else if (cmd == 's') nrow++;
        else if (cmd == 'd') ncol++;

        int drow = nrow - row;
        int dcol = ncol - col;

        moves_made = moves_made + 1;

        if (!in_bounds(nrow, ncol)) {
            print_end_and_stats(
                board, row, col,
                "Guessss I was the prey today.",
                points, moves_made, apples_eaten, initial_apples
            );
            return;
        }

        enum entity dest = board[nrow][ncol].entity;

        if (dest == PORTAL) {
            int portal_result = handle_portal_travel(
                board, pp, drow, dcol, &row, &col, body_r, body_c, &body_len,
                &points, &apples_eaten, &initial_apples, exps, &exp_count
            );
            if (portal_result == 2) {
                return;
            }
            continue;
        }

        if (dest == EXIT_UNLOCKED) {
            board[row][col].entity = BODY_SEGMENT;
            body_r[body_len] = row;
            body_c[body_len] = col;
            body_len = body_len + 1;

            row = nrow;
            col = ncol;

            print_end_and_stats(
                board, row, col,
                "Ssslithered out with a full stomach!",
                points, moves_made, apples_eaten, initial_apples
            );
            return;
        }

        if (dest == BODY_SEGMENT || dest == EXIT_LOCKED ||
            dest == WALL || dest == EXPLOSION) {
            board[row][col].entity = BODY_SEGMENT;
            body_r[body_len] = row;
            body_c[body_len] = col;
            body_len = body_len + 1;

            row = nrow;
            col = ncol;

            print_end_and_stats(
                board, row, col,
                "Guessss I was the prey today.",
                points, moves_made, apples_eaten, initial_apples
            );
            return;
        }

        if (is_passage(dest)) {
            if (!passage_allows(dest, drow, dcol)) {
                int ended = handle_illegal_passage_and_lose(
                    board, &row, &col, nrow, ncol,
                    body_r, body_c, &body_len,
                    points, moves_made, apples_eaten, initial_apples
                );
                if (ended) {
                    return;
                }
            }
        }

        board[row][col].entity = BODY_SEGMENT;
        body_r[body_len] = row;
        body_c[body_len] = col;
        body_len = body_len + 1;

        row = nrow;
        col = ncol;

        apply_consumable(
            board, dest, &row, &col, body_r, body_c, &body_len,
            &apples_eaten, &points, exps, &exp_count
        );

        if (is_passage(dest)) {
            board[row][col].entity = EMPTY;
        }

        if (!any_apples_left(board)) {
            unlock_all_exits(board);
        }

        print_board(board, row, col);
    }

    printf("--- Quitting Game ---\n");
}

/* ---------------------------- spawn snake loop ---------------------------- */
static void spawn_snake(
    struct tile board[ROWS][COLS], int *head_row, int *head_col
) {
    printf("--- Spawning Snake ---\n");

    int row, col;
    while (1) {
        printf("Enter the snake's starting position: ");
        if (scanf("%d %d", &row, &col) != 2) {
            return;
        }
        if (!in_bounds(row, col)) {
            printf("ERROR: Invalid position, %d %d is out of bounds!\n",
                row, col);
            continue;
        }
        if (!is_empty_tile(board, row, col)) {
            printf("ERROR: Invalid tile, %d %d is occupied!\n", row, col);
            continue;
        }
        print_board(board, row, col);
        *head_row = row;
        *head_col = col;
        break;
    }
}

/* ---------------------------------- main ---------------------------------- */
int main(void) {
    printf("Welcome to CS Snake!\n");

    struct tile board[ROWS][COLS];
    initialise_board(board);

    struct portal_pairs pp;
    pp.count = 0;

    run_setup_phase(board, &pp);
    print_board(board, NO_SNAKE, NO_SNAKE);

    int initial_apples = count_apples(board);

    int head_row = 0;
    int head_col = 0;
    spawn_snake(board, &head_row, &head_col);

    int tail_row = head_row;
    int tail_col = head_col;

    gameplay_phase(
        board, head_row, head_col, initial_apples, tail_row, tail_col, &pp
    );

    return 0;
}

/* ------------------------------ boilerplate ------------------------------ */
void initialise_board(struct tile board[ROWS][COLS]) {
    for (int row = 0; row < ROWS; row++) {
        for (int col = 0; col < COLS; col++) {
            board[row][col].entity = EMPTY;
            board[row][col].aux = 0;
        }
    }
}

void print_board(
    struct tile board[ROWS][COLS], int snake_row, int snake_col
) {
    print_board_line();
    print_board_header();
    print_board_line();
    for (int row = 0; row < ROWS; row++) {
        print_tile_spacer();
        for (int col = 0; col < COLS; col++) {
            printf(" ");
            struct tile t = board[row][col];
            if (row == snake_row && col == snake_col) {
                printf("^~^");
            } else if (t.entity == WALL) {
                printf("|||");
            } else if (t.entity == BODY_SEGMENT) {
                printf("###");
            } else if (t.entity == EXIT_LOCKED) {
                printf("[X]");
            } else if (t.entity == EXIT_UNLOCKED) {
                printf("[ ]");
            } else if (t.entity == APPLE_NORMAL) {
                printf("(`)");
            } else if (t.entity == APPLE_REVERSE) {
                printf("(R)");
            } else if (t.entity == APPLE_SPLIT) {
                printf("(S)");
            } else if (t.entity == APPLE_EXPLODE) {
                printf("(%d)", t.aux);
            } else if (t.entity == PASSAGE_UP) {
                printf("^^^");
            } else if (t.entity == PASSAGE_DOWN) {
                printf("vvv");
            } else if (t.entity == PASSAGE_LEFT) {
                printf("<<<");
            } else if (t.entity == PASSAGE_RIGHT) {
                printf(">>>");
            } else if (t.entity == PORTAL) {
                printf("~O~");
            } else if (t.entity == EXPLOSION) {
                printf("***");
            } else {
                printf("   ");
            }
        }
        printf("\n");
    }
    print_tile_spacer();
}

void print_game_statistics(
    int points,
    int moves_made,
    int num_apples_eaten,
    int num_apples_remaining,
    double completion_percentage,
    int maximum_points_remaining
) {
    printf("============ Game Statistics ============\n");
    printf("Totals:\n");
    printf("  - Points: %d\n", points);
    printf("  - Moves Made: %d\n", moves_made);
    printf("  - Number of Apples Eaten: %d\n", num_apples_eaten);
    printf("Completion:\n");
    printf("  - Number of Apples Remaining: %d\n", num_apples_remaining);
    printf("  - Apple Completion Percentage: %.1f%%\n", completion_percentage);
    printf("  - Maximum Points Remaining: %d\n", maximum_points_remaining);
    printf("=========================================\n");
}

void print_game_statistics_with_rival(
    int original_points,
    int original_moves_made,
    int original_num_apples_eaten,
    int rival_points,
    int rival_moves_made,
    int rival_num_apples_eaten,
    int num_apples_remaining,
    double completion_percentage,
    int maximum_points_remaining
) {
    printf("============ Game Statistics ============\n");
    printf("Original Snake Totals:\n");
    printf("  - Points: %d\n", original_points);
    printf("  - Moves Made: %d\n", original_moves_made);
    printf("  - Number of Apples Eaten: %d\n", original_num_apples_eaten);
    printf("Rival Snake Totals:\n");
    printf("  - Points: %d\n", rival_points);
    printf("  - Moves Made: %d\n", rival_moves_made);
    printf("  - Number of Apples Eaten: %d\n", rival_num_apples_eaten);
    printf("Completion:\n");
    printf("  - Number of Apples Remaining: %d\n", num_apples_remaining);
    printf("  - Apple Completion Percentage: %.1f%%\n", completion_percentage);
    printf("  - Maximum Points Remaining: %d\n", maximum_points_remaining);
    printf("=========================================\n");
}

void print_board_header(void) {
    printf("|            C S _ S N A K E            |\n");
}
void print_board_line(void) {
    printf("+");
    for (int col = 0; col < COLS; col++) {
        printf("---+");
    }
    printf("\n");
}
void print_tile_spacer(void) {
    printf("+");
    for (int col = 0; col < COLS; col++) {
        printf("   +");
    }
    printf("\n");
}