#pragma once

struct PixelShader {
    std::function<Uint32(Uint32)> func;
    std::string name;
};