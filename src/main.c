
#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#include <SDL.h>
#include <SDL_image.h>
#include <SDL_ttf.h>

#define GL_GLEXT_PROTOTYPES 1
#include <SDL_opengles2.h>

#else
#include <SDL.h>
#define GL_GLEXT_PROTOTYPES 1
#include <SDL_opengl.h>
#endif

#define CPU_INCLUDE 1
#include "../assets/shaders/common.h"

#include <stdbool.h>
#include <math.h>

#define MK_FILE_PATH_SIZE 2048

//#define MINIMP3_FLOAT_OUTPUT
#define MINIMP3_IMPLEMENTATION
#include "minimp3_ex.h"

typedef struct{
  GLuint program;
  GLuint vec4_array_uniform; 
  char file_path[MK_FILE_PATH_SIZE];
}shader_info_t; 

typedef struct{
    SDL_Renderer* rdr;
    SDL_Window* wnd;
    SDL_GLContext* glc;
    shader_info_t shaders[128];
    double start_time; 
    SDL_AudioDeviceID audio_dev;
    mp3dec_ex_t dec;
    float audio_mean[4]; 

    float audio_rms[4]; 
}state_t; 

typedef struct{
  float vec_array[MK_UNIFORM_ARRAY_SIZE*4];
}shader_state_t;
#if !defined(NDEBUG)
  #define GL_CK_ERROR() {int error; while((error = glGetError())){printf("GL Error: %d line: %d\n",error,__LINE__);}}
#else
  #define GL_CK_ERROR()
#endif 

#define MK_ARR_SIZE(A) (sizeof(A)/sizeof((A)[0]))
state_t state;
shader_state_t shader_state={0};
double mk_time(){
  static int64_t start_time =-1;
  if(start_time<0)start_time = SDL_GetTicks();
  return (SDL_GetTicks()-start_time)/1000.0;
}
char * mk_get_asset_path(const char * path){
  static char * base = NULL;
  if(base==NULL){
    base= SDL_GetBasePath();
  }
  int size = strlen(base)+strlen(path)+1;
  char *buffer=(char*)malloc(size+128);
  snprintf(buffer,size+128,"%s/../../%s",base,path);
  FILE *f = fopen(buffer,"rb");
  if(f){
    fclose(f);
    return buffer; 
  }
  snprintf(buffer,size,"%s/%s",base,path);
  return buffer;
}
char * mk_file_to_string(const char * filename){
  char* asset_path = mk_get_asset_path(filename);
  FILE *f = fopen(asset_path, "rb");
  if(!f){
    printf("Failed to open file:%s\n",asset_path);
    free(asset_path);
    return strdup(" ");
  }
  free(asset_path);
  fseek(f,0,SEEK_END);
  uint64_t size = ftell(f);
  char * data = (char*)malloc(size+1);
  fseek(f,0,SEEK_SET);
  fread(data,size,1,f);
  fclose(f);
  data[size]=0;
  return data; 
}

