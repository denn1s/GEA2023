#include <print.h>

#include "Pong.h"
#include "Systems.h"

#include "ECS/Entity.h"

#include "Game/Graphics/PixelShader.h"

Pong::Pong(const char* name, int width, int height)
  : Game(name, width, height)
{
  Scene* gameplayScene = createGameplayScene();
  setScene(gameplayScene);
}

Pong::~Pong() {
  
}

Scene* Pong::createGameplayScene() {
  Scene* scene = new Scene("GAMEPLAY SCENE");

  Entity white = scene->createEntity("cat1", 0, 0);
  auto& s = white.addComponent<SpriteComponent>(
    "Sprites/Cat/SpriteSheet.png",
    0, 0,
    48,
    8,
    1000
  );
  s.lastUpdate = SDL_GetTicks();

  Entity black = scene->createEntity("cat2", 40, 0);
  black.addComponent<SpriteComponent>(
    "Sprites/Cat/SpriteSheet.png", 
    0, 0,
    48,
    8,
    1000,
    PixelShader{
      [](Uint32 color) -> Uint32 { return (color == 0xF3F2C0FF) ? 0xD2B48CFF : color ; },
      "red"
    },
    SDL_GetTicks()
  );


  scene->addSetupSystem(new SpriteSetupSystem(renderer));
  scene->addRenderSystem(new SpriteRenderSystem());
  scene->addUpdateSystem(new SpriteUpdateSystem());



  return scene;
}