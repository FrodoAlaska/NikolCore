#include "nikol_core.h"


//////////////////////////////////////////////////////////////////////////

#ifdef NIKOL_GFX_CONTEXT_OPENGL  // OpenGL check

#include <glad/glad.h>

#include <cstring>

namespace nikol { // Start of nikol

/// ---------------------------------------------------------------------
/// *** Graphics ***

///---------------------------------------------------------------------------------------------------------------------
/// GfxContext
struct GfxContext {
  GfxContextFlags flags;

  sizei buffer_bits = 0;
};
/// GfxContext
///---------------------------------------------------------------------------------------------------------------------

///---------------------------------------------------------------------------------------------------------------------
/// GfxShader
struct GfxShader {
  u32 id, vert_id, frag_id;

  i8* vert_src; 
  i8* frag_src;
};
/// GfxShader
///---------------------------------------------------------------------------------------------------------------------

///---------------------------------------------------------------------------------------------------------------------
/// GfxTexture
struct GfxTexture {
  u32 id;

  i32 width, height;
  i32 channels;
  void* pixels; 
};
/// GfxTexture
///---------------------------------------------------------------------------------------------------------------------

///---------------------------------------------------------------------------------------------------------------------
/// GfxDrawCall
struct GfxDrawCall {
  u32 vao, vbo, ebo; 

  GfxBufferDesc* vertex_buffer       = nullptr; 
  GfxBufferDesc* index_buffer        = nullptr; 
  GfxShader* shader                  = nullptr; 
  GfxTexture* textures[TEXTURES_MAX] = {nullptr};

  sizei texture_count = 0;
};
/// GfxDrawCall
///---------------------------------------------------------------------------------------------------------------------

///---------------------------------------------------------------------------------------------------------------------
/// Callbacks 

static bool framebuffer_resize(const Event& event, const void* disp, const void* list) {
  if(event.type != EVENT_WINDOW_FRAMEBUFFER_RESIZED) {
    return false;
  }

  glViewport(0, 0, event.window_framebuffer_width, event.window_framebuffer_height);

  return true;
}

/// Callbacks 
///---------------------------------------------------------------------------------------------------------------------

///---------------------------------------------------------------------------------------------------------------------
/// Private functions 

static void gl_check_error(const i8* func_name) {
  GLenum err = glGetError(); 

  switch(err) {
    case GL_INVALID_ENUM: 
      NIKOL_LOG_ERROR("GL_ERROR: FUNC = %s, ERR = GL_INVALID_ENUM", func_name);
      break;
    case GL_INVALID_VALUE: 
      NIKOL_LOG_ERROR("GL_ERROR: FUNC = %s, ERR = GL_INVALID_VALUE", func_name);
      break;
    case GL_INVALID_OPERATION: 
      NIKOL_LOG_ERROR("GL_ERROR: FUNC = %s, ERR = GL_INVALID_OPERATION", func_name);
      break;
    case GL_STACK_OVERFLOW: 
      NIKOL_LOG_ERROR("GL_ERROR: FUNC = %s, ERR = GL_STACK_OVERFLOW", func_name);
      break;
    case GL_STACK_UNDERFLOW: 
      NIKOL_LOG_ERROR("GL_ERROR: FUNC = %s, ERR = GL_STACK_UNDERFLOW", func_name);
      break;
    case GL_OUT_OF_MEMORY: 
      NIKOL_LOG_ERROR("GL_ERROR: FUNC = %s, ERR = GL_OUT_OF_MEMORY", func_name);
      break;
    case GL_INVALID_FRAMEBUFFER_OPERATION: 
      NIKOL_LOG_ERROR("GL_ERROR: FUNC = %s, ERR = GL_INVALID_FRAMEBUFFER_OPERATION", func_name);
      break;
    case GL_CONTEXT_LOST: 
      NIKOL_LOG_ERROR("GL_ERROR: FUNC = %s, ERR = GL_CONTEXT_LOST", func_name);
      break;
    case GL_NO_ERROR:
      break;
  }
}

static void set_gfx_flags(GfxContext* gfx) {
  if((gfx->flags & GFX_FLAGS_DEPTH) == GFX_FLAGS_DEPTH) {
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);

    gfx->buffer_bits |= GL_DEPTH_BUFFER_BIT; 
  }
  
  if((gfx->flags & GFX_FLAGS_STENCIL) == GFX_FLAGS_STENCIL) {
    glEnable(GL_STENCIL_TEST);
    gfx->buffer_bits |= GL_STENCIL_BUFFER_BIT; 
  }
  
  if((gfx->flags & GFX_FLAGS_BLEND) == GFX_FLAGS_BLEND) {
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  }
  
