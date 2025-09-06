#include <GL/glut.h>
#include <cmath>
#include <iostream>
#include <map>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <algorithm>
#include <string> // for score text

// Game constants
const float FISH_SCALE = 0.5f;
const float FISH_SPEED = 0.1f;
const float SMALL_FISH_SCALE = 0.2f;
const float BOUND_LEFT = -5.0f;
const float BOUND_RIGHT = 5.0f;
const float BOUND_BOTTOM = -5.0f;
const float BOUND_TOP = 5.0f;
const int NUM_SMALL_FISH = 10;
const int NUM_BUBBLES = 30;

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

float fishX = -1.0f, fishY = 1.0f;
bool flipFish = false;
std::map<int, bool> keyState;
int score = 0; // [SCORE FEATURE]

struct SmallFish {
    float x, y;
    float dx, dy;
    bool eaten = false;
};

struct Bubble {
    float x, y;
    float radius;
    float speed;
};

std::vector<SmallFish> smallFishes;
std::vector<Bubble> bubbles;

// Utility: draw a filled circle
void drawCircle(float cx, float cy, float r, int num_segments) {
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(cx, cy);
    for (int i = 0; i <= num_segments; i++) {
        float angle = 2.0f * M_PI * i / num_segments;
        glVertex2f(cx + r * cos(angle), cy + r * sin(angle));
    }
    glEnd();
}

// Draw underwater gradient background
void drawUnderwaterGradient() {
    glBegin(GL_QUADS);
    glColor3f(0.0f, 0.4f, 0.7f);
    glVertex2f(-5.0f, 5.0f);
    glVertex2f(5.0f, 5.0f);
    glColor3f(0.0f, 0.1f, 0.3f);
    glVertex2f(5.0f, -5.0f);
    glVertex2f(-5.0f, -5.0f);
    glEnd();
}

// Draw seafloor
void drawSeafloor() {
    glColor3f(0.5f, 0.5f, 0.4f);
    glBegin(GL_QUADS);
    glVertex2f(-5.0f, -3.5f);
    glVertex2f(5.0f, -3.5f);
    glVertex2f(5.0f, -5.0f);
    glVertex2f(-5.0f, -5.0f);
    glEnd();
}

// Draw bubble objects
void drawBubbles() {
    glColor4f(0.8f, 0.9f, 1.0f, 0.6f);
    for (const auto& b : bubbles) {
        drawCircle(b.x, b.y, b.radius, 20);
    }
}

// Draw buildings and ruins
void drawBuilding(float x, float y, float width, float height) {
    glPushMatrix();
    glTranslatef(x, y, 0);
    glColor3f(0.3f, 0.3f, 0.3f);
    glBegin(GL_QUADS);
    glVertex2f(0, 0);
    glVertex2f(width, 0);
    glVertex2f(width, height);
    glVertex2f(0, height);
    glEnd();

    glColor3f(0.0f, 0.0f, 0.0f);
    glBegin(GL_TRIANGLES);
    glVertex2f(width * 0.3f, height * 0.6f);
    glVertex2f(width * 0.5f, height * 0.4f);
    glVertex2f(width * 0.4f, height * 0.2f);
    glEnd();
    glPopMatrix();
}

void drawBrokenColumn(float x, float y, float h) {
    glColor3f(0.4f, 0.4f, 0.4f);
    glBegin(GL_QUADS);
    glVertex2f(x, y);
    glVertex2f(x + 0.25f, y);
    glVertex2f(x + 0.25f, y + h);
    glVertex2f(x, y + h);
    glEnd();

    glColor3f(0.1f, 0.1f, 0.1f);
    glBegin(GL_TRIANGLES);
    glVertex2f(x, y + h);
    glVertex2f(x + 0.125f, y + h + 0.25f);
    glVertex2f(x + 0.25f, y + h);
    glEnd();
}

void drawDebris() {
    glColor3f(0.2f, 0.2f, 0.2f);
    for (float i = -4.5f; i < 4.5f; i += 1.0f) {
        glBegin(GL_TRIANGLES);
        glVertex2f(i, -3.5f);
        glVertex2f(i + 0.25f, -3.0f);
        glVertex2f(i + 0.5f, -3.5f);
        glEnd();
    }
}

// Fish drawing
void drawFish(float x, float y, float sc, bool flip) {
    glPushMatrix();
    glTranslatef(x, y, 0.0f);
    if (flip)
        glScalef(-1.0f, 1.0f, 1.0f);

    glColor3f(1.0, 0.5, 0.0);
    glBegin(GL_POLYGON);
    glVertex2f(0, sc);
    glVertex2f(sc, 0);
    glVertex2f(0, -sc);
    glVertex2f(-sc, 0);
    glEnd();

    glColor3f(1.0, 0.0, 0.3);
    glBegin(GL_TRIANGLES);
    glVertex2f(-sc, 0);
    glVertex2f(-sc * 1.5f, sc * 0.5f);
    glVertex2f(-sc * 1.5f, -sc * 0.5f);
    glEnd();

    glColor3f(0.0, 0.0, 0.0);
    drawCircle(sc * 0.3f, sc * 0.3f, sc * 0.1f, 30);
    glPopMatrix();
}

void drawSmallFish(float x, float y, float sc) {
    glColor3f(0.2f, 1.0f, 0.6f);
    glBegin(GL_POLYGON);
    glVertex2f(x, y + sc);
    glVertex2f(x + sc, y);
    glVertex2f(x, y - sc);
    glVertex2f(x - sc, y);
    glEnd();

    glColor3f(0.2f, 0.0f, 0.6f);
    drawCircle(x + (sc * 0.3f), y + (sc * 0.3f), (sc * 0.1f), 20);
}

