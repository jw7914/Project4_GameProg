/**
* Author: Jason Wu
* Assignment: Rise of the AI
* Date due: 2024-11-9, 11:59pm
* I pledge that I have completed this assignment without
* collaborating with anyone else, in conformance with the
* NYU School of Engineering Policies and Procedures on
* Academic Misconduct.
**/


#define GL_SILENCE_DEPRECATION
#define STB_IMAGE_IMPLEMENTATION
#define LOG(argument) std::cout << argument << '\n'
#define GL_GLEXT_PROTOTYPES 1
#define FIXED_TIMESTEP 0.0166666f
#define PLATFORM_COUNT 10
const int MAP_WIDTH = 16;
const int MAP_HEIGHT = 10;


#ifdef _WINDOWS
#include <GL/glew.h>
#endif

#include <SDL_mixer.h>
#include <SDL.h>
#include <SDL_opengl.h>
#include "glm/mat4x4.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "ShaderProgram.h"
#include "stb_image.h"
#include "cmath"
#include <ctime>
#include <cstdlib>
#include <vector>
#include "Entity.h"
#include "Map.h"

// ––––– STRUCTS AND ENUMS ––––– //
enum FilterType { NEAREST, LINEAR};

struct GameState
{
    Entity* player;
    Entity* cat;
    Entity* platforms;
    Entity* enemies;
    Entity* background;
    Entity* healthbar;
    
    Map* map;
    Mix_Music *bgm;
    Mix_Chunk *jump_sfx;
};

// ––––– CONSTANTS ––––– //
constexpr int WINDOW_WIDTH  = 640 * 2,
          WINDOW_HEIGHT = 800;

constexpr int FONTBANK_SIZE = 16;


constexpr float BG_RED     = 0.1922f,
            BG_BLUE    = 0.549f,
            BG_GREEN   = 0.9059f,
            BG_OPACITY = 1.0f;
constexpr int VIEWPORT_X = 0,
          VIEWPORT_Y = 0,
          VIEWPORT_WIDTH  = WINDOW_WIDTH,
          VIEWPORT_HEIGHT = WINDOW_HEIGHT;

constexpr char V_SHADER_PATH[] = "shaders/vertex_textured.glsl",
           F_SHADER_PATH[] = "shaders/fragment_textured.glsl";

constexpr float MILLISECONDS_IN_SECOND = 1000.0;
constexpr char BACKGROUND_IMG_FILEPATH[]    = "/Users/jasonwu/Desktop/Coding/CompSciClasses/Game_Programming/Project4_GameProg/Project4/assets/windrise-background-4k.png";
constexpr char MAP_TILESET_FILEPATH[] = "/Users/jasonwu/Desktop/Coding/CompSciClasses/Game_Programming/Project4_GameProg/Project4/assets/new_tilemap.png";

constexpr char ENEMY_TILESET_FILEPATH[] = "/Users/jasonwu/Desktop/Coding/CompSciClasses/Game_Programming/Project4_GameProg/Project4/assets/tilemap-characters_packed.png";

constexpr char FONTSHEET_FILEPATH[]   = "/Users/jasonwu/Desktop/Coding/CompSciClasses/Game_Programming/Project4_GameProg/Project4/assets/font1.png";


constexpr int NUMBER_OF_TEXTURES = 1;
constexpr GLint LEVEL_OF_DETAIL  = 0;
constexpr GLint TEXTURE_BORDER   = 0;
GLuint g_font_texture_id;

constexpr int CD_QUAL_FREQ    = 44100,
          AUDIO_CHAN_AMT  = 2,     // stereo
          AUDIO_BUFF_SIZE = 4096;

constexpr char BGM_FILEPATH[] = "/Users/jasonwu/Desktop/Coding/CompSciClasses/Game_Programming/Project4_GameProg/Project4/assets/galactic.mp3",
           SFX_FILEPATH[] = "/Users/jasonwu/Desktop/Coding/CompSciClasses/Game_Programming/Project4_GameProg/Project4/assets/retro-jump-3-236683.wav";

constexpr int PLAY_ONCE = 0,    // play once, loop never
          NEXT_CHNL = -1,   // next available channel
          ALL_SFX_CHNL = -1;

constexpr int NUM_ENEMY = 4;