  if((gfx->flags & GFX_FLAGS_MSAA) == GFX_FLAGS_MSAA) {
    glEnable(GL_MULTISAMPLE);
  }
}

static GLenum get_buffer_type(const GfxBufferType type) {
  switch(type) {
    case GFX_BUFFER_INDEX:
      return GL_ELEMENT_ARRAY_BUFFER;
    case GFX_BUFFER_VERTEX:
      return GL_ARRAY_BUFFER; 
    default:
      return 0;
  }
}

static GLenum get_buffer_mode(const GfxBufferMode mode) {
  switch(mode) {
    case GFX_BUFFER_MODE_DYNAMIC_COPY:
      return GL_DYNAMIC_COPY;
    case GFX_BUFFER_MODE_DYNAMIC_DRAW:
      return GL_DYNAMIC_DRAW;
    case GFX_BUFFER_MODE_DYNAMIC_READ:
      return GL_DYNAMIC_READ;
    case GFX_BUFFER_MODE_STATIC_COPY:
      return GL_STATIC_COPY;
    case GFX_BUFFER_MODE_STATIC_DRAW:
      return GL_STATIC_DRAW;
    case GFX_BUFFER_MODE_STATIC_READ:
      return GL_STATIC_READ;
    case GFX_BUFFER_MODE_STREAM_COPY:
      return GL_STREAM_COPY;
    case GFX_BUFFER_MODE_STREAM_DRAW:
      return GL_STREAM_DRAW;
    case GFX_BUFFER_MODE_STREAM_READ:
      return GL_STREAM_READ;
    default:
      return 0;
  }
}

static sizei get_layout_size(const GfxBufferLayout layout) {
  switch(layout) {
    case GFX_LAYOUT_FLOAT1:
      return sizeof(f32);
    case GFX_LAYOUT_FLOAT2:
      return sizeof(f32) * 2;
    case GFX_LAYOUT_FLOAT3:
      return sizeof(f32) * 3;
    case GFX_LAYOUT_FLOAT4:
      return sizeof(f32) * 4;
    case GFX_LAYOUT_INT1:
      return sizeof(i32);
    case GFX_LAYOUT_INT2:
      return sizeof(i32) * 2;
    case GFX_LAYOUT_INT3:
      return sizeof(i32) * 3;
    case GFX_LAYOUT_INT4:
      return sizeof(i32) * 4;
    case GFX_LAYOUT_UINT1:
      return sizeof(u32);
    case GFX_LAYOUT_UINT2:
      return sizeof(u32) * 2;
    case GFX_LAYOUT_UINT3:
      return sizeof(u32) * 3;
    case GFX_LAYOUT_UINT4:
      return sizeof(u32) * 4;
    default: 
      return 0;
  }
}

static sizei get_layout_type(const GfxBufferLayout layout) {
  switch(layout) {
    case GFX_LAYOUT_FLOAT1:
    case GFX_LAYOUT_FLOAT2:
    case GFX_LAYOUT_FLOAT3:
    case GFX_LAYOUT_FLOAT4:
      return GL_FLOAT;
    case GFX_LAYOUT_INT1:
    case GFX_LAYOUT_INT2:
    case GFX_LAYOUT_INT3:
    case GFX_LAYOUT_INT4:
      return GL_INT;
    case GFX_LAYOUT_UINT1:
    case GFX_LAYOUT_UINT2:
    case GFX_LAYOUT_UINT3:
    case GFX_LAYOUT_UINT4:
      return GL_UNSIGNED_INT;
    default:
      return 0;
  }
}

static sizei get_layout_count(const GfxBufferLayout layout) {
  switch(layout) {
    case GFX_LAYOUT_FLOAT1:
    case GFX_LAYOUT_INT1:
    case GFX_LAYOUT_UINT1:
      return 1;
    case GFX_LAYOUT_FLOAT2:
    case GFX_LAYOUT_INT2:
    case GFX_LAYOUT_UINT2:
      return 2;
    case GFX_LAYOUT_FLOAT3:
    case GFX_LAYOUT_INT3:
    case GFX_LAYOUT_UINT3:
      return 3;
    case GFX_LAYOUT_FLOAT4:
    case GFX_LAYOUT_INT4:
    case GFX_LAYOUT_UINT4:
      return 4;
    default:
      return 0;
  }
}

static sizei calc_stride(const GfxBufferLayout* layout, const sizei count) {
  sizei stride = 0; 

  for(sizei i = 0; i < count; i++) {
    stride += get_layout_size(layout[i]);
  }

  return stride;
}

