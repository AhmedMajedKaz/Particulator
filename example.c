#include "raylib.h"
#include "raymath.h"
#define RAYGUI_IMPLEMENTATION
#include "raygui.h"

#include "box2d/box2d.h"
#include <math.h>

#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 600

#define MAXIMUM_POINTS_NUM 4096
#define PARTICLE_RADIUS 4.0f
#define FPS 60

#define MAXIMUM_SHOCKWAVE_NUM 128
#define SHOCKWAVE_POWER 300.0f
#define SHOCKWAVE_EDGE_POWER 5000.0f

#define MAGNATIC_COEFFICIENT 800.0f

typedef struct shockwave
{
    int active;
    float x, y;
    float t;
    float direction;
} shockwave;

int main(int argc, const char **argv)
{
    InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Particle Simulator");

    // Set the path for raylib to find it's assets
    int splitNum = 0;
    const char **split = TextSplit(argv[0], '\\', &splitNum);
    const char *path = TextJoin(split, splitNum - 1, "\\");
    ChangeDirectory(path);

    // Initialize All Arrays
    b2BodyId pointsIds[MAXIMUM_POINTS_NUM];
    Vector2 prevVelocity[MAXIMUM_POINTS_NUM];
    shockwave shockwaves[MAXIMUM_SHOCKWAVE_NUM] = {0};
    int pointsNum = 0;

    SetTargetFPS(FPS);

    // Initialize box2d world
    b2WorldDef worldDef = b2DefaultWorldDef();
    worldDef.gravity = (b2Vec2){0.0f, -10.0f};
    b2WorldId worldId = b2CreateWorld(&worldDef);
    b2BodyDef groundBodyDef = b2DefaultBodyDef();

    // Bottom Ground
    groundBodyDef.position = (b2Vec2){0.0f, -10.0f};
    b2BodyId groundId = b2CreateBody(worldId, &groundBodyDef);
    b2Polygon groundBox = b2MakeBox(WINDOW_WIDTH, 10.0f);
    b2ShapeDef groundShapeDef = b2DefaultShapeDef();
    b2CreatePolygonShape(groundId, &groundShapeDef, &groundBox);

    // Right Ground
    groundBodyDef.position = (b2Vec2){-10.0f, -10.0f};
    groundId = b2CreateBody(worldId, &groundBodyDef);
    groundBox = b2MakeBox(10.0f, WINDOW_HEIGHT);
    groundShapeDef = b2DefaultShapeDef();
    b2CreatePolygonShape(groundId, &groundShapeDef, &groundBox);

    // LEFT Ground
    groundBodyDef.position = (b2Vec2){WINDOW_WIDTH + 10.0f, -10.0f};
    groundId = b2CreateBody(worldId, &groundBodyDef);
    groundBox = b2MakeBox(10.0f, WINDOW_HEIGHT);
    groundShapeDef = b2DefaultShapeDef();
    b2CreatePolygonShape(groundId, &groundShapeDef, &groundBox);

    // UP Ground
    groundBodyDef.position = (b2Vec2){0, WINDOW_HEIGHT + 10.0f};
    groundId = b2CreateBody(worldId, &groundBodyDef);
    groundBox = b2MakeBox(WINDOW_WIDTH, 10.0f);
    groundShapeDef = b2DefaultShapeDef();
    b2CreatePolygonShape(groundId, &groundShapeDef, &groundBox);

    // Dynamic body Definition
    b2BodyDef bodyDef = b2DefaultBodyDef();
    bodyDef.type = b2_dynamicBody;
    b2ShapeDef shapeDef = b2DefaultShapeDef();
    shapeDef.density = 1.0f;
    shapeDef.material.friction = 0.3f;

    // Render Texture (two to swap between them)
    RenderTexture2D target = LoadRenderTexture(WINDOW_WIDTH, WINDOW_HEIGHT);
    RenderTexture2D target2 = LoadRenderTexture(WINDOW_WIDTH, WINDOW_HEIGHT);

    // Load Shader
    Shader shader = LoadShader(0, "shockwave.fs");
    int resLoc = GetShaderLocation(shader, "resolution");
    int clickLoc = GetShaderLocation(shader, "clickPos");
    int strengthLoc = GetShaderLocation(shader, "strength");

    Vector2 resolution = {WINDOW_WIDTH, WINDOW_HEIGHT};
    SetShaderValue(shader, resLoc, &resolution, SHADER_UNIFORM_VEC2);

    // Some Values for the drawing and simulating process
    float timeStep = 5.0f / (float)FPS;
    int subStepCount = 4;
    const Vector2 inverseSize = (Vector2){1.0f / (float)WINDOW_WIDTH, 1.0f / (float)WINDOW_HEIGHT};
    const float sizeLength = WINDOW_HEIGHT + 30;

    // GUI values
    Rectangle inverseBtn = {WINDOW_WIDTH - 150, 10, 20, 20};
    bool inverseBool = false;

    bool magnaticCursor = false;

    while (!WindowShouldClose())
    {
        const float delta = GetFrameTime();

        // Controlls
        if (
            IsMouseButtonDown(MOUSE_BUTTON_LEFT) &&
            pointsNum < MAXIMUM_POINTS_NUM &&
            !CheckCollisionPointRec(GetMousePosition(), inverseBtn))
        {
            Vector2 newPoint = Vector2Add(GetMousePosition(), (Vector2){GetRandomValue(-5, 5), GetRandomValue(-5, 5)});
            newPoint = (Vector2){newPoint.x, WINDOW_HEIGHT - newPoint.y};

            bodyDef.position = (b2Vec2){newPoint.x, newPoint.y};
            pointsIds[pointsNum] = b2CreateBody(worldId, &bodyDef);
            b2Circle pointCircle = (b2Circle){(b2Vec2){0, 0}, PARTICLE_RADIUS};
            b2CreateCircleShape(pointsIds[pointsNum++], &shapeDef, &pointCircle);
        }

        if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT))
        {
            Vector2 position = GetMousePosition();
            position = (Vector2){position.x, WINDOW_HEIGHT - position.y};

            const Vector2 click = Vector2Multiply(position, inverseSize);
            int i = 0;
            while (shockwaves[i].active)
            {
                i++;
            }
            if (i < MAXIMUM_SHOCKWAVE_NUM)
            {
                shockwaves[i].active = true;
                shockwaves[i].x = position.x;
                shockwaves[i].y = position.y;
                shockwaves[i].direction = inverseBool ? -1 : 1;
                shockwaves[i].t = shockwaves[i].direction > 0 ? 0 : 1;
            }
        }

        // Magnatic Effect
        if (IsKeyPressed(KEY_C))
        {
            magnaticCursor = !magnaticCursor;
        }

        if (magnaticCursor)
        {
            Vector2 position = GetMousePosition();
            position = (Vector2){position.x, WINDOW_HEIGHT - position.y};
            for (int j = 0; j < pointsNum; j++)
            {
                b2Vec2 point = b2Body_GetWorldPoint(pointsIds[j], (b2Vec2){0, 0});
                Vector2 direction = (Vector2){position.x - point.x, position.y - point.y};
                float distance = Vector2Length(direction);

                Vector2 normalizedDirection = Vector2Normalize(direction);
                b2Body_ApplyForce(pointsIds[j],
                                  (b2Vec2){normalizedDirection.x * MAGNATIC_COEFFICIENT, normalizedDirection.y * MAGNATIC_COEFFICIENT},
                                  (b2Vec2){position.x, position.y},
                                  true);
            }
        }

        // Update ShockWaves
        for (int i = 0; i < MAXIMUM_SHOCKWAVE_NUM; i++)
        {
            if (shockwaves[i].active)
            {
                shockwaves[i].t += delta * 0.4f * shockwaves[i].direction;
                if (shockwaves[i].t >= 1 || shockwaves[i].t < 0)
                {
                    shockwaves[i].active = false;
                }
                else
                {
                    float maxT = shockwaves[i].t * sizeLength;
                    float minT = shockwaves[i].t * (sizeLength - 60.0f);
                    Vector2 center = (Vector2){shockwaves[i].x, shockwaves[i].y};
                    for (int j = 0; j < pointsNum; j++)
                    {
                        b2Vec2 point = b2Body_GetWorldPoint(pointsIds[j], (b2Vec2){0, 0});
                        Vector2 direction = (Vector2){center.x - point.x, center.y - point.y};
                        float distance = Vector2Length(direction);

                        if (distance < maxT)
                        {
                            Vector2 normalizedDirection = Vector2Normalize(direction);
                            b2Body_ApplyForce(pointsIds[j],
                                              (b2Vec2){normalizedDirection.x * (distance > minT ? -SHOCKWAVE_EDGE_POWER : -SHOCKWAVE_POWER) * shockwaves[i].direction, normalizedDirection.y * (distance > minT ? -SHOCKWAVE_EDGE_POWER : -SHOCKWAVE_POWER) * shockwaves[i].direction},
                                              (b2Vec2){center.x, center.y},
                                              true);
                        }
                    }
                }
            }
        }

        // Update Physics Engine
        b2World_Step(worldId, timeStep, subStepCount);

        // Initialize Render Texture Pointers
        RenderTexture2D *current = &target;
        RenderTexture2D *next = &target2;

        // Render To first Texture
        BeginTextureMode(*current);
        ClearBackground((Color){40, 40, 40, 255});
        for (int i = 0; i < pointsNum; i++)
        {
            b2Vec2 position = b2Body_GetPosition(pointsIds[i]);
            b2Vec2 velocityVec = b2Body_GetLinearVelocity(pointsIds[i]);
            Vector2 velocityDifference = Vector2Subtract(prevVelocity[i], (Vector2){velocityVec.x, velocityVec.y});
            float prevVelocityLength = Vector2Length(prevVelocity[i]);
            prevVelocity[i] = (Vector2){velocityVec.x, velocityVec.y};
            float velocity = Vector2Length(prevVelocity[i]);
            float differnce = Vector2Length(velocityDifference);
            Color color = ColorLerp(BLUE, RED, velocity * 0.01f > 1.0f ? 1 : velocity * 0.01f);
            DrawCircle(position.x, WINDOW_HEIGHT - position.y, PARTICLE_RADIUS, color);
        }

        char frameText[50], particleText[50];
        sprintf(frameText, "FPS: %d", (int)(1.0f / GetFrameTime()));
        sprintf(particleText, "Particles: %d", pointsNum);
        DrawText(frameText, 0, 0, 12, WHITE);
        DrawText(particleText, 0, 12, 12, WHITE);
        EndTextureMode();

        // Add Shockwave Effects by swapping textures and applying the shader to them
        for (int i = 0; i < MAXIMUM_SHOCKWAVE_NUM; i++)
        {
            if (shockwaves[i].active)
            {
                BeginTextureMode(*next);
                BeginShaderMode(shader);
                Vector2 center = Vector2Multiply((Vector2){shockwaves[i].x, shockwaves[i].y}, inverseSize);
                SetShaderValue(shader, clickLoc, &center, SHADER_UNIFORM_VEC2);
                SetShaderValue(shader, strengthLoc, &shockwaves[i].t, SHADER_UNIFORM_FLOAT);
                ClearBackground((Color){40, 40, 40, 255});
                // DrawTexture(current->texture, 0, 0, RAYWHITE);
                DrawTextureRec(current->texture, (Rectangle){0, 0, current->texture.width, -current->texture.height}, (Vector2){0, 0}, RAYWHITE);
                EndShaderMode();
                EndTextureMode();

                // Swap textures
                RenderTexture2D *temp = current;
                current = next;
                next = temp;
            }
        }

        // Drawing
        BeginDrawing();
        ClearBackground((Color){40, 40, 40, 255});
        DrawTextureRec(current->texture, (Rectangle){0, 0, target.texture.width, -target.texture.height}, (Vector2){0, 0}, RAYWHITE);

        // GUI
        inverseBool = GuiCheckBox(inverseBtn, "Inverse ShockWave", inverseBool);

        EndDrawing();
    }

    // Dispose of all Entities
    UnloadShader(shader);
    b2DestroyWorld(worldId);
    CloseWindow();
}