// ––––– GLOBAL VARIABLES ––––– //
GameState g_game_state;

SDL_Window* g_display_window;
bool g_game_is_running = true;

ShaderProgram g_program;
glm::mat4 g_view_matrix, g_projection_matrix;

float g_previous_ticks = 0.0f;
float g_accumulator = 0.0f;

int healthState = 0;
int NUM_ACTIVE_ENEMY;
bool isRunning = true;

unsigned int MAP_DATA[] =
{
        10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10,  // Row 0
        10, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 10,  // Row 1
        10, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 10,  // Row 2
        10, 0, 0, 0, 0, 0, 156, 156, 156, 156, 0, 0, 0, 0, 0, 10,  // Row 3
        10, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 10,  // Row 4
        10, 0, 6, 6, 6, 6, 0, 0, 0, 0, 16, 16, 16, 16, 0, 10,  // Row 5
        10, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 10,  // Row 6
        10, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 10,  // Row 7
        10, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 10,  // Row 8
        10, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20  // Row 9
};


// ––––– GENERAL FUNCTIONS ––––– //
GLuint load_texture(const char* filepath, FilterType filterType)
{
   int width, height, number_of_components;
   unsigned char* image = stbi_load(filepath, &width, &height, &number_of_components, STBI_rgb_alpha);

   if (image == NULL)
   {
       LOG("Unable to load image. Make sure the path is correct.");
       assert(false);
   }

   GLuint textureID;
   glGenTextures(NUMBER_OF_TEXTURES, &textureID);
   glBindTexture(GL_TEXTURE_2D, textureID);
   glTexImage2D(GL_TEXTURE_2D, LEVEL_OF_DETAIL, GL_RGBA, width, height, TEXTURE_BORDER,
                GL_RGBA, GL_UNSIGNED_BYTE, image);

   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                   filterType == NEAREST ? GL_NEAREST : GL_LINEAR);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER,
                   filterType == NEAREST ? GL_NEAREST : GL_LINEAR);

   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

   stbi_image_free(image);

   return textureID;
}

void draw_text(ShaderProgram *shader_program, GLuint font_texture_id, std::string text,
               float font_size, float spacing, glm::vec3 position)
{
    // Scale the size of the fontbank in the UV-plane
    // We will use this for spacing and positioning
    float width = 1.0f / FONTBANK_SIZE;
    float height = 1.0f / FONTBANK_SIZE;

    // Instead of having a single pair of arrays, we'll have a series of pairs—one for
    // each character. Don't forget to include <vector>!
    std::vector<float> vertices;
    std::vector<float> texture_coordinates;

    // For every character...
    for (int i = 0; i < text.size(); i++) {
        // 1. Get their index in the spritesheet, as well as their offset (i.e. their
        //    position relative to the whole sentence)
        int spritesheet_index = (int) text[i];  // ascii value of character
        float offset = (font_size + spacing) * i;

        // 2. Using the spritesheet index, we can calculate our U- and V-coordinates
        float u_coordinate = (float) (spritesheet_index % FONTBANK_SIZE) / FONTBANK_SIZE;
        float v_coordinate = (float) (spritesheet_index / FONTBANK_SIZE) / FONTBANK_SIZE;

        // 3. Inset the current pair in both vectors
        vertices.insert(vertices.end(), {
            offset + (-0.5f * font_size), 0.5f * font_size,
            offset + (-0.5f * font_size), -0.5f * font_size,
            offset + (0.5f * font_size), 0.5f * font_size,
            offset + (0.5f * font_size), -0.5f * font_size,
            offset + (0.5f * font_size), 0.5f * font_size,
            offset + (-0.5f * font_size), -0.5f * font_size,
        });

        texture_coordinates.insert(texture_coordinates.end(), {
            u_coordinate, v_coordinate,
            u_coordinate, v_coordinate + height,
            u_coordinate + width, v_coordinate,
            u_coordinate + width, v_coordinate + height,
            u_coordinate + width, v_coordinate,
            u_coordinate, v_coordinate + height,
        });
    }

    // 4. And render all of them using the pairs
    glm::mat4 model_matrix = glm::mat4(1.0f);
    model_matrix = glm::translate(model_matrix, position);

    shader_program->set_model_matrix(model_matrix);
    glUseProgram(shader_program->get_program_id());

    glVertexAttribPointer(shader_program->get_position_attribute(), 2, GL_FLOAT, false, 0,
                          vertices.data());
    glEnableVertexAttribArray(shader_program->get_position_attribute());

    glVertexAttribPointer(shader_program->get_tex_coordinate_attribute(), 2, GL_FLOAT,
                          false, 0, texture_coordinates.data());
    glEnableVertexAttribArray(shader_program->get_tex_coordinate_attribute());

    glBindTexture(GL_TEXTURE_2D, font_texture_id);
    glDrawArrays(GL_TRIANGLES, 0, (int) (text.size() * 6));

    glDisableVertexAttribArray(shader_program->get_position_attribute());
    glDisableVertexAttribArray(shader_program->get_tex_coordinate_attribute());
}