static void seperate_shader_src(const i8* src, GfxShader* shader) {
  sizei src_len = strlen(src);
  
  sizei vert_start = 0;
  sizei frag_start = 0;

  // Find the vertex start
  for(sizei i = 0; i < src_len; i++) {
    if(src[i] != '#') { 
      continue;
    }

    // We found the start
    vert_start = i; 
    break;
  }

  // Find the frag start.
  // There's no point going from the beginning of the string. 
  // Just start from where we last stopped.
  for(sizei i = vert_start + 1; i < src_len; i++) {
    if(src[i] != '#') { 
      continue;
    }

    // We found the start
    frag_start = i; 
    break;
  }

  // Calculate where each shader ends
  sizei vert_end = frag_start - 1; 
  sizei frag_end = src_len;

  sizei frag_src_len = frag_end - frag_start;

  // // Allocate new strings
  shader->vert_src = (i8*)memory_allocate(vert_end);
  memory_zero(shader->vert_src, vert_end);

  shader->frag_src = (i8*)memory_allocate(frag_src_len);
  memory_zero(shader->frag_src, frag_src_len);

  // Copying the correct strings over 
  memory_copy(shader->vert_src, src, vert_end);
  memory_copy(shader->frag_src, &src[frag_start], frag_src_len);
}

static void check_shader_compile_error(const sizei shader) {
  i32 success;
  i8 log_info[512];

  glGetShaderiv(shader, GL_COMPILE_STATUS, &success); 

  if(!success) {
    glGetShaderInfoLog(shader, 512, nullptr, log_info);
    NIKOL_LOG_WARN("SHADER-ERROR: %s", log_info);
  }
}

static void check_shader_linker_error(const GfxShader* shader) {
  i32 success;
  i8 log_info[512];

  glGetProgramiv(shader->id, GL_LINK_STATUS, &success); 

  if(!success) {
    glGetProgramInfoLog(shader->id, 512, nullptr, log_info);
    NIKOL_LOG_WARN("SHADER-ERROR: %s", log_info);
  }
}

/// Private functions 
///---------------------------------------------------------------------------------------------------------------------

///---------------------------------------------------------------------------------------------------------------------
/// Context functions 

GfxContext* gfx_context_create(Window* window, const i32 flags) {
  GfxContext* gfx = (GfxContext*)memory_allocate(sizeof(GfxContext));
  
  gfx->buffer_bits = GL_COLOR_BUFFER_BIT;

  // Glad init
  if(!gladLoadGL()) {
    NIKOL_LOG_FATAL("Could not create an OpenGL instance");
    return nullptr;
  }
  
  // Setting the window context to this OpenGL context 
  window_set_current_context(window);

  // Setting up the viewport for OpenGL
  i32 width, height; 
  window_get_size(window, &width, &height);
  glViewport(0, 0, width, height);

  // Setting the flags
  gfx->flags = (GfxContextFlags)flags;
  set_gfx_flags(gfx);

  // Listening to events 
  event_listen(EVENT_WINDOW_FRAMEBUFFER_RESIZED, framebuffer_resize);

  // Getting some OpenGL information
  const u8* vendor = glGetString(GL_VENDOR); 
  const u8* renderer = glGetString(GL_RENDERER); 
  const u8* gl_version = glGetString(GL_VERSION);
  const u8* glsl_version = glGetString(GL_SHADING_LANGUAGE_VERSION);

  NIKOL_LOG_INFO("A graphics context was successfully created:\n" 
                 "              VENDOR: %s\n" 
                 "              RENDERER: %s\n" 
                 "              GL VERSION: %s\n" 
                 "              GLSL VERSION: %s", 
                 vendor, renderer, gl_version, glsl_version);
  return gfx;
}

void gfx_context_destroy(GfxContext* gfx) {
  if(!gfx) {
    return;
  }

  NIKOL_LOG_INFO("The graphics context was successfully destroyed");
  memory_free(gfx);
}

void gfx_context_clear(GfxContext* gfx, const f32 r, const f32 g, const f32 b, const f32 a) {
  NIKOL_ASSERT(gfx, "Invalid GfxContext struct passed");

  glClear(gfx->buffer_bits);
  glClearColor(r, g, b, a);
}

void gfx_context_set_flag(GfxContext* gfx, const i32 flag, const bool value) {
  NIKOL_ASSERT(gfx, "Invalid GfxContext struct passed");
}

const GfxContextFlags gfx_context_get_flags(GfxContext* gfx) {
  NIKOL_ASSERT(gfx, "Invalid GfxContext struct passed");
  return gfx->flags;
}

