#pragma once

#include <glm/glm.hpp>
#include <memory>
#include <unordered_map>

#include "defines.h"

struct AppState;
struct ReloadDesc;

class Renderer {
 public:
  Renderer() = delete;
  Renderer(AppState* state) : app_state_(state), frame_number_(0){};
  virtual ~Renderer(){};

  virtual b8 Init() = 0;
  virtual void Load(ReloadDesc* desc) = 0;
  virtual void UnLoad(ReloadDesc* desc) = 0;
  virtual void Update(float deltaTime) = 0;
  virtual void Draw() = 0;
  virtual void Shutdown() = 0;
  // virtual b8 OnResize(u32 w, u32 h) = 0;

 protected:
  // platform internal state
  AppState* app_state_;
  u32 frame_number_;
};