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
const int MAP_WIDTH = 10;
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
    Entity* platforms;
    Entity* background;
    Entity* healthbar;
    
    Map* map;
    Mix_Music *bgm;
    Mix_Chunk *jump_sfx;
};

// ––––– CONSTANTS ––––– //
constexpr int WINDOW_WIDTH  = 640 * 2,
          WINDOW_HEIGHT = 800;

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
constexpr char SPRITESHEET_FILEPATH[] = "assets/george_0.png";
constexpr char PLATFORM_FILEPATH[]    = "assets/platformPack_tile027.png";
constexpr char BACKGROUND_IMG_FILEPATH[]    = "/Users/jasonwu/Desktop/Coding/CompSciClasses/Game_Programming/Project4_GameProg/Project4/assets/windrise-background-4k.png";
constexpr char MAP_TILESET_FILEPATH[] = "/Users/jasonwu/Desktop/Coding/CompSciClasses/Game_Programming/Project4_GameProg/Project4/assets/new_tilemap.png";

constexpr int NUMBER_OF_TEXTURES = 1;
constexpr GLint LEVEL_OF_DETAIL  = 0;
constexpr GLint TEXTURE_BORDER   = 0;

constexpr int CD_QUAL_FREQ    = 44100,
          AUDIO_CHAN_AMT  = 2,     // stereo
          AUDIO_BUFF_SIZE = 4096;

constexpr char BGM_FILEPATH[] = "/Users/jasonwu/Desktop/Coding/CompSciClasses/Game_Programming/Project4_GameProg/Project4/assets/galactic.mp3",
           SFX_FILEPATH[] = "/Users/jasonwu/Desktop/Coding/CompSciClasses/Game_Programming/Project4_GameProg/Project4/assets/retro-jump-3-236683.wav";

constexpr int PLAY_ONCE = 0,    // play once, loop never
          NEXT_CHNL = -1,   // next available channel
          ALL_SFX_CHNL = -1;

// ––––– GLOBAL VARIABLES ––––– //
GameState g_game_state;

SDL_Window* g_display_window;
bool g_game_is_running = true;

ShaderProgram g_program;
glm::mat4 g_view_matrix, g_projection_matrix;

float g_previous_ticks = 0.0f;
float g_accumulator = 0.0f;

int healthState = 0;

