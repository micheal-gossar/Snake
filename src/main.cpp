#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include <array>
#include <cstdint>
#include <random>
#include <string>
#include <vector>

namespace {

constexpr int cellSize = 40;
constexpr int columns = 30;
constexpr int rows = 18;
constexpr int windowWidth = columns * cellSize;
constexpr int windowHeight = rows * cellSize;
constexpr Uint64 moveInterval = 120;

struct Point {
    int x;
    int y;
};

enum class Direction {
    Up,
    Down,
    Left,
    Right
};

SDL_Window* window = nullptr;
SDL_Renderer* renderer = nullptr;
std::vector<Point> snake;
Point apple{};
Direction direction = Direction::Right;
Direction queuedDirection = Direction::Right;
bool hasQueuedTurn = false;
bool gameOver = false;
bool playerWon = false;
bool running = true;
int score = 0;
Uint64 nextMoveTime = 0;
std::mt19937 randomEngine{std::random_device{}()};

bool samePoint(Point left, Point right) {
    return left.x == right.x && left.y == right.y;
}

bool isOpposite(Direction first, Direction second) {
    return (first == Direction::Up && second == Direction::Down) ||
           (first == Direction::Down && second == Direction::Up) ||
           (first == Direction::Left && second == Direction::Right) ||
           (first == Direction::Right && second == Direction::Left);
}

void setWindowTitle() {
    std::string title = "Snake | Score: " + std::to_string(score);
    if (playerWon) {
        title += " | You win! Press R to play again";
    } else if (gameOver) {
        title += " | Game over - press R to restart";
    }
    SDL_SetWindowTitle(window, title.c_str());
}

bool placeApple() {
    std::vector<Point> emptyCells;
    emptyCells.reserve(columns * rows - static_cast<int>(snake.size()));

    for (int y = 0; y < rows; ++y) {
        for (int x = 0; x < columns; ++x) {
            const Point candidate{x, y};
            bool occupied = false;
            for (const Point segment : snake) {
                if (samePoint(candidate, segment)) {
                    occupied = true;
                    break;
                }
            }
            if (!occupied) {
                emptyCells.push_back(candidate);
            }
        }
    }

    if (emptyCells.empty()) {
        return false;
    }

    std::uniform_int_distribution<std::size_t> pickCell(0, emptyCells.size() - 1);
    apple = emptyCells[pickCell(randomEngine)];
    return true;
}

void resetGame() {
    snake.clear();
    snake.push_back({columns / 2, rows / 2});
    snake.push_back({columns / 2 - 1, rows / 2});
    snake.push_back({columns / 2 - 2, rows / 2});
    direction = Direction::Right;
    queuedDirection = Direction::Right;
    hasQueuedTurn = false;
    gameOver = false;
    playerWon = false;
    score = 0;
    nextMoveTime = SDL_GetTicks() + moveInterval;
    placeApple();
    setWindowTitle();
}

void queueTurn(Direction requested) {
    if (!hasQueuedTurn && !isOpposite(requested, direction)) {
        queuedDirection = requested;
        hasQueuedTurn = true;
    }
}

bool processEvents() {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_EVENT_QUIT) {
            return false;
        }

        if (event.type != SDL_EVENT_KEY_DOWN || event.key.repeat) {
            continue;
        }

        if (event.key.key == SDLK_ESCAPE) {
            return false;
        }

        if (gameOver) {
            if (event.key.key == SDLK_R) {
                resetGame();
            }
            continue;
        }

        switch (event.key.key) {
            case SDLK_UP:
            case SDLK_W:
                queueTurn(Direction::Up);
                break;
            case SDLK_DOWN:
            case SDLK_S:
                queueTurn(Direction::Down);
                break;
            case SDLK_LEFT:
            case SDLK_A:
                queueTurn(Direction::Left);
                break;
            case SDLK_RIGHT:
            case SDLK_D:
                queueTurn(Direction::Right);
                break;
            default:
                break;
        }
    }
    return true;
}

