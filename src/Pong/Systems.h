#pragma once
#include <print.h>
#include "Components.h"

#include "ECS/Entity.h"
#include "ECS/SystemTypes/SystemTypes.h"

#include "Game/Graphics/TextureManager.h"


class HelloWorldSystem : public SetupSystem {
  public:
    HelloWorldSystem() {
      print("Hello World Constructor");
    }

    ~HelloWorldSystem() {
      print("Hello World Destructor");
    }

    void run() {
      print("Hello World run!");
    }
};

class RectRenderSystem : public RenderSystem {
  void run(SDL_Renderer* renderer) {
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 1);

    const auto view = scene->r.view<TransformComponent, SizeComponent>();

    for (const auto entity : view) {
      const auto t = view.get<TransformComponent>(entity);
      const auto s = view.get<SizeComponent>(entity);


      int x = t.position.x;
      int y = t.position.y;
      int w = s.w;
      int h = s.h;

      SDL_Rect rect = { x, y, w, h};
      SDL_RenderFillRect(renderer, &rect);
    }
  }
};

class MovementUpdateSystem : public UpdateSystem {
  public:
    MovementUpdateSystem(int screen_width, int screen_height) 
    : screen_width(screen_width), screen_height(screen_height) {}

    void run(float dT) {
      const auto view = scene->r.view<TransformComponent, SpeedComponent, SizeComponent>();

      for (const auto entity : view) {
        auto& t = view.get<TransformComponent>(entity);
        auto& s = view.get<SpeedComponent>(entity);
        auto& sz = view.get<SizeComponent>(entity);


        if (s.x == 0 && s.y == 0) {
          continue;
        }

        const int nx = t.position.x + s.x * dT;
        const int ny = t.position.y + s.y * dT;

        if (nx <= 0) {
          s.x *= -1.2;
        }
        if (nx + sz.w >= screen_width) {
          s.x *= -1.2;
        }
        if (ny <= 0) {
          s.y *= -1.2;
        }
        if (ny + sz.h > screen_height) {
          exit(1);
        }

        t.position.x = nx;
        t.position.y = ny;
      }
    }
  private:
    int screen_width;
    int screen_height;
};


class PlayerInputSystem : public EventSystem {
  void run(SDL_Event event) {
    scene->r.view<SpeedComponent, PlayerComponent>()
      .each(
        [&](const auto entity, auto& s, auto& p) {
          if (event.type == SDL_KEYDOWN) {
            switch(event.key.keysym.sym) {
              case SDLK_RIGHT:
                s.x = p.moveSpeed;
                break;
              case SDLK_LEFT:
                s.x = -p.moveSpeed;
                break;
            }
          }
          if (event.type == SDL_KEYUP) {
            s.x = 0;
          }
        }
      );
    }
};

class CollisionDetectionUpdateSystem : public UpdateSystem {
  void run(float dT) {
    scene->r.view<TransformComponent, SizeComponent, ColliderComponent>()
      .each(
        [&](const auto entity,
            auto& transformComponent,
            auto& sizeComponent,
            auto& colliderComponent
          ) {
            // cada entidad que tiene collider
            // AABB
            scene->r.view<TransformComponent, SpeedComponent, SizeComponent>()
            .each(
              [&](const auto entity2,
                  auto& transformComponent2,
                  auto& speedComponent2,
                  auto& sizeComponent2
                ) {
                  if (entity == entity2) {
                    // skip self collision
                    return;
                  }

                  SDL_Rect boxCol1 = {
                    static_cast<int>(transformComponent.position.x),
                    static_cast<int>(transformComponent.position.y),
                    sizeComponent.w,
                    sizeComponent.h
                  };

                  SDL_Rect boxCol2 = {
                    static_cast<int>(transformComponent2.position.x),
                    static_cast<int>(transformComponent2.position.y),
                    sizeComponent2.w,
                    sizeComponent2.h
                  };

                  if (SDL_HasIntersection(&boxCol1, &boxCol2)) {
                    colliderComponent.triggered = true;
                    colliderComponent.transferSpeed = speedComponent2.x;
                  }

              }
            );
        }
      );
    }
};

