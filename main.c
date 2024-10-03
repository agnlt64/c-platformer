#include "include/raylib.h"

#include <stdio.h>

#define PLAYER_SPEED 7
#define PLAYER_SIZE 32
#define SKEL_SIZE 64

#define SCALE_FACTOR 2

// scaling factors are not the same but when rendreing, both sprites
// have the same hitbox by default. this is because the player sprite
// is 32*32 and the skeleton sprite is 64*64.
// so when we compute the render with of both sprites, we get:
// player: 32*2*3 = 192px
// skeleton: 64*2*1.5 = 192px
// but since the skeleton sprite is bigger, we need to 
// scale its hitbox down.
#define PLAYER_SCALE_FACTOR 3*SCALE_FACTOR
#define SKEL_SCALE_FACTOR 1.5*SCALE_FACTOR

#define JUMP_FORCE -350.0f
#define GRAVITY 600.0f

#define FONT_SIZE 20

bool debug = false;

Font font = {0};

Texture2D player_idle = {0};
Texture2D player_run = {0};
Texture2D player_jump = {0};
Texture2D player_attack1 = {0};
Texture2D player_attack3 = {0};
Texture2D player_death = {0};

Texture2D skeleton_idle = {0};
Texture2D skeleton_walk = {0};
Texture2D skeleton_hit = {0};
Texture2D skeleton_attack = {0};
Texture2D skeleton_death = {0};

enum {
    IDLE,
    JUMP,
    RUN,
    WALK,
    ATTACK_1,
    ATTACK_3,
    HIT,
    DEATH,
} Animation;

enum {
    LEFT  = -1,
    RIGHT = 1,
} Direction;

typedef struct {
    float timer;
    int   max_frames;
    int   num_frames;
    int   frame_width;
    int   frame;
} Animator;

typedef struct {
    int  width, height;
    int  scale;
    int  current_animation;
    int  direction;

    bool is_jumping;
    bool is_attacking;
    bool is_moving;

    Texture2D sprite;
    Rectangle hitbox;
    Vector2   position;
    Vector2   velocity;
    Animator  animator;
} Entity;

void create_hitbox(Entity* entity)
{
    entity->hitbox = (Rectangle){
        .x = entity->position.x + entity->width*2,
        .y = entity->position.y,
        .width = entity->width * 2,
        .height = entity->height * entity->scale - 50,
    };
}

Animator create_animator(int frame_width)
{
    return (Animator){
        .timer = 0.0f,
        .max_frames = 0,
        .num_frames = 0,
        .frame_width = frame_width,
        .frame = 0,
    };
}

Entity create_entity(Vector2 pos, int width, int height, int scale, Texture2D default_sprite)
{
    Entity entity = {
        .position = pos,
        .velocity = (Vector2){0, 0},
        .width = width,
        .height = height,
        .scale = scale,
        .is_jumping = false,
        .is_attacking = false,
        .is_moving = false,
        .current_animation = IDLE,
        .direction = RIGHT,
        .sprite = default_sprite,
        .animator = create_animator(width),
    };
    create_hitbox(&entity);
    return entity;
}

int ground(Entity entity)
{
    return GetScreenHeight() - entity.height * entity.scale;
}

Rectangle generate_platform()
{
    Rectangle rec = (Rectangle){
      .x = 100,
      .y = GetScreenHeight() - 100,
      .width = 100,
      .height = 20,  
    };
    return rec;
}

void apply_gravity(Entity* entity)
{
    int floor = 458;
    entity->velocity.y += GRAVITY * GetFrameTime();
    entity->position.y += entity->velocity.y * GetFrameTime();

    if (entity->position.y >= floor)
    {
        entity->position.y = floor;
        entity->velocity.y = 0;
        entity->is_jumping = false;
    }
}

void update_skeleton_hitbox(Entity* skeleton)
{
    if (skeleton->direction == RIGHT) skeleton->hitbox.x = skeleton->position.x;
    else skeleton->hitbox.x = skeleton->position.x + 50;
    skeleton->hitbox.y = skeleton->position.y;
}

void update_player_hitbox(Entity* player)
{
    player->hitbox.x = player->position.x + player->width*2;
    player->hitbox.y = player->position.y;
}