void updateGame(Uint64 now) {
    if (gameOver || now < nextMoveTime) {
        return;
    }

    nextMoveTime = now + moveInterval;
    if (hasQueuedTurn) {
        direction = queuedDirection;
        hasQueuedTurn = false;
    }

    Point newHead = snake.front();
    switch (direction) {
        case Direction::Up:
            --newHead.y;
            break;
        case Direction::Down:
            ++newHead.y;
            break;
        case Direction::Left:
            --newHead.x;
            break;
        case Direction::Right:
            ++newHead.x;
            break;
    }


    newHead.x = (newHead.x + columns) % columns;
    newHead.y = (newHead.y + rows) % rows;

    const bool eatingApple = samePoint(newHead, apple);
    const std::size_t checkedSegments = snake.size() - (eatingApple ? 0 : 1);
    for (std::size_t index = 0; index < checkedSegments; ++index) {
        if (samePoint(newHead, snake[index])) {
            gameOver = true;
            setWindowTitle();
            return;
        }
    }

    snake.insert(snake.begin(), newHead);
    if (eatingApple) {
        ++score;
        if (!placeApple()) {
            gameOver = true;
            playerWon = true;
        }
        setWindowTitle();
    } else {
        snake.pop_back();
    }
}

void drawScore() {
    constexpr std::array<std::uint16_t, 10> digitPixels{
        0b111101101101111, 0b010110010010111, 0b111001111100111,
        0b111001111001111, 0b101101111001001, 0b111100111001111,
        0b111100111101111, 0b111001001001001, 0b111101111101111,
        0b111101111001111
    };
    constexpr int pixelSize = 4;
    constexpr int digitWidth = 3;
    constexpr int digitHeight = 5;
    constexpr int digitSpacing = 4;

    std::string digits = std::to_string(score);
    const int totalWidth = static_cast<int>(digits.size()) * (digitWidth * pixelSize + digitSpacing);
    const int left = (windowWidth - totalWidth) / 2;
    const int top = 12;

    SDL_SetRenderDrawColor(renderer, 245, 245, 245, 255);
    for (std::size_t digitIndex = 0; digitIndex < digits.size(); ++digitIndex) {
        const int digit = digits[digitIndex] - '0';
        for (int row = 0; row < digitHeight; ++row) {
            for (int column = 0; column < digitWidth; ++column) {
                const int bit = digitHeight * digitWidth - 1 - (row * digitWidth + column);
                if ((digitPixels[digit] & (1u << bit)) == 0) {
                    continue;
                }
                SDL_FRect pixel{
                    static_cast<float>(left + static_cast<int>(digitIndex) *
                                               (digitWidth * pixelSize + digitSpacing) +
                                       column * pixelSize),
                    static_cast<float>(top + row * pixelSize),
                    static_cast<float>(pixelSize),
                    static_cast<float>(pixelSize)
                };
                SDL_RenderFillRect(renderer, &pixel);
            }
        }
    }
}

void render() {
    SDL_SetRenderDrawColor(renderer, 18, 22, 30, 255);
    SDL_RenderClear(renderer);

    SDL_SetRenderDrawColor(renderer, 35, 42, 52, 255);
    for (int x = 0; x <= windowWidth; x += cellSize) {
        SDL_RenderLine(renderer, static_cast<float>(x), 0.0f,
                       static_cast<float>(x), static_cast<float>(windowHeight));
    }
    for (int y = 0; y <= windowHeight; y += cellSize) {
        SDL_RenderLine(renderer, 0.0f, static_cast<float>(y),
                       static_cast<float>(windowWidth), static_cast<float>(y));
    }

    SDL_FRect appleRect{
        static_cast<float>(apple.x * cellSize + 6),
        static_cast<float>(apple.y * cellSize + 6),
        static_cast<float>(cellSize - 12),
        static_cast<float>(cellSize - 12)
    };
    SDL_SetRenderDrawColor(renderer, 245, 75, 75, 255);
    SDL_RenderFillRect(renderer, &appleRect);

    for (std::size_t index = 0; index < snake.size(); ++index) {
        const Point segment = snake[index];
        SDL_FRect segmentRect{
            static_cast<float>(segment.x * cellSize + 2),
            static_cast<float>(segment.y * cellSize + 2),
            static_cast<float>(cellSize - 4),
            static_cast<float>(cellSize - 4)
        };
        if (index == 0) {
            SDL_SetRenderDrawColor(renderer, 110, 235, 125, 255);
        } else {
            SDL_SetRenderDrawColor(renderer, 50, 180, 90, 255);
        }
        SDL_RenderFillRect(renderer, &segmentRect);
    }

    drawScore();
    SDL_RenderPresent(renderer);
}

} // namespace

int main(int, char*[]) {
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS)) {
        return 1;
    }

    window = SDL_CreateWindow("Snake", windowWidth, windowHeight, 0);
    if (!window) {
        SDL_Quit();
        return 1;
    }

    renderer = SDL_CreateRenderer(window, nullptr);
    if (!renderer) {
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    resetGame();
    while (running) {
        running = processEvents();
        if (!running) {
            break;
        }
        updateGame(SDL_GetTicks());
        render();
        SDL_Delay(8);
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
