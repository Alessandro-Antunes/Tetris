#include "raylib.h"
#include <stdio.h>
#include <string.h>
#include <time.h>

#define N 4
#define ROWS 20
#define COLS 10
#define CELL_SIZE 30
#define NUM_OF_PIECES 7
#define DISSOLVE_FRAMES 40 // duração do efeito de dissolver (60 FPS)
#define REPEAT_DELAY 12    // frames segurando antes de começar a repetir
#define REPEAT_RATE 3      // frames entre cada repetição

typedef struct {
    Vector2 pos;
    int shape[N][N];
    Color color;
} Piece;

void CreateArea(Rectangle *r);
void DrawPlayArea(Rectangle r);
Piece GeneratePiece(Piece pieces[NUM_OF_PIECES]);
void DrawPiece(Piece *p, Rectangle r);
void SetPiecePos(Piece *p);
void MoveRight(Piece *p);
void MoveLeft(Piece *p);
void MoveDown(Piece *p);
void Rotate(Piece *p);
int CanMoveRight(Piece *p, int grid[ROWS][COLS]);
int CanMoveLeft(Piece *p, int grid[ROWS][COLS]);
int CanMoveDown(Piece *p, int grid[ROWS][COLS]);
int FitsAt(Piece *p, int grid[ROWS][COLS]);
int TryRotate(Piece *p, int grid[ROWS][COLS]);
int KeyRepeat(int key, int *counter);
void DrawFixedPieces(int grid[ROWS][COLS], Rectangle r, int clearing,
                     int clearRows[ROWS], int noise[ROWS][COLS],
                     int clearTimer);
Color GetPieceColor(int cell);

int IsRowFull(int row[COLS]);
void ShiftDown(int grid[ROWS][COLS], int row);
int MarkFullRows(int grid[ROWS][COLS], int clearRows[ROWS]);
int RemoveMarkedRows(int grid[ROWS][COLS], int clearRows[ROWS]);
int LineScore(int lines);
int IsGameOver(Piece *p, int grid[ROWS][COLS]);
void DrawNextPiece(Piece *p, Rectangle r, int windowWidth);
void DrawScore(int score, int lines, int level);
int GetLevel(int lines);
int GetDropInterval(int level);
void DrawGhostPiece(Piece *p, Rectangle r, int grid[ROWS][COLS]);
int HardDrop(Piece *p, int grid[ROWS][COLS]);
void ResetGame(Piece pieces[NUM_OF_PIECES], Piece *current, Piece *next,
               int grid[ROWS][COLS], int *timer, int *gameOver, int *score,
               int *lines);