// Score text
void drawScore() {
    std::string scoreText = "Score: " + std::to_string(score);
    glColor3f(1.0f, 1.0f, 1.0f);
    glRasterPos2f(BOUND_LEFT + 0.2f, BOUND_TOP - 0.5f);
    for (char c : scoreText) {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, c);
    }
}

// Collision detection
bool checkCollision(float x1, float y1, float r1, float x2, float y2, float r2) {
    float dx = x1 - x2;
    float dy = y1 - y2;
    float distSq = dx * dx + dy * dy;
    float radSum = r1 + r2;
    return distSq <= radSum * radSum;
}

// Respawn small fish
void respawnFish(SmallFish &f) {
    f.x = BOUND_LEFT + static_cast<float>(rand()) / RAND_MAX * (BOUND_RIGHT - BOUND_LEFT);
    f.y = BOUND_BOTTOM + static_cast<float>(rand()) / RAND_MAX * (BOUND_TOP - BOUND_BOTTOM);
    f.dx = ((rand() % 100) / 100.0f - 0.5f) * 0.15f;
    f.dy = ((rand() % 100) / 100.0f - 0.5f) * 0.15f;
    f.eaten = false;
}

void display() {
    glClear(GL_COLOR_BUFFER_BIT);
    drawUnderwaterGradient();
    drawSeafloor();
    drawBubbles();

    drawBuilding(-4.5f, -3.5f, 1.5f, 3.0f);
    drawBuilding(-2.5f, -3.5f, 1.2f, 2.5f);
    drawBuilding(0.0f, -3.5f, 1.8f, 3.5f);
    drawBuilding(2.5f, -3.5f, 1.0f, 2.0f);

    drawBrokenColumn(-3.0f, -3.5f, 1.5f);
    drawBrokenColumn(1.75f, -3.5f, 1.25f);
    drawDebris();

    drawFish(fishX, fishY, FISH_SCALE, flipFish);

    for (auto &f : smallFishes) {
        if (!f.eaten)
            drawSmallFish(f.x, f.y, SMALL_FISH_SCALE);
    }

    drawScore(); // [SCORE FEATURE]
    glutSwapBuffers();
}

// Update game logic
void update(int value) {
    if (keyState[GLUT_KEY_LEFT]) { fishX -= FISH_SPEED; flipFish = true; }
    if (keyState[GLUT_KEY_RIGHT]) { fishX += FISH_SPEED; flipFish = false; }
    if (keyState[GLUT_KEY_UP]) fishY += FISH_SPEED;
    if (keyState[GLUT_KEY_DOWN]) fishY -= FISH_SPEED;

    fishX = std::max(BOUND_LEFT + FISH_SCALE * 1.5f, std::min(BOUND_RIGHT - FISH_SCALE * 1.5f, fishX));
    fishY = std::max(BOUND_BOTTOM + FISH_SCALE, std::min(BOUND_TOP - FISH_SCALE, fishY));

    for (auto &f : smallFishes) {
        if (f.eaten) continue;
        f.x += f.dx; f.y += f.dy;
        if (f.x < BOUND_LEFT || f.x > BOUND_RIGHT) f.dx *= -1;
        if (f.y < BOUND_BOTTOM || f.y > BOUND_TOP) f.dy *= -1;

        if (checkCollision(fishX, fishY, FISH_SCALE, f.x, f.y, SMALL_FISH_SCALE)) {
            f.eaten = true;
            score++;
            respawnFish(f);
        }
    }

    for (auto &b : bubbles) {
        b.y += b.speed;
        if (b.y > BOUND_TOP) {
            b.y = BOUND_BOTTOM;
            b.x = BOUND_LEFT + static_cast<float>(rand()) / RAND_MAX * (BOUND_RIGHT - BOUND_LEFT);
        }
    }

    glutPostRedisplay();
    glutTimerFunc(16, update, 0);
}

// Input
void specialKeyDown(int key, int, int) { keyState[key] = true; }
void specialKeyUp(int key, int, int) { keyState[key] = false; }

// Initialize small fish
void initSmallFishes() {
    smallFishes.clear();
    for (int i = 0; i < NUM_SMALL_FISH; ++i) {
        SmallFish f;
        respawnFish(f);
        smallFishes.push_back(f);
    }
}

// Initialize bubbles
void initBubbles() {
    bubbles.clear();
    for (int i = 0; i < NUM_BUBBLES; ++i) {
        Bubble b;
        b.x = BOUND_LEFT + static_cast<float>(rand()) / RAND_MAX * (BOUND_RIGHT - BOUND_LEFT);
        b.y = BOUND_BOTTOM + static_cast<float>(rand()) / RAND_MAX * 5.0f;
        b.radius = 0.05f + static_cast<float>(rand()) / RAND_MAX * 0.05f;
        b.speed = 0.01f + static_cast<float>(rand()) / RAND_MAX * 0.02f;
        bubbles.push_back(b);
    }
}

// OpenGL initial
void init() {
    glClearColor(0.0, 0.3, 0.5, 1.0);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(BOUND_LEFT, BOUND_RIGHT, BOUND_BOTTOM, BOUND_TOP);
    srand(static_cast<unsigned>(time(0)));
    initSmallFishes();
    initBubbles();
}

// Main
int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
    glutInitWindowSize(800, 600);
    glutCreateWindow("Fish Game in Destroyed Underwater City");
    init();
    glutDisplayFunc(display);
    glutSpecialFunc(specialKeyDown);
    glutSpecialUpFunc(specialKeyUp);
    glutTimerFunc(0, update, 0);
    glutMainLoop();
    return 0;
}