void mk_set_root_shader(const char* path){
  for(int i=0;i<MK_ARR_SIZE(state.shaders);++i){
    state.shaders[0].program=0;
  }
  if(path!=state.shaders[0].file_path){
    strncpy(state.shaders[0].file_path,path,MK_FILE_PATH_SIZE);
  } 
}
void mk_check_shader_log(GLuint shader){
  GLint log_len = 0;
  glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &log_len);
  char * compile_log = (char*)malloc(log_len);
	glGetShaderInfoLog(shader, log_len, &log_len, compile_log);
  if(log_len)printf("Compile Log: %s\n",compile_log);
  free(compile_log);
}
char * mk_strcat(const char* a, const char*b){
  size_t lena = strlen(a);
  size_t lenb = strlen(b);
  size_t size = lena+lenb;
  char* res = (char*)malloc(size+1);
  memcpy(res,a,lena);
  memcpy(res+lena,b,lenb+1);
  return res;
}
char * mk_process_includes(const char * shader){
  char * processed = (char*)malloc(1);
  processed[0]=0;
  while(shader[0]){
    char* first_occur = strstr(shader,"#include ");
    if(first_occur){
      if(first_occur ==shader){
      }else{
        char* new_str = mk_strcat(processed,first_occur-1);
        free(processed);
        processed=new_str;
      }
      char* first_quote = strstr(first_occur,"<")+1;
      if(!first_quote){
        printf("Malformed Include Statement\n");
        return processed; 
      }
      char* second_quote = strstr(first_quote+1,">");
      if(!second_quote){
        printf("Malformed Include Statement\n");
      }

      char* file_path = malloc(second_quote-first_quote+1);
      memcpy(file_path,first_quote,second_quote-first_quote);
      file_path[second_quote-first_quote]=0; 
      char* file_data = mk_file_to_string(file_path);
      char* append_data = mk_process_includes(file_data);
      free(file_data);
      char* new_str = mk_strcat(processed,append_data);
      free(append_data);
      free(file_path);

      free(processed);
      processed=new_str;
      shader = second_quote+1; 
    }else{
      char* new_str = mk_strcat(processed,shader);
      free(processed);
      processed=new_str;
      break;
    }
  }
  return processed; 

}
void mk_make_shader_program(int shad_id, const char* shader){
  char * inc_processed = mk_process_includes(shader);

  printf("Compiling Shader:\n%s\n",inc_processed);
  const char * header = "#version 120\n";
  const char* vert_source[] = {header,"#define VERTEX_SHADER 1\n",inc_processed};
  const char* frag_source[] = {header,"#define PIXEL_SHADER 1\n",inc_processed};

  // Create and compile the vertex shader
  GLuint vert = glCreateShader(GL_VERTEX_SHADER);
  glShaderSource(vert, 3, vert_source, NULL);
  glCompileShader(vert);
  mk_check_shader_log(vert);
 

  // Create and compile the fragment shader
  GLuint frag = glCreateShader(GL_FRAGMENT_SHADER);
  glShaderSource(frag, 3, frag_source, NULL);
  glCompileShader(frag);
  mk_check_shader_log(frag);

  GL_CK_ERROR();

  // Link the vertex and fragment shader into a shader program
  GLuint shaderProgram = glCreateProgram();
  glAttachShader(shaderProgram, vert);
  glAttachShader(shaderProgram, frag);
  glLinkProgram(shaderProgram);
  GL_CK_ERROR();
  glDeleteShader(vert);
  glDeleteShader(frag);

  free(inc_processed);

  // Specify the layout of the vertex data
  GLint vert_id_attrib = glGetAttribLocation(shaderProgram, "mk_vertex_id");
  GL_CK_ERROR();
  glEnableVertexAttribArray(vert_id_attrib);
  GL_CK_ERROR();
  glVertexAttribPointer(vert_id_attrib, 1, GL_INT, GL_FALSE, 0, 0);
  GL_CK_ERROR();
  state.shaders[shad_id].program = shaderProgram;
  state.shaders[shad_id].vec4_array_uniform = glGetUniformLocation(shaderProgram,"mk_uniform_array");
}
void mk_flush_shaders(){
  for(int i=0;i<MK_ARR_SIZE(state.shaders);++i){
    shader_info_t *sh = state.shaders+i;
    if(sh->program)glDeleteProgram(sh->program);
    sh->program=0; 
  }
}
void mk_frame(){
  static double last_reload = 0; 

  if(mk_time()-last_reload>1.0){
    mk_flush_shaders();
    last_reload = mk_time();
  }
  int max_queues=4;
  while(SDL_GetQueuedAudioSize(state.audio_dev)<4*1024&&--max_queues){
    int16_t buffer[1024];
    size_t read= mp3dec_ex_read(&state.dec, buffer, 1024);
    for(int s= 0; s<read;++s){
      int ch = s%state.dec.info.channels;
      if(ch<4){
        float value = buffer[s]/32768.;
        float env =0;
        float mean_gain =0.1;
        state.audio_mean[ch]=value*mean_gain+ state.audio_mean[ch]*(1.0-mean_gain);
        float delta = value-state.audio_mean[ch];
        delta*=delta;
        float rms_gain =0.02;
        if(delta<state.audio_rms[ch])rms_gain*=0.01;
        state.audio_rms[ch]=delta*rms_gain+state.audio_rms[ch]*(1.0-rms_gain);
      }
    }
    SDL_QueueAudio(state.audio_dev, buffer, read*2);  
    if(read==0)mp3dec_ex_seek(&state.dec,0);
  }
  int wx, wy;
  SDL_GetWindowSize(state.wnd, &wx,&wy);
  glViewport(0,0,wx,wy);
  shader_state.vec_array[0] = wx; 
  shader_state.vec_array[1] = wy; 
  shader_state.vec_array[2]=mk_time(); //Time
  shader_state.vec_array[3]+=1; //Frame

  for(int i=0;i<4;++i){
    shader_state.vec_array[4+i]=log(sqrt(state.audio_rms[i])+1);
  }

  // Clear the screen to black
  glClearColor(1.0f, 0.0f, 0.0f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT);
  shader_info_t *sh = state.shaders+0;
  if(!sh->program){
    char * shader = mk_file_to_string(sh->file_path);
    mk_make_shader_program(0,shader);
    free(shader);
  }
  if(sh->program){

    glUseProgram(sh->program);
    glUniform4fv(sh->vec4_array_uniform,MK_UNIFORM_ARRAY_SIZE,shader_state.vec_array);

    // Draw a triangle from the 3 vertices
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    GL_CK_ERROR();
  }

  SDL_GL_SwapWindow(state.wnd);
}
void mk_sdl_main_loop(){ 
  SDL_Event e;
  while(SDL_PollEvent(&e)){
      if(e.type == SDL_QUIT) exit(0);
      if(e.type == SDL_KEYDOWN){
        if(e.key.keysym.scancode==SDL_SCANCODE_ESCAPE)exit(0);
      }
  }
  mk_frame();
 }