//------------------------------------------------------------------------------------
// Program main entry point
//------------------------------------------------------------------------------------
int main(void) {
    // Initialization
    //--------------------------------------------------------------------------------------
    const int screenWidth = 600;
    const int screenHeight = 800;

    InitWindow(screenWidth, screenHeight, "Tetris");

    SetTargetFPS(60); // Set our game to run at 60 frames-per-second
    //--------------------------------------------------------------------------------------

    Rectangle playArea;
    CreateArea(&playArea);

    Piece L = {
        .shape = {{0, 1, 0, 0}, {0, 1, 0, 0}, {0, 1, 1, 0}, {0, 0, 0, 0}},
        .color = RED};
    Piece I = {
        .shape = {{0, 0, 2, 0}, {0, 0, 2, 0}, {0, 0, 2, 0}, {0, 0, 2, 0}},
        .color = GREEN};
    Piece J = {
        .shape = {{0, 0, 0, 0}, {0, 0, 3, 0}, {0, 0, 3, 0}, {0, 3, 3, 0}},
        .color = WHITE};
    Piece O = {
        .shape = {{0, 0, 0, 0}, {0, 4, 4, 0}, {0, 4, 4, 0}, {0, 0, 0, 0}},
        .color = ORANGE};
    Piece S = {
        .shape = {{0, 0, 0, 0}, {0, 0, 5, 5}, {0, 5, 5, 0}, {0, 0, 0, 0}},
        .color = YELLOW};
    Piece Z = {
        .shape = {{0, 0, 0, 0}, {0, 6, 6, 0}, {0, 0, 6, 6}, {0, 0, 0, 0}},
        .color = VIOLET};
    Piece T = {
        .shape = {{0, 0, 0, 0}, {0, 0, 7, 0}, {0, 7, 7, 7}, {0, 0, 0, 0}},
        .color = BLUE};

    Piece pieces[NUM_OF_PIECES] = {L, I, J, S, O, T, Z};

    SetRandomSeed(time(NULL));
    Piece currentPiece = GeneratePiece(pieces);
    SetPiecePos(&currentPiece);
    Piece nextPiece = GeneratePiece(pieces);

    int grid[ROWS][COLS] = {0};

    int timer = 0;
    int gameOver = 0;
    int score = 0;
    int lines = 0;

    // Estado do efeito de dissolver
    int clearing = 0;   // 1 enquanto a animação está rodando
    int clearTimer = 0; // frames desde o início da animação
    int clearRows[ROWS] = {0};
    int dissolveNoise[ROWS][COLS] = {0};

    int paused = 0;
    int repeatLeft = 0, repeatRight = 0, repeatDown = 0;

    // Main game loop
    while (!WindowShouldClose()) // Detect window close button or ESC key
    {
        // Update
        //----------------------------------------------------------------------------------

        // Reiniciar o jogo (funciona a qualquer momento)
        if (IsKeyPressed(KEY_R)) {
            ResetGame(pieces, &currentPiece, &nextPiece, grid, &timer,
                      &gameOver, &score, &lines);
            clearing = 0;
            clearTimer = 0;
            paused = 0;
        }

        // Pausar / continuar
        if (IsKeyPressed(KEY_P) && !gameOver) {
            paused = !paused;
        }

        if (paused) {
            // jogo parado: nada é atualizado
        } else if (!gameOver && clearing) {
            // Animação de dissolver: o jogo fica pausado até terminar
            clearTimer++;
            if (clearTimer >= DISSOLVE_FRAMES) {
                int cleared = RemoveMarkedRows(grid, clearRows);
                lines += cleared;
                score += LineScore(cleared);
                clearing = 0;

                currentPiece = nextPiece;
                nextPiece = GeneratePiece(pieces);
                SetPiecePos(&currentPiece);

                if (IsGameOver(&currentPiece, grid)) {
                    gameOver = 1;
                }
            }
        } else if (!gameOver) {

            if (!CanMoveDown(&currentPiece, grid)) {
                // Set occupied cell
                for (int i = 0; i < N; i++) {
                    for (int j = 0; j < N; j++) {
                        if (currentPiece.shape[i][j] != 0) {
                            int col = (int)currentPiece.pos.x + j;
                            int row = (int)currentPiece.pos.y + i;
                            if (row >= 0 && row < ROWS) {
                                grid[row][col] = currentPiece.shape[i][j];
                            }
                        }
                    }
                }

                if (MarkFullRows(grid, clearRows) > 0) {
                    // Há linhas completas: inicia o efeito de dissolver
                    clearing = 1;
                    clearTimer = 0;
                    for (int i = 0; i < ROWS; i++) {
                        for (int j = 0; j < COLS; j++) {
                            dissolveNoise[i][j] = GetRandomValue(5, 100);
                        }
                    }
                } else {
                    currentPiece = nextPiece;
                    nextPiece = GeneratePiece(pieces);
                    SetPiecePos(&currentPiece);

                    if (IsGameOver(&currentPiece, grid)) {
                        gameOver = 1;
                    }
                }
            }

            if (!clearing) {
                // segurar a tecla repete o movimento
                if (KeyRepeat(KEY_RIGHT, &repeatRight) &&
                    CanMoveRight(&currentPiece, grid)) {
                    MoveRight(&currentPiece);
                }
                if (KeyRepeat(KEY_LEFT, &repeatLeft) &&
                    CanMoveLeft(&currentPiece, grid)) {
                    MoveLeft(&currentPiece);
                }
                if (KeyRepeat(KEY_DOWN, &repeatDown) &&
                    CanMoveDown(&currentPiece, grid)) {
                    MoveDown(&currentPiece);
                    score += 1; // ponto por descida manual
                }
                if (IsKeyPressed(KEY_UP)) {
                    TryRotate(&currentPiece, grid); // com wall kick
                }

                if (IsKeyPressed(KEY_SPACE)) {
                    // queda rápida: 2 pontos por célula percorrida
                    score += 2 * HardDrop(&currentPiece, grid);
                }

                // gravidade: quanto maior o nível, menor o intervalo
                timer++;
                if (timer >= GetDropInterval(GetLevel(lines))) {
                    timer = 0;
                    if (CanMoveDown(&currentPiece, grid)) {
                        currentPiece.pos.y += 1;
                    }
                }
            }
        }
        // Draw
        //----------------------------------------------------------------------------------
        BeginDrawing();

        ClearBackground(BLACK);

        DrawPlayArea(playArea);
        DrawFixedPieces(grid, playArea, clearing, clearRows, dissolveNoise,
                        clearTimer);
        DrawScore(score, lines, GetLevel(lines));

        if (!gameOver) {
            if (!clearing) {
                DrawGhostPiece(&currentPiece, playArea, grid);
                DrawPiece(&currentPiece, playArea);
            }
            DrawNextPiece(&nextPiece, playArea, screenWidth);
        } else {
            const char *title = "Game Over!";
            const char *hint = "Pressione R para reiniciar";
            int titleSize = CELL_SIZE;
            int hintSize = 20;
            int centerX = playArea.x + playArea.width / 2;

            DrawText(title, centerX - MeasureText(title, titleSize) / 2, 20,
                     titleSize, RED);
            DrawText(hint, centerX - MeasureText(hint, hintSize) / 2, 70,
                     hintSize, WHITE);
        }

        if (paused) {
            // escurece a área de jogo e mostra o aviso de pausa
            DrawRectangleRec(playArea, Fade(BLACK, 0.85f));

            const char *pauseTitle = "PAUSADO";
            const char *pauseHint = "Pressione P para continuar";
            int pauseTitleSize = CELL_SIZE + 10;
            int pauseHintSize = 20;
            int cx = playArea.x + playArea.width / 2;
            int cy = playArea.y + playArea.height / 2;

            DrawText(pauseTitle, cx - MeasureText(pauseTitle, pauseTitleSize) / 2,
                     cy - 40, pauseTitleSize, YELLOW);
            DrawText(pauseHint, cx - MeasureText(pauseHint, pauseHintSize) / 2,
                     cy + 20, pauseHintSize, WHITE);
        }

        EndDrawing();
        //----------------------------------------------------------------------------------
    }

    // De-Initialization
    //--------------------------------------------------------------------------------------
    CloseWindow(); // Close window and OpenGL context
    //--------------------------------------------------------------------------------------

    return 0;
}