void initialise()
{
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);
    g_display_window = SDL_CreateWindow("Project 4",
                                      SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                      WINDOW_WIDTH, WINDOW_HEIGHT,
                                      SDL_WINDOW_OPENGL);

    SDL_GLContext context = SDL_GL_CreateContext(g_display_window);
    SDL_GL_MakeCurrent(g_display_window, context);

#ifdef _WINDOWS
    glewInit();
#endif

    // ––––– VIDEO ––––– //
    glViewport(VIEWPORT_X, VIEWPORT_Y, VIEWPORT_WIDTH, VIEWPORT_HEIGHT);

    g_program.load(V_SHADER_PATH, F_SHADER_PATH);

    g_view_matrix = glm::mat4(1.0f);
    g_projection_matrix = glm::ortho(-5.0f, 5.0f, -3.75f, 3.75f, -1.0f, 1.0f);

    g_program.set_projection_matrix(g_projection_matrix);
    g_program.set_view_matrix(g_view_matrix);

    glUseProgram(g_program.get_program_id());

    glClearColor(BG_RED, BG_BLUE, BG_GREEN, BG_OPACITY);

    // ––––– BGM ––––– //
    Mix_OpenAudio(CD_QUAL_FREQ, MIX_DEFAULT_FORMAT, AUDIO_CHAN_AMT, AUDIO_BUFF_SIZE);
    g_game_state.bgm = Mix_LoadMUS(BGM_FILEPATH);
    // STEP 2: Play music
    g_game_state.bgm = Mix_LoadMUS(BGM_FILEPATH);
    Mix_PlayMusic(g_game_state.bgm, -1);
    Mix_VolumeMusic(MIX_MAX_VOLUME / 2.0);

    // ––––– SFX ––––– //
    g_game_state.jump_sfx = Mix_LoadWAV(SFX_FILEPATH);

    // ––––– Background Image ––––– //
    GLuint background_texture_id = load_texture(BACKGROUND_IMG_FILEPATH, LINEAR);
    g_game_state.background = new Entity();
    g_game_state.background->set_texture_id(background_texture_id);
    g_game_state.background->set_scale(glm::vec3(10.0f, 8.0f, 0.0f));
    g_game_state.background->update(0.0f, NULL, NULL, 0, NULL);

    // ––––– MAP ––––– //
    GLuint map_texture_id = load_texture(MAP_TILESET_FILEPATH, NEAREST);
    g_game_state.map = new Map(MAP_WIDTH, MAP_HEIGHT, MAP_DATA, map_texture_id, 1.0f, 20, 9);
    
    // ––––– HEALTH BAR ––––– //
    g_game_state.healthbar = new Entity[5];
    std::vector<GLuint> health_texture_ids = {
        load_texture("/Users/jasonwu/Desktop/Coding/CompSciClasses/Game_Programming/Project4_GameProg/Project4/assets/health_full.png", NEAREST),
        load_texture("/Users/jasonwu/Desktop/Coding/CompSciClasses/Game_Programming/Project4_GameProg/Project4/assets/health_medium.png", NEAREST),
        load_texture("/Users/jasonwu/Desktop/Coding/CompSciClasses/Game_Programming/Project4_GameProg/Project4/assets/health_low.png", NEAREST),
        load_texture("/Users/jasonwu/Desktop/Coding/CompSciClasses/Game_Programming/Project4_GameProg/Project4/assets/health_verylow.png", NEAREST),
        load_texture("/Users/jasonwu/Desktop/Coding/CompSciClasses/Game_Programming/Project4_GameProg/Project4/assets/health_none.png", NEAREST)
    };
    for (int i = 0; i < 5; i++) {
        g_game_state.healthbar[i] = Entity();
        g_game_state.healthbar[i].set_texture_id(health_texture_ids[i]);
        g_game_state.healthbar[i].set_position(glm::vec3(4.5f, 3.5f, 0.0f));
        g_game_state.healthbar[i].set_scale(glm::vec3(0.5f, 0.25f, 0.0f));
        g_game_state.healthbar[i].update(0.0f, NULL, NULL, 0, NULL);
    }
    
    // ––––– PLAYER (CAT) ––––– //
    glm::vec3 acceleration = glm::vec3(0.0f,-4.905f, 0.0f);

    std::vector<GLuint> cat_texture_ids = {
        load_texture("/Users/jasonwu/Desktop/Coding/CompSciClasses/Game_Programming/Project4_GameProg/Project4/assets/Meow-Knight_Idle.png", NEAREST),   // IDLE spritesheet
        load_texture("/Users/jasonwu/Desktop/Coding/CompSciClasses/Game_Programming/Project4_GameProg/Project4/assets/Meow-Knight_Attack_3.png", NEAREST),  // ATTACK spritesheet
        load_texture("/Users/jasonwu/Desktop/Coding/CompSciClasses/Game_Programming/Project4_GameProg/Project4/assets/Meow-Knight_Death.png", NEAREST), // DEATH spritesheet
        load_texture("/Users/jasonwu/Desktop/Coding/CompSciClasses/Game_Programming/Project4_GameProg/Project4/assets/Meow-Knight_Dodge.png", NEAREST), // RUN spritesheet
        load_texture("/Users/jasonwu/Desktop/Coding/CompSciClasses/Game_Programming/Project4_GameProg/Project4/assets/Meow-Knight_Take_Damage.png", NEAREST) // DAMAGE spritesheet
    };



    
    std::vector<std::vector<int>> cat_animations = {
           {0, 1, 2, 3, 4, 5},       // IDLE animation frames
           {0, 1, 2, 3},  // ATTACK animation frames
           {0, 1, 2, 3, 4, 5},       // DEATH animation frames
           {2, 2, 3, 3, 5, 5, 6, 6}, //RUN animation frames
           {0, 1, 2} //DAMAGE animation frames
       };
