#include "raylib.h"
#include <vector>
#include <algorithm>

const int BLOCK_SIZE = 48;
const int COLS = 10;
const int ROWS = 20;

enum TetrominoType
{
    I,
    O,
    T
};
enum GameState
{
    MENU,
    PLAYING,
    GAME_OVER
};

bool CheckButton(int x, int y, int w, int h)
{
    Vector2 m = GetMousePosition();
    return m.x >= x && m.x <= x + w && m.y >= y && m.y <= y + h && IsMouseButtonPressed(MOUSE_LEFT_BUTTON);
}

struct Tetromino
{
    TetrominoType type;
    Vector2 position;
    std::vector<Vector2> blocks;
    Color color;

    void Draw()
    {
        for (auto &b : blocks)
        {
            int x = (int)(position.x + b.x);
            int y = (int)(position.y + b.y);
            DrawRectangle(x * BLOCK_SIZE, y * BLOCK_SIZE, BLOCK_SIZE - 2, BLOCK_SIZE - 2, color);
        }
    }

    void Move(Vector2 dir)
    {
        position.x += dir.x;
        position.y += dir.y;
    }

    void Rotate()
    {
        for (auto &b : blocks)
        {
            float temp = b.x;
            b.x = -b.y;
            b.y = temp;
        }
    }

    std::vector<Vector2> GetBlockPositions() const
    {
        std::vector<Vector2> world;
        for (auto &b : blocks)
        {
            world.push_back({position.x + b.x, position.y + b.y});
        }
        return world;
    }
};

Tetromino CreateTetromino(TetrominoType type)
{
    Tetromino t;
    t.position = {4, 0};
    t.type = type;
    switch (type)
    {
    case I:
        t.blocks = {{0, 0}, {1, 0}, {-1, 0}, {-2, 0}};
        t.color = SKYBLUE;
        break;
    case O:
        t.blocks = {{0, 0}, {1, 0}, {0, 1}, {1, 1}};
        t.color = YELLOW;
        break;
    case T:
        t.blocks = {{0, 0}, {-1, 0}, {1, 0}, {0, 1}};
        t.color = PURPLE;
        break;
    }
    return t;
}

class Observer
{
public:
    virtual void OnScoreChanged(int newScore) = 0;
};

class Subject
{
    std::vector<Observer *> observers;
    int score = 0;

public:
    void Attach(Observer *obs) { observers.push_back(obs); }
    void SetScore(int newScore)
    {
        score = newScore;
        for (auto obs : observers)
            obs->OnScoreChanged(score);
    }
    int GetScore() const { return score; }
};

class ScoreUI : public Observer
{
    int currentScore = 0;

public:
    void OnScoreChanged(int newScore) override
    {
        currentScore = newScore;
    }

    void Draw()
    {
        DrawText(TextFormat("Score: %d", currentScore), 10, 10, 30, BLACK);
    }

    int Get() const { return currentScore; }
};

Color grid[ROWS][COLS] = {0};

bool CheckCollision(const Tetromino &t)
{
    for (auto &b : t.GetBlockPositions())
    {
        int x = (int)b.x;
        int y = (int)b.y;
        if (x < 0 || x >= COLS || y < 0 || y >= ROWS)
            return true;
        if (grid[y][x].a != 0)
            return true;
    }
    return false;
}

void LockToGrid(const Tetromino &t)
{
    for (auto &b : t.GetBlockPositions())
    {
        int x = (int)b.x;
        int y = (int)b.y;
        if (y >= 0 && y < ROWS && x >= 0 && x < COLS)
            grid[y][x] = t.color;
    }
}

int ClearLines()
{
    int lines = 0;
    for (int y = ROWS - 1; y >= 0; y--)
    {
        bool full = true;
        for (int x = 0; x < COLS; x++)
        {
            if (grid[y][x].a == 0)
            {
                full = false;
                break;
            }
        }
        if (full)
        {
            lines++;
            for (int row = y; row > 0; row--)
                for (int col = 0; col < COLS; col++)
                    grid[row][col] = grid[row - 1][col];
            for (int col = 0; col < COLS; col++)
                grid[0][col] = {0};
            y++;
        }
    }
    return lines;
}

void DrawGrid()
{
    for (int y = 0; y < ROWS; y++)
    {
        for (int x = 0; x < COLS; x++)
        {
            if (grid[y][x].a != 0)
                DrawRectangle(x * BLOCK_SIZE, y * BLOCK_SIZE, BLOCK_SIZE - 2, BLOCK_SIZE - 2, grid[y][x]);
        }
    }
}