void ResetGame(Piece pieces[NUM_OF_PIECES], Piece *current, Piece *next,
               int grid[ROWS][COLS], int *timer, int *gameOver, int *score,
               int *lines) {
    memset(grid, 0, sizeof(int) * ROWS * COLS);

    *current = GeneratePiece(pieces);
    SetPiecePos(current);
    *next = GeneratePiece(pieces);

    *timer = 0;
    *gameOver = 0;
    *score = 0;
    *lines = 0;
}

// Pontos por quantidade de linhas limpas de uma vez
int LineScore(int lines) {
    switch (lines) {
    case 1:
        return 100;
    case 2:
        return 300;
    case 3:
        return 500;
    case 4:
        return 800;
    default:
        return 0;
    }
}

void DrawScore(int score, int lines, int level) {
    int x = 430;
    int y = 400;
    DrawText("Score:", x, y, CELL_SIZE, WHITE);
    DrawText(TextFormat("%d", score), x, y + CELL_SIZE + 5, CELL_SIZE, YELLOW);
    DrawText("Lines:", x, y + 2 * CELL_SIZE + 30, CELL_SIZE, WHITE);
    DrawText(TextFormat("%d", lines), x, y + 3 * CELL_SIZE + 35, CELL_SIZE,
             YELLOW);
    DrawText("Level:", x, y + 4 * CELL_SIZE + 70, CELL_SIZE, WHITE);
    DrawText(TextFormat("%d", level), x, y + 5 * CELL_SIZE + 75, CELL_SIZE,
             YELLOW);
}

// Sobe 1 nível a cada 10 linhas limpas
int GetLevel(int lines) { return lines / 10 + 1; }

// Frames entre cada descida automática (nível 1 = 20 frames, mínimo 2)
int GetDropInterval(int level) {
    int interval = 20 - (level - 1) * 2;
    return interval < 2 ? 2 : interval;
}

// Desenha o contorno de onde a peça vai cair
void DrawGhostPiece(Piece *p, Rectangle r, int grid[ROWS][COLS]) {
    Piece ghost = *p;
    while (CanMoveDown(&ghost, grid)) {
        ghost.pos.y++;
    }

    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            if (ghost.shape[i][j] != 0) {
                int screenX = r.x + (ghost.pos.x + j) * CELL_SIZE;
                int screenY = r.y + (ghost.pos.y + i) * CELL_SIZE;
                DrawRectangleLines(screenX, screenY, CELL_SIZE - 1,
                                   CELL_SIZE - 1, Fade(p->color, 0.6f));
            }
        }
    }
}