class BounceUpdateSystem : public UpdateSystem {
  void run(float dT) {
    scene->r.view<TransformComponent, SpeedComponent, ColliderComponent>()
      .each(
        [&](const auto entity,
            auto& transformComponent,
            auto& speedComponent,
            auto& colliderComponent
          ) {
            if (colliderComponent.triggered) {
              speedComponent.y *= -1.5;
              speedComponent.x += colliderComponent.transferSpeed;

              colliderComponent.triggered = false;
            }
          }
      );
    }
};

class SpriteSetupSystem : public SetupSystem {
  public:
    SpriteSetupSystem(SDL_Renderer* renderer)
      : renderer(renderer) { }

    ~SpriteSetupSystem() {
      auto view = scene->r.view<SpriteComponent>();

      for(auto entity : view) {
        const auto spriteComponent = view.get<SpriteComponent>(entity);
  
        TextureManager::UnloadTexture(spriteComponent.name, spriteComponent.shader.name);
      }
    }

    void run() {
      auto view = scene->r.view<SpriteComponent>();

      for(auto entity : view) {
        const auto spriteComponent = view.get<SpriteComponent>(entity);
  
        TextureManager::LoadTexture(spriteComponent.name, renderer, spriteComponent.shader);
      }
    }

  private:
    SDL_Renderer* renderer;
};

class SpriteRenderSystem : public RenderSystem {
  public:
    void run(SDL_Renderer* renderer) {
      auto view = scene->r.view<TransformComponent, SpriteComponent>();

      for(auto entity : view) {
        const auto spriteComponent = view.get<SpriteComponent>(entity);
        const auto transformComponent = view.get<TransformComponent>(entity);
  
        Texture* texture = TextureManager::GetTexture(spriteComponent.name, spriteComponent.shader.name);
  
        SDL_Rect clip = {
          spriteComponent.xIndex * spriteComponent.size,
          spriteComponent.yIndex * spriteComponent.size,
          spriteComponent.size,
          spriteComponent.size
        };

        texture->render(
          transformComponent.position.x,
          transformComponent.position.y,
          48 * 5,
          48 * 5,
          &clip
        );
      }
    }
};

class SpriteUpdateSystem : public UpdateSystem {
  public:
    void run(float dT) {
      auto view = scene->r.view<SpriteComponent>();

      Uint32 now = SDL_GetTicks();

      for(auto entity : view) {
        auto& spriteComponent = view.get<SpriteComponent>(entity);

        if (spriteComponent.animationFrames > 0) {
          float timeSinceLastUpdate = now - spriteComponent.lastUpdate;

          int framesToUpdate = static_cast<int>(
            timeSinceLastUpdate / 
            spriteComponent.animationDuration * spriteComponent.animationFrames
          );

          if (framesToUpdate > 0) {
            spriteComponent.xIndex += framesToUpdate;
            spriteComponent.xIndex %= spriteComponent.animationFrames;
            spriteComponent.lastUpdate = now;            
          }
        }
      }
    }
};


class TilemapSetupSystem : public SetupSystem {
  public:
    TilemapSetupSystem(SDL_Renderer* renderer)
      : renderer(renderer) { }

    ~TilemapSetupSystem() {
    }

    void run() {
      Texture* waterTexture = TextureManager::LoadTexture("Tiles/Water.png", renderer);
      Texture* grassTexture = TextureManager::LoadTexture("Tiles/Grass.png", renderer);

      int map[] = {
        0, 1, 0,
        0, 1, 0,
        0, 0, 1
      };

      auto& tilemap = scene->world->get<TilemapComponent>();
      tilemap.width = 3;
      tilemap.height = 3;
      tilemap.tileSize = 16;

      for(int i = 0; i < tilemap.height * tilemap.width; i++) {
        tilemap.map.push_back((map[i] == 0) ? grassTexture : waterTexture);
      }
    }

  private:
    SDL_Renderer* renderer;
};


class TilemapRenderSystem : public RenderSystem {
  public:
    void run(SDL_Renderer* renderer) {
      auto& tilemap = scene->world->get<TilemapComponent>();

      for (int y = 0; y < tilemap.height; y++) {
        for (int x = 0; x < tilemap.width; x++) {
          Texture* texture = tilemap.map[y * tilemap.width + x];

          int size = tilemap.tileSize * 5;

          texture->render(
            x * size,
            y * size,
            size,
            size
          );
        }
      }
    }
};