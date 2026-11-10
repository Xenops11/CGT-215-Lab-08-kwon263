// CGT-215-Lab-08-kwon263.cpp : This file contains the 'main' function. Program execution begins and ends there.
//
#include <iostream>
#include <SFML/Graphics.hpp>
#include <SFPhysics.h>
#include <vector>
#include <SFML/Audio.hpp>

using namespace std;
using namespace sf;
using namespace sfp;

const float KB_SPEED = 0.2f; // Crossbow movement speed

void LoadTex(Texture& tex, string filename) {
    if (!tex.loadFromFile(filename)) {
        cout << "Could not load " << filename << endl;
    }
}

// Function to move the crossbow left and right
void MoveCrossbow(PhysicsSprite& crossbow, int elapsedMS) {
    if (Keyboard::isKeyPressed(Keyboard::Right)) {
        Vector2f newPos(crossbow.getCenter());
        newPos.x = newPos.x + (KB_SPEED * elapsedMS);
        crossbow.setCenter(newPos);
    }
    if (Keyboard::isKeyPressed(Keyboard::Left)) {
        Vector2f newPos(crossbow.getCenter());
        newPos.x = newPos.x - (KB_SPEED * elapsedMS);
        crossbow.setCenter(newPos);
    }
}

Vector2f GetTextSize(Text text) {
    FloatRect r = text.getGlobalBounds();
    return Vector2f(r.width, r.height);
}