void update_player(Entity* player)
{
    // debug mode
    if (IsKeyPressed(KEY_D)) debug = !debug;

    update_player_hitbox(player);

    if (IsKeyPressed(KEY_SPACE) && !player->is_jumping)
    {
        player->current_animation = JUMP;
        player->velocity.y = JUMP_FORCE;
        player->is_jumping = true;
        player->is_moving = true;
    }
    else if (IsKeyDown(KEY_RIGHT) && player->position.x < GetScreenWidth() - player->width*PLAYER_SCALE_FACTOR)
    {
        player->current_animation = RUN;
        player->direction = RIGHT;
        player->position.x += PLAYER_SPEED;
        player->is_moving = true;
    }
    else if (IsKeyDown(KEY_LEFT) && player->position.x > -player->width*SCALE_FACTOR) // SCALE_FACTOR is intentional here
    {
        player->current_animation = RUN;
        player->direction = LEFT;
        player->position.x -= PLAYER_SPEED;
        player->is_moving = true;
    }
    else if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
    {
        player->current_animation = ATTACK_1;
        player->is_attacking = true;
    }
    else if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT))
    {
        player->current_animation = ATTACK_3;
        player->is_attacking = true;
    }
    else if (!player->is_jumping && !player->is_attacking)
    {
        player->current_animation = IDLE;
        player->is_moving = false;
    }

    apply_gravity(player);
}

void update_skeleton(Entity* skeleton, Entity* player)
{
    update_skeleton_hitbox(skeleton);
    if (skeleton->current_animation == WALK)
    {
        skeleton->is_moving = true;
    }

    if (skeleton->position.x > GetScreenWidth() - skeleton->width*SKEL_SCALE_FACTOR)
    {
        skeleton->direction = LEFT;
        skeleton->is_moving = true;
    }
    else if (skeleton->position.x < 0)
    {
        skeleton->direction = RIGHT;
        skeleton->is_moving = true;
    }

    if (skeleton->is_moving)
    {
        if (skeleton->direction == RIGHT)
        {
            skeleton->position.x += 1;
        }
        else
        {
            skeleton->position.x -= 1;
        }
    }

    if (CheckCollisionRecs(skeleton->hitbox, player->hitbox))
    {
        DrawTextEx(font, "COLLISION", (Vector2){10, 50}, FONT_SIZE, 0, BLACK);
        if (skeleton->direction == RIGHT)
        {
            skeleton->direction = LEFT;
            skeleton->position.x -= 1;
        }
        else
        {    
            skeleton->direction = RIGHT;
            skeleton->position.x += 1;
        }
    }

    apply_gravity(skeleton);
}

void update_animator(Entity* entity)
{
    entity->animator.timer += GetFrameTime();

    if (entity->animator.timer >= 0.1f)
    {
        entity->animator.timer = 0.0f;
        entity->animator.frame++;
    }

    entity->animator.frame_width = entity->sprite.width / entity->animator.num_frames;
    entity->animator.max_frames = entity->sprite.width / entity->animator.frame_width;

    if (entity->animator.frame == entity->animator.max_frames - 1)
    {
        if (entity->current_animation == ATTACK_1 || entity->current_animation == ATTACK_3)
        {
            entity->is_attacking = false;
        }
    }
    entity->animator.frame %= entity->animator.max_frames;
}

void draw_debug_ui(Entity entity)
{
    int spacing = 0;
    DrawTextEx(font, "DEBUG MODE", (Vector2){10, 10}, FONT_SIZE, spacing, BLACK);
    DrawTextEx(font, "Press D to toggle debug mode", (Vector2){10, 30}, FONT_SIZE, spacing, BLACK);

    const char* fps_text = TextFormat("%d FPS", GetFPS());
    Vector2 text_pos = (Vector2){GetScreenWidth() - MeasureTextEx(font, fps_text, FONT_SIZE, spacing).x - 10, 10};

    DrawTextEx(font, fps_text, text_pos, FONT_SIZE, spacing, BLACK);

    DrawRectangleRec(entity.hitbox, RED);
    Vector2 pos_pos = (Vector2){entity.hitbox.x, entity.hitbox.y - 20};
    DrawTextEx(font, TextFormat("X: %d, Y: %d", (int)entity.position.x, (int)entity.position.y), pos_pos, (int)(FONT_SIZE*0.9), spacing, BLACK);
}