//    Entity(std::vector<GLuint> texture_ids, float speed, glm::vec3 acceleration, float jump_power, std::vector<std::vector<int>> animations, float animation_time, int animation_frames, int animation_index, int animation_cols, int animation_rows, float width, float height, EntityType EntityType, Animation animation);
    
    g_game_state.player =  new Entity(
                                    cat_texture_ids,
                                    5.0f,
                                    acceleration,
                                    3.0f,
                                    cat_animations,
                                    0.0f,
                                    3,
                                    0,
                                    1,
                                    3,
                                    0.75f,
                                    1.0f,
                                    PLAYER,
                                    DEFAULT
                                );
    g_game_state.player->set_position(glm::vec3(7.0f, -8.0f, 0.0f));
    g_game_state.player->update(FIXED_TIMESTEP, g_game_state.player, NULL, 0,
                                g_game_state.map);


    // Jumping
    g_game_state.player->set_jumping_power(6.0f);
    
    
    // ––––– Enemies ––––– //
    g_game_state.enemies = new Entity[NUM_ENEMY];
    std::vector<GLuint> enemy_texture_ids = {
        load_texture("/Users/jasonwu/Desktop/Coding/CompSciClasses/Game_Programming/Project4_GameProg/Project4/assets/enemy1.png", NEAREST),
        load_texture("/Users/jasonwu/Desktop/Coding/CompSciClasses/Game_Programming/Project4_GameProg/Project4/assets/enemy2.png", NEAREST),
        load_texture("/Users/jasonwu/Desktop/Coding/CompSciClasses/Game_Programming/Project4_GameProg/Project4/assets/enemy3.png", NEAREST)
    };
    
    
    g_game_state.enemies[0] =  Entity(enemy_texture_ids[0], 0.0f, 1.0f, 1.0f, ENEMY, PATROL);
    g_game_state.enemies[1] =  Entity(enemy_texture_ids[1], 0.0f, 1.0f, 1.0f, ENEMY, JUMPING);
    g_game_state.enemies[2] =  Entity(enemy_texture_ids[2], 0.0f, 1.0f, 1.0f, ENEMY, WALKING);
    g_game_state.enemies[3] =  Entity(enemy_texture_ids[2], 0.0f, 1.0f, 1.0f, ENEMY, WALKING);
    for (int i = 0; i < NUM_ENEMY; i++)
    {
        g_game_state.enemies[i].set_scale(glm::vec3(1.0f,1.0f,0.0f));
        g_game_state.enemies[i].set_movement(glm::vec3(0.0f));
        g_game_state.enemies[i].set_acceleration(glm::vec3(0.0f, -9.81f, 0.0f));
        g_game_state.enemies[i].activate();
        g_game_state.enemies[i].set_entity_type(ENEMY);
        g_game_state.enemies[i].set_speed(1.0f);
    }
    
    g_game_state.enemies[3].set_speed(0.0f);

    
    g_game_state.enemies[0].set_ai_type(PATROL);
    g_game_state.enemies[1].set_ai_type(JUMPING);
    g_game_state.enemies[2].set_ai_type(WALKING);
    g_game_state.enemies[3].set_ai_type(WALKING);

    g_game_state.enemies[0].set_position(glm::vec3(2.0f, -4.0f, 0.0f));
    g_game_state.enemies[1].set_position(glm::vec3(11.0f, -4.0f, 0.0f));
    g_game_state.enemies[2].set_position(glm::vec3(7.0f, -2.0f, 0.0f));
    g_game_state.enemies[3].set_position(glm::vec3(7.0f, -2.0f, 0.0f));

    
   

    // ––––– GENERAL ––––– //
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
}

