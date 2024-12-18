#include <iostream>
#include <vector>
#include <unistd.h> // Для usleep
#include <termios.h>
#include <cstdlib> // Для rand()

using namespace std;

class Bullet; // Предварительное объявление класса Bullet

// Функция для проверки нажатия клавиши
bool kbhit() {
    struct termios oldt, newt;
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    int ch = getchar();
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    if (ch != EOF) {
        ungetc(ch, stdin);
        return true;
    }
    return false;
}

// Функция для получения символа нажатой клавиши
char getch() {
    struct termios oldt, newt;
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    char ch = getchar();
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    return ch;
}

// Шаблон "Абстрактная фабрика" (Abstract Factory)
// Базовый класс Entity, от которого наследуются все игровые объекты
class Entity {
public:
    int x, y; // Координаты
    int speed; // Скорость
    char symbol; // Символ для отображения

    Entity(int startX, int startY, int spd, char sym)
        : x(startX), y(startY), speed(spd), symbol(sym) {}

    virtual void update(vector<Entity*>& entities) = 0; // Чисто виртуальная функция
};

// Шаблон "Стратегия" (Strategy)
// Класс Bullet реализует стратегию движения для пуль
class Bullet : public Entity {
public:
    Bullet(int startX, int startY, int direction)
        : Entity(startX, startY, direction, '*') {} // Направление задается при создании

    void update(vector<Entity*>&) override {
        y += speed; // Движение пули
    }
};

// Шаблон "Стратегия" (Strategy)
// Класс Alien реализует стратегию движения и стрельбы для пришельцев
class Alien : public Entity {
public:
    bool isShooting;

    Alien(int startX, int startY, int spd, char sym, bool shoot = false)
        : Entity(startX, startY, spd, sym), isShooting(shoot) {}

    void update(vector<Entity*>& entities) override {
        // Логика движения пришельца
        x += speed;

        // Если пришелец стреляет
        if (isShooting && rand() % 100 < 5) { // 5% шанс выстрела
            entities.push_back(new Bullet(x, y + 1, 1)); // Пуля движется вниз
        }

        // Проверка на столкновение со стенами
        if (x <= 0 || x >= 79) {
            speed = -speed; // Изменение направления
            y++; // Опускаемся на одну строку вниз
        }
    }
};

// Шаблон "Стратегия" (Strategy)
// Класс Player реализует стратегию управления игроком
class Player : public Entity {
public:
    int lives;

    Player(int startX, int startY)
        : Entity(startX, startY, 0, 'P'), lives(3) {}

    void update(vector<Entity*>& entities) override {
        // Движение игрока
        if (kbhit()) {
            char key = getch();
            switch (key) {
                case 'a': // Движение влево
                    if (x > 0) x--;
                    break;
                case 'd': // Движение вправо
                    if (x < 79) x++;
                    break;
                case ' ': // Стрельба
                    entities.push_back(new Bullet(x, y - 1, -1)); // Пуля движется вверх
                    break;
                case 'q': // Выход из игры
                    exit(0);
            }
        }
    }
};

// Шаблон "Компоновщик" (Composite)
// Класс Game управляет всеми сущностями как единой композицией
class Game {
private:
    vector<Entity*> entities;
    Player* player;
    bool gameOn;
    int score;

public:
    Game() : gameOn(true), score(0) {
        player = new Player(40, 20); // Начальная позиция игрока
        entities.push_back(player);

        // Создание пришельцев
        for (int i = 0; i < 5; ++i) {
            entities.push_back(new Alien(i * 15, 1, 1, 'V'));
            entities.push_back(new Alien(i * 15 + 5, 2, 1, 'O', true)); // Стреляющие пришельцы
        }
    }

    ~Game() {
        for (auto entity : entities) {
            delete entity;
        }
    }

    void update() {
        for (auto entity : entities) {
            entity->update(entities); // Передаем список объектов для обновления
        }

        // Проверка столкновений
        for (size_t i = 0; i < entities.size(); ++i) {
            for (size_t j = 0; j < entities.size(); ++j) {
                if (i != j && entities[i]->x == entities[j]->x && entities[i]->y == entities[j]->y) {
                    // Логика столкновения
                    if (dynamic_cast<Bullet*>(entities[i]) && dynamic_cast<Alien*>(entities[j])) {
                        cout << "Bullet hit an alien!" << endl;
                        delete entities[j]; // Удаляем пришельца
                        entities.erase(entities.begin() + j);
                        score += 10; // Увеличиваем счет
                    } else if (dynamic_cast<Bullet*>(entities[i]) && dynamic_cast<Player*>(entities[j])) {
                        cout << "Alien hit the player!" << endl;
                        player->lives--; // Уменьшаем жизни игрока
                        delete entities[i]; // Удаляем пулю
                        entities.erase(entities.begin() + i);
                        if (player->lives <= 0) {
                            gameOn = false; // Игра окончена
                        }
                    }
                }
            }
        }

        // Проверка на поражение (пришельцы достигли уровня игрока)
        for (auto entity : entities) {
            if (dynamic_cast<Alien*>(entity) && entity->y >= player->y) {
                gameOn = false; // Игра окончена
                cout << "Aliens reached the player! Game Over!" << endl;
            }
        }

        // Проверка на победу (все пришельцы уничтожены)
        bool allAliensDestroyed = true;
        for (auto entity : entities) {
            if (dynamic_cast<Alien*>(entity)) {
                allAliensDestroyed = false;
                break;
            }
        }
        if (allAliensDestroyed) {
            gameOn = false; // Игра окончена
            cout << "All aliens destroyed! You win!" << endl;
        }
    }

    void draw() {
        system("clear"); // Очистка консоли
        for (int i = 0; i < 25; ++i) {
            for (int j = 0; j < 80; ++j) {
                bool drawn = false;
                for (auto entity : entities) {
                    if (entity->x == j && entity->y == i) {
                        cout << entity->symbol;
                        drawn = true;
                        break;
                    }
                }
                if (!drawn) cout << ' ';
            }
            cout << endl;
        }
        cout << "Score: " << score << " Lives: " << player->lives << endl;
    }

    void run() {
        while (gameOn) {
            player->update(entities); // Обработка ввода игрока
            update();
            draw();
            usleep(100000); // Задержка для управления скоростью игры
        }
        cout << "Final Score: " << score << endl;
    }
};

int main() {
    Game game;
    game.run();
    return 0;
}