int main()
{
    InitWindow(COLS * BLOCK_SIZE, ROWS * BLOCK_SIZE, "Tetris - Enlarged");
    SetTargetFPS(60);
    InitAudioDevice();

    Texture2D bg = LoadTexture("Assets/background.png");

    Sound sfxLineClear = LoadSound("Sounds/clear.mp3");
    Sound sfxGameOver = LoadSound("Sounds/game-over.mp3");
    Sound sfxSelect = LoadSound("Sounds/rotate.mp3");
    Sound BGM = LoadSound("Sounds/music.mp3");

    PlaySound(BGM);

    GameState currentState = MENU;

    Subject gameScore;
    ScoreUI ui;
    gameScore.Attach(&ui);

    Tetromino current;
    float fallTimer = 0;
    float fallDelay = 0.5f;

    bool gameOverSoundPlayed = false; // Tambahkan variabel ini

    while (!WindowShouldClose())
    {
        BeginDrawing();
        ClearBackground(RAYWHITE);

        DrawTexture(bg, 0, 0, WHITE);

        if (currentState == MENU)
        {
            DrawText("TETRIS", 130, 150, 60, WHITE);
            DrawText("Press Play to Start", 80, 250, 30, BLACK);
            if (CheckButton(160, 300, 160, 60))
            {
                PlaySound(sfxSelect);
                PlaySound(BGM); // BGM mulai lagi saat play
                gameScore.SetScore(0);
                fallTimer = 0;
                for (int y = 0; y < ROWS; y++)
                    for (int x = 0; x < COLS; x++)
                        grid[y][x] = {0};
                current = CreateTetromino((TetrominoType)(GetRandomValue(0, 2)));
                currentState = PLAYING;
                gameOverSoundPlayed = false; // Reset status game over
            }
            DrawRectangle(160, 300, 160, 60, LIGHTGRAY);
            DrawText("Play", 210, 315, 28, BLACK);

            if (CheckButton(160, 400, 160, 60))
            {
                PlaySound(sfxSelect);
                break;
            }
            DrawRectangle(160, 400, 160, 60, LIGHTGRAY);
            DrawText("Exit", 210, 415, 28, BLACK);
        }

        else if (currentState == PLAYING)
        {
            if (IsKeyPressed(KEY_LEFT))
            {
                current.Move({-1, 0});
                if (CheckCollision(current))
                    current.Move({1, 0});
            }
            if (IsKeyPressed(KEY_RIGHT))
            {
                current.Move({1, 0});
                if (CheckCollision(current))
                    current.Move({-1, 0});
            }
            if (IsKeyPressed(KEY_UP))
            {
                current.Rotate();
                if (CheckCollision(current))
                {
                    current.Rotate();
                    current.Rotate();
                    current.Rotate();
                }
            }

            fallTimer += GetFrameTime();
            if (fallTimer >= fallDelay || IsKeyDown(KEY_DOWN))
            {
                fallTimer = 0;
                current.Move({0, 1});
                if (CheckCollision(current))
                {
                    current.Move({0, -1});
                    LockToGrid(current);
                    int lines = ClearLines();
                    if (lines > 0)
                    {
                        PlaySound(sfxLineClear);
                        gameScore.SetScore(gameScore.GetScore() + lines * 100);
                    }
                    current = CreateTetromino((TetrominoType)(GetRandomValue(0, 2)));
                    if (CheckCollision(current))
                    {
                        if (!gameOverSoundPlayed)
                        {
                            StopSound(BGM); // Stop BGM saat game over
                            PlaySound(sfxGameOver);
                            gameOverSoundPlayed = true;
                        }
                        currentState = GAME_OVER;
                    }
                }
            }

            DrawGrid();
            current.Draw();
            ui.Draw();
        }

        else if (currentState == GAME_OVER)
        {
            DrawText("GAME OVER", 90, 300, 48, RED);
            DrawText(TextFormat("Final Score: %d", ui.Get()), 120, 370, 28, BLACK);
            if (CheckButton(140, 450, 200, 60))
            {
                PlaySound(sfxSelect);
                currentState = MENU;
            }
            DrawRectangle(140, 450, 200, 60, LIGHTGRAY);
            DrawText("Back to Menu", 150, 470, 24, BLACK);
        }

        EndDrawing();
    }

    UnloadTexture(bg);
    UnloadSound(sfxLineClear);
    UnloadSound(sfxGameOver);
    UnloadSound(sfxSelect);
    UnloadSound(BGM);
    CloseAudioDevice();
    CloseWindow();
    return 0;
}