// Leva a peça até o fundo e retorna quantas células ela desceu
int HardDrop(Piece *p, int grid[ROWS][COLS]) {
    int distance = 0;
    while (CanMoveDown(p, grid)) {
        p->pos.y++;
        distance++;
    }
    return distance;
}

void CreateArea(Rectangle *r) {
    Vector2 point = {100, 100};
    Vector2 size = {COLS * CELL_SIZE, ROWS * CELL_SIZE};
    r->x = point.x;
    r->y = point.y;
    r->width = size.x;
    r->height = size.y;
}

void DrawPlayArea(Rectangle r) {
    float thickness = 3;
    // A borda é desenhada para dentro do retângulo, então expandimos o
    // retângulo para que ela fique por fora e não cubra os blocos.
    Rectangle border = {r.x - thickness, r.y - thickness,
                        r.width + 2 * thickness, r.height + 2 * thickness};
    DrawRectangleLinesEx(border, thickness, WHITE);
}

Piece GeneratePiece(Piece pieces[NUM_OF_PIECES]) {
    int num = GetRandomValue(0, NUM_OF_PIECES - 1);
    return pieces[num];
}

void DrawPiece(Piece *p, Rectangle r) {
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            if (p->shape[i][j] != 0) {
                int screenX = r.x + (p->pos.x + j) * CELL_SIZE;
                int screenY = r.y + (p->pos.y + i) * CELL_SIZE;
                DrawRectangle(screenX, screenY, CELL_SIZE - 1, CELL_SIZE - 1,
                              p->color);
            }
        }
    }
}

void SetPiecePos(Piece *p) {
    p->pos.x = 3;
    p->pos.y = 0;
}

void MoveRight(Piece *p) { p->pos.x++; }
void MoveLeft(Piece *p) { p->pos.x--; }
void MoveDown(Piece *p) { p->pos.y++; }

void Rotate(Piece *p) {
    // transpose matrix
    for (int i = 0; i < N; i++) {
        for (int j = i + 1; j < N; j++) {
            int temp = p->shape[i][j];
            p->shape[i][j] = p->shape[j][i];
            p->shape[j][i] = temp;
        }
    }

    // reverse each row
    for (int i = 0; i < N; i++) {
        int start = 0, end = N - 1;
        while (start < end) {
            int temp = p->shape[i][start];
            p->shape[i][start] = p->shape[i][end];
            p->shape[i][end] = temp;

            start++;
            end--;
        }
    }
}

int CanMoveRight(Piece *p, int grid[ROWS][COLS]) {
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            if (p->shape[i][j] != 0) {
                int col = (int)p->pos.x + j;
                int row = (int)p->pos.y + i;
                if (col + 1 >= COLS || grid[row][col + 1] != 0) {
                    return 0;
                }
            }
        }
    }
    return 1;
}
int CanMoveLeft(Piece *p, int grid[ROWS][COLS]) {
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            if (p->shape[i][j] != 0) {
                int col = (int)p->pos.x + j;
                int row = (int)p->pos.y + i;
                if (col - 1 < 0 || grid[row][col - 1] != 0) {
                    return 0;
                }
            }
        }
    }
    return 1;
}

int CanMoveDown(Piece *p, int grid[ROWS][COLS]) {
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            if (p->shape[i][j] != 0) {
                int col = (int)p->pos.x + j;
                int row = (int)p->pos.y + i;
                if (row + 1 >= ROWS || grid[row + 1][col] != 0) {
                    return 0;
                }
            }
        }
    }
    return 1;
}

// Verifica se a peça, na posição atual, cabe dentro da área e sem colidir
int FitsAt(Piece *p, int grid[ROWS][COLS]) {
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            if (p->shape[i][j] != 0) {
                int col = (int)p->pos.x + j;
                int row = (int)p->pos.y + i;

                if (col < 0 || col >= COLS || row < 0 || row >= ROWS) {
                    return 0;
                }
                if (grid[row][col] != 0) {
                    return 0;
                }
            }
        }
    }
    return 1;
}

// Gira a peça. Se a rotação normal for bloqueada, tenta deslocá-la
// (wall kick). Retorna 1 se conseguiu girar.
int TryRotate(Piece *p, int grid[ROWS][COLS]) {
    // deslocamentos testados, em ordem: x, y
    const int kicks[][2] = {{0, 0},  {-1, 0}, {1, 0},
                            {-2, 0}, {2, 0},  {0, -1}};
    const int numKicks = sizeof(kicks) / sizeof(kicks[0]);

    Piece rotated = *p;
    Rotate(&rotated);

    for (int k = 0; k < numKicks; k++) {
        Piece test = rotated;
        test.pos.x += kicks[k][0];
        test.pos.y += kicks[k][1];

        if (FitsAt(&test, grid)) {
            *p = test;
            return 1;
        }
    }
    return 0;
}