void process_input()
{
    g_game_state.player->set_movement(glm::vec3(0.0f));

    SDL_Event event;
    if (isRunning){
        g_game_state.player->set_animation_state(DEFAULT);
    }
    bool attacking;
    
    while (SDL_PollEvent(&event))
    {
        switch (event.type) {
            // End game
            case SDL_QUIT:
            case SDL_WINDOWEVENT_CLOSE:
                g_game_is_running = false;
                break;

            case SDL_KEYDOWN:
                switch (event.key.keysym.sym) {
                    case SDLK_q:
                        // Quit the game with a keystroke
                        g_game_is_running = false;
                        break;
                    
                    
                    case SDLK_SPACE:
                        // Jump
                        if (g_game_state.player->get_collided_bottom())
                        {
                            if(isRunning){
                                g_game_state.player->jump();
                                Mix_PlayChannel(NEXT_CHNL, g_game_state.jump_sfx, 0);
                            }
                        }
                        break;

                    case SDLK_h:
                        // Stop music
                        Mix_HaltMusic();
                        break;

                    case SDLK_p:
                        Mix_PlayMusic(g_game_state.bgm, -1);
                        break;
                    
                    default:
                        break;
                }

            default:
                break;
        }
    }

    const Uint8 *key_state = SDL_GetKeyboardState(NULL);

    if (isRunning){
        if (key_state[SDL_SCANCODE_A]) {
            g_game_state.player->set_animation_state(ATTACK);
            attacking = true;
        }
        else {
            attacking = false;
        }
     
        if (!attacking){
            if (key_state[SDL_SCANCODE_LEFT])
            {
                g_game_state.player->move_left();
            }
            else if (key_state[SDL_SCANCODE_RIGHT])
            {
                g_game_state.player->move_right();
            }
        }
    }
    
    


    if (glm::length(g_game_state.player->get_movement()) > 1.0f)
    {
        g_game_state.player->normalise_movement();
    }
}