int main()
{
    RenderWindow window(VideoMode(800, 600), "Duck Hunter");
    World world(Vector2f(0, 0));

    int score(0);
    int arrows(5); // Total 5 arrows provided[cite: 1]

    SoundBuffer popBuffer;
    if (!popBuffer.loadFromFile("balloonpop.ogg")) {
        cout << "Could not load balloonpop.ogg" << endl;
        exit(5);
    }
    Sound popSound;
    popSound.setBuffer(popBuffer);

    Music music;
    if (!music.openFromFile("circus.ogg")) {
        cout << "Failed to load circus.ogg ";
        exit(6);
    }
    music.play();

    // Setup crossbow position (bottom center)
    PhysicsSprite& crossBow = *new PhysicsSprite();
    Texture cbowTex;
    LoadTex(cbowTex, "images/crossbow.png");
    crossBow.setTexture(cbowTex);
    Vector2f sz = crossBow.getSize();
    crossBow.setCenter(Vector2f(400, 600 - (sz.y / 2)));

    // Arrow setup
    PhysicsSprite arrow;
    Texture arrowTex;
    LoadTex(arrowTex, "images/arrow.png");
    arrow.setTexture(arrowTex);
    bool drawingArrow(false);

    // Setup boundary walls
    PhysicsRectangle top;
    top.setSize(Vector2f(800, 10));
    top.setCenter(Vector2f(400, 5));
    top.setStatic(true);
    world.AddPhysicsBody(top);

    // Setup right boundary (to remove ducks exiting the screen)[cite: 1]
    PhysicsRectangle right;
    right.setSize(Vector2f(10, 600));
    right.setCenter(Vector2f(815, 300));
    right.setStatic(true);
    world.AddPhysicsBody(right);

    // Top boundary and arrow collision handler
    top.onCollision = [&drawingArrow, &world, &arrow](PhysicsBodyCollisionResult result) {
        if (result.object2 == arrow) {
            drawingArrow = false;
            world.RemovePhysicsBody(arrow);
        }
        };

    // Duck texture and shape list setup[cite: 1]
    Texture duckTex;
    LoadTex(duckTex, "images/duck.png");
    PhysicsShapeList<PhysicsSprite> ducks;

    // Right boundary and duck collision handler (remove duck when it goes off screen)[cite: 1]
    right.onCollision = [&world, &ducks](PhysicsBodyCollisionResult result) {
        for (PhysicsShape& duckShape : ducks) {
            PhysicsSprite& duck = (PhysicsSprite&)duckShape;
            if (result.object2 == duck) {
                world.RemovePhysicsBody(duck);
                ducks.QueueRemove(duck);
                break;
            }
        }
        };

    Font fnt;
    if (!fnt.loadFromFile("arial.ttf")) {
        cout << "Could not load font." << endl;
        exit(3);
    }

    // Initialize UI text objects
    Text arrowCountText;
    arrowCountText.setFont(fnt);
    arrowCountText.setCharacterSize(20);
    arrowCountText.setPosition(Vector2f(20, 560));

    Text scoreText;
    scoreText.setFont(fnt);
    scoreText.setCharacterSize(20);

    Clock clock;
    Time lastTime(clock.getElapsedTime());
    Time currentTime(lastTime);

    // Duck spawn timer variables (ms)[cite: 1]
    long duckTimer = 0;
    const long DUCK_INTERVAL = 1500; // Spawn a duck every 1.5 seconds

    // Lambda function: Spawn duck and define individual collisions[cite: 1]
    auto SpawnDuck = [&]() {
        PhysicsSprite& duck = ducks.Create();
        duck.setTexture(duckTex);

        // Adjust duck hitbox size (optimized for actual body collision)
        duck.setSize(Vector2f(90, 65));

        duck.setCenter(Vector2f(-50, 100)); // Start off-screen to the left
        duck.setVelocity(Vector2f(0.45f, 0));  // Move from left to right with fast speed

        world.AddPhysicsBody(duck);

        // Individual duck collision handler with arrow
        duck.onCollision = [&drawingArrow, &world, &arrow, &duck, &ducks, &score, &popSound](PhysicsBodyCollisionResult result) {
            if (result.object2 == arrow) {
                popSound.play();
                drawingArrow = false;
                world.RemovePhysicsBody(arrow);
                world.RemovePhysicsBody(duck);
                ducks.QueueRemove(duck);
                score += 10;
            }
            };
        };

    // Spawn the first duck at game start
    SpawnDuck();

    // Main game loop: Runs while arrows remain or an arrow is currently flying[cite: 2]
    while (window.isOpen() && (arrows > 0 || drawingArrow)) {
        Event event;
        while (window.pollEvent(event)) {
            if (event.type == Event::Closed) {
                window.close();
            }
        }

        currentTime = clock.getElapsedTime();
        Time deltaTime = currentTime - lastTime;
        long deltaMS = deltaTime.asMilliseconds();

        if (deltaMS > 9) {
            lastTime = currentTime;
            world.UpdatePhysics(deltaMS);

            // Handle crossbow movement[cite: 1, 2]
            MoveCrossbow(crossBow, deltaMS);

            // Accumulate duck spawn timer and create ducks continuously[cite: 1]
            duckTimer += deltaMS;
            if (duckTimer >= DUCK_INTERVAL) {
                duckTimer = 0;
                SpawnDuck();
            }

            // Spacebar input handling (fire arrow)[cite: 1, 2]
            if (Keyboard::isKeyPressed(Keyboard::Space) && !drawingArrow && arrows > 0) {
                drawingArrow = true;
                arrows--;
                arrow.setCenter(crossBow.getCenter());
                arrow.setVelocity(Vector2f(0, -1));
                world.AddPhysicsBody(arrow);
            }

            window.clear();

            if (drawingArrow) {
                window.draw(arrow);
            }

            // Update and render ducks[cite: 1]
            ducks.DoRemovals();
            for (PhysicsShape& duck : ducks) {
                window.draw((PhysicsSprite&)duck);
            }

            window.draw(crossBow);

            // 1. Update and draw remaining arrows (bottom-left)[cite: 1, 2]
            arrowCountText.setString("Number of Arrows Left: " + to_string(arrows));
            window.draw(arrowCountText);

            // 2. Update and draw score (bottom-right)[cite: 1, 2]
            scoreText.setString("Score: " + to_string(score));
            Vector2f scoreSz = GetTextSize(scoreText);
            scoreText.setPosition(Vector2f(780 - scoreSz.x, 560));
            window.draw(scoreText);

            window.display();
        }
    }

    // Prevent residual spacebar input carryover after firing the last arrow
    while (Keyboard::isKeyPressed(Keyboard::Space)) {
        // Wait until user releases spacebar
    }

    // Game Over & Wait loop
    bool waitingForSpace = true;
    while (window.isOpen() && waitingForSpace) {
        Event event;
        while (window.pollEvent(event)) {
            if (event.type == Event::Closed) {
                window.close();
                waitingForSpace = false;
            }
            // Wait until user presses spacebar again to close/exit[cite: 5]
            if (event.type == Event::KeyPressed && event.key.code == Keyboard::Space) {
                waitingForSpace = false;
            }
        }

        window.clear();

        // Keep rendering the last frame elements and Game Over message[cite: 5]
        ducks.DoRemovals();
        for (PhysicsShape& duck : ducks) {
            window.draw((PhysicsSprite&)duck);
        }
        window.draw(crossBow);

        arrowCountText.setString("Number of Arrows Left: " + to_string(arrows));
        window.draw(arrowCountText);

        scoreText.setString("Score: " + to_string(score));
        Vector2f scoreSz = GetTextSize(scoreText);
        scoreText.setPosition(Vector2f(780 - scoreSz.x, 560));
        window.draw(scoreText);

        Text gameOverText;
        gameOverText.setString("GAME OVER");
        gameOverText.setFont(fnt);
        gameOverText.setCharacterSize(40);
        Vector2f gSz = GetTextSize(gameOverText);
        gameOverText.setPosition(400 - (gSz.x / 2), 300 - (gSz.y / 2));
        window.draw(gameOverText);

        window.display();
    }

    return 0;
}
// Run program: Ctrl + F5 or Debug > Start Without Debugging menu
// Debug program: F5 or Debug > Start Debugging menu

// Tips for Getting Started: 
//   1. Use the Solution Explorer window to add/manage files
//   2. Use the Team Explorer window to connect to source control
//   3. Use the Output window to see build output and other messages
//   4. Use the Error List window to view errors
//   5. Go to Project > Add New Item to create new code files, or Project > Add Existing Item to add existing code files to the project
//   6. In the future, to open this project again, go to File > Open > Project and select the .sln file