void gfx_context_draw(GfxContext* gfx, const GfxDrawCall* call) {
  NIKOL_ASSERT(gfx, "Invalid GfxContext struct passed");
  NIKOL_ASSERT(call, "Invalid GfxDrawCall struct passed");

  // Bind the shader 
  NIKOL_ASSERT(call->shader, "Must provide a shader for a draw call"); 
  glUseProgram(call->shader->id);

  // Bind/draw all of the textures if it is valid
  if(call->texture_count > 0) {
    for(sizei i = 0; i < call->texture_count; i++) {
      glActiveTexture(GL_TEXTURE0 + i);
      glBindTexture(GL_TEXTURE_2D, call->textures[i]->id);
    }
  }

  // Bind the vertex array
  glBindVertexArray(call->vao);

  // The vertex buffer MUST be valid
  NIKOL_ASSERT(call->vertex_buffer, "Cannot commit a draw call without a vertex buffer");

  // Always prioritize the index buffer 
  if(call->index_buffer) {
    glDrawElements(GL_TRIANGLES, call->index_buffer->elements_count, GL_UNSIGNED_INT, 0);
  }
  // Draw the vertex buffer instead if the index buffer is invalid 
  else if(call->vertex_buffer) {
    glDrawArrays(GL_TRIANGLES, 0, call->vertex_buffer->elements_count);
  }

  // Unbind the buffers 
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
  glBindVertexArray(0);
}

void gfx_context_draw_batch(GfxContext* gfx, GfxDrawCall** calls, const sizei count) {
  NIKOL_ASSERT(gfx, "Invalid GfxContext struct passed");
  NIKOL_ASSERT(calls, "Invalid calls array passed"); 
  
  for(sizei i = 0; i < count; i++) {
    gfx_context_draw(gfx, calls[i]);
  }
}

/// Context functions 
///---------------------------------------------------------------------------------------------------------------------

///---------------------------------------------------------------------------------------------------------------------
/// Shader functions 

GfxShader* gfx_shader_create(const i8* src) {
  GfxShader* shader = (GfxShader*)memory_allocate(sizeof(GfxShader));
  memory_zero(shader, sizeof(GfxShader));

  seperate_shader_src(src, shader);

  // Vertex shader
  shader->vert_id = glCreateShader(GL_VERTEX_SHADER);
  glShaderSource(shader->vert_id, 1, &shader->vert_src, 0); 
  glCompileShader(shader->vert_id);
  check_shader_compile_error(shader->vert_id);
    
  // Fragment shader
  shader->frag_id = glCreateShader(GL_FRAGMENT_SHADER);
  glShaderSource(shader->frag_id, 1, &shader->frag_src, 0); 
  glCompileShader(shader->frag_id);
  check_shader_compile_error(shader->frag_id);

  // Linking
  shader->id = glCreateProgram();
  glAttachShader(shader->id, shader->vert_id);
  glAttachShader(shader->id, shader->frag_id);
  glLinkProgram(shader->id);
  check_shader_linker_error(shader);
  
  // Detaching
  glDetachShader(shader->id, shader->vert_id);
  glDetachShader(shader->id, shader->frag_id);
  glDeleteShader(shader->vert_id);
  glDeleteShader(shader->frag_id);

  // // We don't need the strings anymore
  return shader;
}

void gfx_shader_destroy(GfxShader* shader) {
  if(!shader) {
    return;
  }
  
  memory_free(shader->vert_src);
  memory_free(shader->frag_src);

  glDeleteProgram(shader->id);
  memory_free(shader);
}

/// Shader functions 
///---------------------------------------------------------------------------------------------------------------------

///---------------------------------------------------------------------------------------------------------------------
/// Texture functions 

GfxTexture* gfx_texture_create(void* data, 
                               const sizei channels_count, 
                               const i32 width, 
                               const i32 height,
                               const GfxTextureFromat format, 
                               const GfxTextureFilter filter) {
  GfxTexture* texture = (GfxTexture*)memory_allocate(sizeof(GfxTexture)); 

  return texture;
}

void gfx_texture_destroy(GfxTexture* texture) {
  if(!texture) {
    return;
  }
    
  memory_free(texture);
}

void gfx_texture_get_size(GfxTexture* texture, i32* width, i32* height) {
  NIKOL_ASSERT(texture, "Could not retrieve the size of an invalid texture");
  
  *width  = texture->width;
  *height = texture->height; 
}

void gfx_texture_get_pixels(GfxTexture* texture, void* pixels) {
  NIKOL_ASSERT(texture, "Could not retrieve the size of an invalid texture");
  pixels = texture->pixels;
}