int mk_sdl_event_watch(void* data, SDL_Event* ev){
  if(ev->type==SDL_WINDOWEVENT){
    mk_frame();
  }
  if(ev->type == SDL_QUIT) exit(0);
  return 0;
}
void mk_open_audio(const char* path){
  char* asset_path = mk_get_asset_path(path);
  if (mp3dec_ex_open(&state.dec, asset_path, MP3D_SEEK_TO_SAMPLE)){
    printf("Failed to open audio_file: %s\n",path);
  }
  free(asset_path);
  SDL_AudioSpec audio_sets = {};
  audio_sets.freq = state.dec.info.hz; // Our sampling rate
  audio_sets.format = AUDIO_S16; // Use 16-bit amplitude values
  audio_sets.channels = state.dec.info.channels; // Stereo samples
  audio_sets.samples = 1024; // Size, in samples, of audio buffer

  state.audio_dev = SDL_OpenAudioDevice(NULL, 0, &audio_sets, NULL, 0);
  if(state.audio_dev==0){
    printf("Failed to open audio dev:%s\n", SDL_GetError());
  }
  // Start music playing
  SDL_PauseAudioDevice(state.audio_dev, 0);
}

int main(int argc, char** argv){
  SDL_Init(SDL_INIT_EVERYTHING);
  printf("Initializing Maksa Engine:\nCommit: %s\nBranch: %s\n",GIT_COMMIT_HASH,GIT_BRANCH);
  state.wnd = SDL_CreateWindow("Maksa", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,640, 480, SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN|SDL_WINDOW_RESIZABLE);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
  SDL_GL_SetSwapInterval(1);
  SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
  SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
  state.glc = SDL_GL_CreateContext(state.wnd);

  state.rdr = SDL_CreateRenderer(state.wnd, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_TARGETTEXTURE);

  SDL_GL_MakeCurrent(state.wnd,state.glc);

  GL_CK_ERROR();

  // Create a Vertex Buffer Object and copy the vertex data to it
  GLuint vbo;
  glGenBuffers(1, &vbo);

  #define MAX_VERTS (16*1024*1024)
  GLuint *vert_id_buff = (GLuint*)malloc(sizeof(GLuint)*MAX_VERTS);
  for(int i=0;i<MAX_VERTS;++i)vert_id_buff[i]=i;

  glBindBuffer(GL_ARRAY_BUFFER, vbo);
  glBufferData(GL_ARRAY_BUFFER, sizeof(GLuint)*MAX_VERTS, vert_id_buff, GL_STATIC_DRAW);
  free(vert_id_buff);
  GL_CK_ERROR();

  if(argc>1)mk_set_root_shader(argv[1]);
  else mk_set_root_shader("assets/shaders/root_shader.shd");
  
  GL_CK_ERROR();
  SDL_AddEventWatch(mk_sdl_event_watch, state.wnd);

  mk_open_audio("assets/wait.mp3");

#ifdef __EMSCRIPTEN__
  emscripten_set_main_loop(mk_sdl_main_loop, 0, true);
#else
  while(true) mk_sdl_main_loop();
#endif

  return 0;
}