void draw_entity(Entity entity)
{
    if (debug) draw_debug_ui(entity);

    Vector2 origin = {0, 0};
    Rectangle source = {
        entity.animator.frame_width * entity.animator.frame, 0,
        entity.animator.frame_width * entity.direction, entity.sprite.height
    };
    Rectangle dest = {
        entity.position.x, entity.position.y,
        entity.width * entity.scale, entity.height * entity.scale
    };

    DrawTexturePro(entity.sprite, source, dest, origin, 0, WHITE);
}

void update_player_animation(Entity* player)
{
    switch (player->current_animation)
    {
    case IDLE:
        player->sprite = player_idle;
        player->animator.num_frames = 7;
        break;
    case RUN:
        player->sprite = player_run;
        player->animator.num_frames = 8;
        break;
    case JUMP:
        player->sprite = player_jump;
        player->animator.num_frames = 5;
        break;
    case ATTACK_1:
        player->sprite = player_attack1;
        player->animator.num_frames = 6;
        break;
    case ATTACK_3:
        player->sprite = player_attack3;
        player->animator.num_frames = 6;
        break;
    case DEATH:
        player->sprite = player_death;
        player->animator.num_frames = 12;
        break;
    default:
        break;
    }
}

void update_skeleton_animation(Entity* skeleton)
{
    switch (skeleton->current_animation)
    {
    case IDLE:
        skeleton->sprite = skeleton_idle;
        skeleton->animator.num_frames = 4;
        break;
    case WALK:
        skeleton->sprite = skeleton_walk;
        skeleton->animator.num_frames = 12;
        break;
    case HIT:
        skeleton->sprite = skeleton_hit;
        skeleton->animator.num_frames = 3;
        break;
    case ATTACK_1:
        skeleton->sprite = skeleton_attack;
        skeleton->animator.num_frames = 13;
        break;
    case DEATH:
        skeleton->sprite = skeleton_death;
        skeleton->animator.num_frames = 13;
        break;
    default:
        break;
    }
}

void load_textures()
{
    player_idle     = LoadTexture("assets/player/idle.png");
    player_run      = LoadTexture("assets/player/run.png");
    player_jump     = LoadTexture("assets/player/jump.png");
    player_attack1  = LoadTexture("assets/player/attack1.png");
    player_attack3  = LoadTexture("assets/player/attack3.png");
    player_death    = LoadTexture("assets/player/death.png");

    skeleton_idle   = LoadTexture("assets/skeleton/idle.png");
    skeleton_walk   = LoadTexture("assets/skeleton/walk.png");
    skeleton_hit    = LoadTexture("assets/skeleton/hit.png");
    skeleton_attack = LoadTexture("assets/skeleton/attack.png");
    skeleton_death  = LoadTexture("assets/skeleton/death.png");
}

void cleanup()
{
    UnloadTexture(player_idle);
    UnloadTexture(player_run);
    UnloadTexture(player_jump);
    UnloadTexture(player_attack1);
    UnloadTexture(player_attack3);
    UnloadTexture(player_death);

    UnloadTexture(skeleton_idle);
    UnloadTexture(skeleton_walk);
    UnloadTexture(skeleton_hit);
    UnloadTexture(skeleton_attack);
    UnloadTexture(skeleton_death);
}

int main(void)
{
    InitWindow(800, 600, "Doodle Jump Animation");

    load_textures();
    font = LoadFont("./assets/fonts/BreatheFire-65pg.ttf");

    Entity player = create_entity((Vector2){-PLAYER_SIZE*SCALE_FACTOR, 200}, PLAYER_SIZE, PLAYER_SIZE, PLAYER_SCALE_FACTOR, player_idle);
    Entity skeleton = create_entity((Vector2){500, 200}, SKEL_SIZE, SKEL_SIZE, SKEL_SCALE_FACTOR, skeleton_walk);
    skeleton.current_animation = WALK;

    SetTargetFPS(60);

    while (!WindowShouldClose())
    {
        BeginDrawing();
        ClearBackground(RAYWHITE);

        Rectangle platform = generate_platform();
        DrawRectangleRec(platform, BLACK);

        update_player_animation(&player);
        update_skeleton_animation(&skeleton);

        update_animator(&player);
        update_animator(&skeleton);

        draw_entity(player);
        draw_entity(skeleton);

        update_player(&player);
        update_skeleton(&skeleton, &player);

        EndDrawing();
    }

    cleanup();

    CloseWindow();

    return 0;
}