/// Texture functions 
///---------------------------------------------------------------------------------------------------------------------

///---------------------------------------------------------------------------------------------------------------------
/// DrawCall functions 

/// Create and return a `GfxDrawCall` struct. 
GfxDrawCall* gfx_draw_call_create(GfxContext* gfx) {
  NIKOL_ASSERT(gfx, "Invalid GfxContext struct passed");
  
  GfxDrawCall* call = (GfxDrawCall*)memory_allocate(sizeof(GfxDrawCall));
  memory_zero(call, sizeof(GfxDrawCall));

  glGenVertexArrays(1, &call->vao);

  return call;
}

void gfx_draw_call_push_buffer(GfxDrawCall* call, GfxContext* gfx, const GfxBufferDesc* buff) {
  NIKOL_ASSERT(gfx, "Invalid GfxContext struct passed");
  NIKOL_ASSERT(call, "Invalid GfxDrawCall struct passed");

  glBindVertexArray(call->vao);

  GLenum gl_data_mode = get_buffer_mode(buff->mode);

  switch(buff->type) {
    case GFX_BUFFER_VERTEX:
      glGenBuffers(1, &call->vbo);
      glBindBuffer(GL_ARRAY_BUFFER, call->vbo);
      glBufferData(GL_ARRAY_BUFFER, buff->size, buff->data, gl_data_mode);
      call->vertex_buffer = (GfxBufferDesc*)buff;
      break;
    case GFX_BUFFER_INDEX:
      glGenBuffers(1, &call->ebo);
      glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, call->ebo);
      glBufferData(GL_ELEMENT_ARRAY_BUFFER, buff->size, buff->data, gl_data_mode);
      call->index_buffer = (GfxBufferDesc*)buff;
      break;
  }
  
  glBindVertexArray(0);
}

void gfx_draw_call_set_layout(GfxDrawCall* call, GfxContext* gfx, const GfxBufferDesc* buff, GfxBufferLayout* layout, const sizei layout_count) {
  NIKOL_ASSERT(gfx, "Invalid GfxContext struct passed");
  NIKOL_ASSERT(call, "Invalid GfxDrawCall struct passed");
  NIKOL_ASSERT(layout, "Empty layout array passed into buffer");

  glBindVertexArray(call->vao);
  glBindBuffer(GL_ARRAY_BUFFER, call->vbo);

  sizei stride = calc_stride(layout, layout_count);
  sizei size   = get_layout_size(layout[0]);
  
  for(sizei i = 0; i < layout_count; i++) {
    glEnableVertexAttribArray(i);

    sizei offset        = i * size; 
    GLenum gl_comp_type = get_layout_type(layout[i]);
    sizei comp_count    = get_layout_count(layout[i]);
    
    size = get_layout_size(layout[i]);

    glVertexAttribPointer(i, comp_count, gl_comp_type, false, stride, (void*)offset);
  }
  
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindVertexArray(0);
}

void gfx_draw_call_push_shader(GfxDrawCall* call, GfxContext* gfx, const GfxShader* shader) {
  NIKOL_ASSERT(gfx, "Invalid GfxContext struct passed");
  NIKOL_ASSERT(call, "Invalid GfxDrawCall struct passed");
  NIKOL_ASSERT(shader, "Invalid GfxShader struct passed");

  call->shader = (GfxShader*)shader;
}

void gfx_draw_call_push_texture(GfxDrawCall* call, GfxContext* gfx, const GfxTexture* texture) {
  NIKOL_ASSERT(gfx, "Invalid GfxContext struct passed");
  NIKOL_ASSERT(call, "Invalid GfxDrawCall struct passed");
  NIKOL_ASSERT(texture, "Invalid GfxTexture struct passed");
  
  call->textures[call->texture_count] = (GfxTexture*)texture;
  call->texture_count++;
}

void gfx_draw_call_push_texture_batch(GfxDrawCall* call, GfxContext* gfx, const GfxTexture** textures, const sizei count) {
  for(sizei i = 0; i < count; i++) {
    gfx_draw_call_push_texture(call, gfx, textures[i]);
  }
}

void gfx_draw_call_destroy(GfxDrawCall* call) {
  if(!call) {
    return;
  }

  glDeleteBuffers(1, &call->vbo);
  glDeleteBuffers(1, &call->ebo);
  glDeleteVertexArrays(1, &call->vao);

  memory_free(call);
}

/// DrawCall functions 
///---------------------------------------------------------------------------------------------------------------------

/// *** Graphics ***
/// ---------------------------------------------------------------------

} // End of nikol

#endif // OpenGL check

//////////////////////////////////////////////////////////////////////////