unsigned int MAP_DATA[] =
{
        1, 1, 1, 1, 1, 1, 1, 1, 1, 3,  // Row 0
        1, 0, 0, 0, 0, 0, 0, 0, 0, 1,  // Row 1
        1, 0, 1, 1, 1, 1, 1, 0, 0, 1,  // Row 2
        1, 0, 1, 0, 0, 0, 1, 0, 0, 1,  // Row 3
        1, 0, 1, 0, 1, 1, 1, 0, 0, 1,  // Row 4
        1, 0, 0, 0, 0, 0, 0, 0, 0, 1,  // Row 5
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1,  // Row 6
        0, 0, 0, 0, 0, 0, 0, 0, 0, 1,  // Row 7
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1,  // Row 8
        4, 1, 1, 1, 1, 1, 1, 1, 1, 2   // Row 9
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
    g_game_state.background->set_scale(glm::vec3(1.0f, 1.0f, 0.0f));
    g_game_state.background->update(0.0f, NULL, NULL, 0, NULL);

    // ––––– MAP ––––– //
    GLuint map_texture_id = load_texture(MAP_TILESET_FILEPATH, NEAREST);
    g_game_state.map = new Map(MAP_WIDTH, MAP_HEIGHT, MAP_DATA, map_texture_id, 1.0f, 20, 9);
    GLuint platform_texture_id = load_texture(PLATFORM_FILEPATH, LINEAR);

    g_game_state.platforms = new Entity[PLATFORM_COUNT];

    // Set the type of every platform entity to PLATFORM
    for (int i = 0; i < PLATFORM_COUNT; i++)
    {
        g_game_state.platforms[i].set_texture_id(platform_texture_id);
        g_game_state.platforms[i].set_position(glm::vec3(i - PLATFORM_COUNT / 2.0f, -3.0f, 0.0f));
        g_game_state.platforms[i].set_width(0.8f);
        g_game_state.platforms[i].set_height(1.0f);
        g_game_state.platforms[i].set_entity_type(PLATFORM);
        g_game_state.platforms[i].update(0.0f, NULL, NULL, 0, NULL);
    }
    
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
    

    // ––––– PLAYER (GEORGE) ––––– //
    GLuint player_texture_id = load_texture(SPRITESHEET_FILEPATH, NEAREST);

    int player_walking_animation[4][4] =
    {
        { 1, 5, 9, 13 },  // for George to move to the left,
        { 3, 7, 11, 15 }, // for George to move to the right,
        { 2, 6, 10, 14 }, // for George to move upwards,
        { 0, 4, 8, 12 }   // for George to move downwards
    };

    glm::vec3 acceleration = glm::vec3(0.0f,-4.905f, 0.0f);

    g_game_state.player = new Entity(
        player_texture_id,         // texture id
        5.0f,                      // speed
        acceleration,              // acceleration
        3.0f,                      // jumping power
        player_walking_animation,  // animation index sets
        0.0f,                      // animation time
        4,                         // animation frame amount
        0,                         // current animation index
        4,                         // animation column amount
        4,                         // animation row amount
        1.0f,                      // width
        1.0f,                       // height
        PLAYER
    );
    
    std::vector<GLuint> cat_texture_ids = {
        load_texture("/Users/jasonwu/Desktop/Coding/CompSciClasses/Game_Programming/Project4_GameProg/Project4/assets/Meow-Knight_Attack_2.png", NEAREST),   // IDLE spritesheet
        load_texture("/Users/jasonwu/Desktop/Coding/CompSciClasses/Game_Programming/Project4_GameProg/Project4/assets/Meow-Knight_Idle.png", NEAREST),  // ATTACK spritesheet
        load_texture("/Users/jasonwu/Desktop/Coding/CompSciClasses/Game_Programming/Project4_GameProg/Project4/assets/Meow-Knight_Death.png", NEAREST), // DEATH spritesheet
        load_texture("/Users/jasonwu/Desktop/Coding/CompSciClasses/Game_Programming/Project4_GameProg/Project4/assets/Meow-Knight_Run.png", NEAREST), // RUN spritesheet
        load_texture("/Users/jasonwu/Desktop/Coding/CompSciClasses/Game_Programming/Project4_GameProg/Project4/assets/Meow-Knight_Take_Damage.png", NEAREST) // DAMAGE spritesheet
    };
    
    


    // Jumping
    g_game_state.player->set_jumping_power(5.0f);

    // ––––– GENERAL ––––– //
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
}

void process_input()
{
    g_game_state.player->set_movement(glm::vec3(0.0f));

    SDL_Event event;
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
                            g_game_state.player->jump();
                            Mix_PlayChannel(NEXT_CHNL, g_game_state.jump_sfx, 0);
                        }
                        break;

                    case SDLK_h:
                        // Stop music
                        Mix_HaltMusic();
                        break;

                    case SDLK_p:
                        Mix_PlayMusic(g_game_state.bgm, -1);

                    default:
                        break;
                }

            default:
                break;
        }
    }

    const Uint8 *key_state = SDL_GetKeyboardState(NULL);

    if (key_state[SDL_SCANCODE_LEFT])
    {
        g_game_state.player->move_left();
    }
    else if (key_state[SDL_SCANCODE_RIGHT])
    {
        g_game_state.player->move_right();
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
        g_game_state.player->update(FIXED_TIMESTEP, g_game_state.player, NULL, 0,
                                    g_game_state.map);
        delta_time -= FIXED_TIMESTEP;
    }
    
    g_accumulator = delta_time;
}

void render()
{
    glClear(GL_COLOR_BUFFER_BIT);
    g_game_state.background->render(&g_program);

    g_game_state.healthbar[healthState].render(&g_program);
    g_game_state.map->render(&g_program);
//    for (int i = 0; i < PLATFORM_COUNT; i++) g_game_state.platforms[i].render(&g_program);
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