void update()
{
    float ticks = (float)SDL_GetTicks() / MILLISECONDS_IN_SECOND;
    float delta_time = ticks - g_previous_ticks;
    g_previous_ticks = ticks;

    delta_time += g_accumulator;

    if (delta_time < FIXED_TIMESTEP)
    {
        g_accumulator = delta_time;
        return;
    }

    while (delta_time >= FIXED_TIMESTEP)
    {
        int active = 0;
        int playerCollsion = g_game_state.player->update(FIXED_TIMESTEP, g_game_state.player, g_game_state.enemies, NUM_ENEMY - 1, g_game_state.map);
        
        for (int i = 0; i < NUM_ENEMY; i++){
            g_game_state.enemies[i].update(FIXED_TIMESTEP,
                                           g_game_state.player,
                                           NULL, 0,
                                           g_game_state.map);
            
//            LOG("ENEMEY " << i << ": " <<g_game_state.enemies[i].get_entity_type());
            if (g_game_state.enemies[i].isActive()) {
                active++;
            }
        }
        if (playerCollsion == 1) {
            if (healthState < 4) {
                healthState++;
            }
            else {
                g_game_state.player->set_animation_state(DEATH);
                isRunning = false;
            }
        }
        NUM_ACTIVE_ENEMY = active;
        if (NUM_ACTIVE_ENEMY == 1) {
            isRunning = false;
        }

        delta_time -= FIXED_TIMESTEP;
    }
    
    g_accumulator = delta_time;
    g_view_matrix = glm::mat4(1.0f);
    glm::vec3 player_pos = g_game_state.player->get_position();
    
    // Camera Follows the player
    if (g_game_state.player->get_curr_animation() == ATTACK) {
        g_game_state.background->set_position(glm::vec3(player_pos.x - 0.5f, player_pos.y + 2.25f, 0.0f));
        for (int i = 0; i < 5; i++){
            g_game_state.healthbar[i].set_position(glm::vec3(player_pos.x + 4.0f, player_pos.y + 5.5f, 0.0f));
            g_game_state.healthbar[i].update(0.0f, NULL, NULL, 0, NULL);
        }
        g_view_matrix = glm::translate(g_view_matrix, glm::vec3(-player_pos.x + 0.5f, -player_pos.y - 2.25f, 0.0f));
    }
    else {
        g_game_state.background->set_position(glm::vec3(player_pos.x, player_pos.y + 2.25f, 0.0f));
        for (int i = 0; i < 5; i++){
            g_game_state.healthbar[i].set_position(glm::vec3(player_pos.x + 4.5f, player_pos.y + 5.5f, 0.0f));
            g_game_state.healthbar[i].update(0.0f, NULL, NULL, 0, NULL);
        }
        g_view_matrix = glm::translate(g_view_matrix, glm::vec3(-player_pos.x, -player_pos.y - 2.25f, 0.0f));
    }
    
    g_game_state.background->update(0.0f, NULL, NULL, 0, NULL);
}

void render()
{
    g_program.set_view_matrix(g_view_matrix);
    glClear(GL_COLOR_BUFFER_BIT);
    g_game_state.background->render(&g_program);
    g_game_state.map->render(&g_program);
    g_game_state.healthbar[healthState].render(&g_program);
    for (int i = 0; i < NUM_ENEMY - 1; i++) {
        if (g_game_state.enemies[i].isActive()){
            g_game_state.enemies[i].render(&g_program);
        }
    }
    
    g_font_texture_id = load_texture(FONTSHEET_FILEPATH, NEAREST);
     
    if (NUM_ACTIVE_ENEMY == 1){
        draw_text(&g_program, g_font_texture_id, "PLAYER WIN", 0.5f, 0.05f,
              glm::vec3(g_game_state.player->get_position().x - 3.0f, g_game_state.player->get_position().y + 3.0f, 0.0f));
    }
    
    if (healthState == 4) {
        draw_text(&g_program, g_font_texture_id, "PLAYER LOSE", 0.5f, 0.05f,
              glm::vec3(g_game_state.player->get_position().x - 3.0f, g_game_state.player->get_position().y + 3.0f, 0.0f));
    }
    g_game_state.player->render(&g_program);
    SDL_GL_SwapWindow(g_display_window);
}

void shutdown()
{
    SDL_Quit();

    delete [] g_game_state.platforms;
    delete[] g_game_state.healthbar;
    delete g_game_state.player;
    delete    g_game_state.map;
    Mix_FreeChunk(g_game_state.jump_sfx);
    Mix_FreeMusic(g_game_state.bgm);
}

// ––––– GAME LOOP ––––– //
int main(int argc, char* argv[])
{
    initialise();

    while (g_game_is_running)
    {
        process_input();
        update();
        render();
    }

    shutdown();
    return 0;
}