// Retorna 1 no frame em que a tecla é apertada e, se for mantida, volta a
// retornar 1 a cada REPEAT_RATE frames depois de REPEAT_DELAY frames.
int KeyRepeat(int key, int *counter) {
    if (IsKeyPressed(key)) {
        *counter = 0;
        return 1;
    }
    if (IsKeyDown(key)) {
        (*counter)++;
        if (*counter >= REPEAT_DELAY &&
            (*counter - REPEAT_DELAY) % REPEAT_RATE == 0) {
            return 1;
        }
    } else {
        *counter = 0;
    }
    return 0;
}

void DrawFixedPieces(int grid[ROWS][COLS], Rectangle r, int clearing,
                     int clearRows[ROWS], int noise[ROWS][COLS],
                     int clearTimer) {
    // progresso da animação de 0 a 100
    int progress = clearTimer * 100 / DISSOLVE_FRAMES;

    for (int i = 0; i < ROWS; i++) {
        for (int j = 0; j < COLS; j++) {
            if (grid[i][j] != 0) {
                int screenX = r.x + j * CELL_SIZE;
                int screenY = r.y + i * CELL_SIZE;
                Color color = GetPieceColor(grid[i][j]);

                if (clearing && clearRows[i]) {
                    // cada célula some quando o progresso passa do seu limite
                    if (progress >= noise[i][j]) {
                        continue;
                    }
                    // as que ainda restam vão ficando transparentes
                    color = Fade(color, 1.0f - 0.6f * progress / 100.0f);
                }

                DrawRectangle(screenX, screenY, CELL_SIZE - 1, CELL_SIZE - 1,
                              color);
            }
        }
    }
}

Color GetPieceColor(int cell) {
    switch (cell) {
    case 1:
        return RED;
    case 2:
        return GREEN;
    case 3:
        return WHITE;
    case 4:
        return ORANGE;
    case 5:
        return YELLOW;
    case 6:
        return VIOLET;
    case 7:
        return BLUE;
    default:
        return BLANK;
    }
}

int IsRowFull(int row[COLS]) {
    for (int i = 0; i < COLS; i++) {
        if (!row[i]) {
            return 0;
        }
    }
    return 1;
}

void ShiftDown(int grid[ROWS][COLS], int row) {
    for (int col = 0; col < COLS; col++) {
        grid[row][col] = 0;
    }
    for (int r = row; r > 0; r--) {
        for (int c = 0; c < COLS; c++) {
            grid[r][c] = grid[r - 1][c];
        }
    }
}

// Marca as linhas completas em clearRows e retorna quantas são
int MarkFullRows(int grid[ROWS][COLS], int clearRows[ROWS]) {
    int count = 0;
    for (int r = 0; r < ROWS; r++) {
        clearRows[r] = IsRowFull(grid[r]);
        if (clearRows[r]) {
            count++;
        }
    }
    return count;
}

// Remove as linhas marcadas e retorna quantas foram removidas
int RemoveMarkedRows(int grid[ROWS][COLS], int clearRows[ROWS]) {
    int cleared = 0;
    for (int r = 0; r < ROWS; r++) {
        if (clearRows[r]) {
            ShiftDown(grid, r);
            clearRows[r] = 0;
            cleared++;
        }
    }
    return cleared;
}

int IsGameOver(Piece *p, int grid[ROWS][COLS]) {
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            int col = p->pos.x + j;
            int row = p->pos.y + i;
            if (p->shape[i][j] != 0 && grid[row][col] != 0) {
                return 1;
            }
        }
    }
    return 0;
}

void DrawNextPiece(Piece *p, Rectangle r, int windowWidth) {
    int distanceX = windowWidth - r.x + r.width;
    distanceX = distanceX / 2;
    int distanceY = r.height / 2 - N * CELL_SIZE;
    DrawText("Next Piece:", distanceX + CELL_SIZE, distanceY, CELL_SIZE, WHITE);

    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            if (p->shape[i][j] != 0) {
                int screenX = distanceX + (p->pos.x + j) * CELL_SIZE;
                int screenY =
                    distanceY + CELL_SIZE + (p->pos.y + i) * CELL_SIZE;
                DrawRectangle(screenX, screenY, CELL_SIZE - 1, CELL_SIZE - 1,
                              p->color);
            }
        }
    }
}