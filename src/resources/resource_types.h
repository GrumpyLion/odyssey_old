#pragma once

#include <vector>
#include <string>

enum class ResourceType
{
    TYPE_TEXT,
    TYPE_BINARY,
    TYPE_TEXTURE,
    TYPE_MATERIAL,
    TYPE_SHADER,
    TYPE_MESH,
    TYPE_CUSTOM
};

struct Resource
{
    int myLoaderId{};
    const char* myName{};
    char* myFullPath{};
    long mySize{};
    std::vector<char> mydata{};
};

struct ImageParams
{
    bool myFlipY{};
};

struct ImageData
{
    char myChannelCount{};
    int myWidth{};
    int myHeight{};
    std::vector<char> myPixels{};
};

enum class FaceCullMode
{
    NONE,
    FRONT,
    BACK,
    FRONT_AND_BACK
};

enum class TextureFlag : char
{
    HAS_TRANSPARENCY = 0x1,
    IS_WRITEABLE = 0x2,
    IS_WRAPPED = 0x4
};

typedef char TextureFlagBits;

enum class TextureType
{
    TYPE_2D,
    TYPE_CUBE
};

struct Texture
{
    uint32_t myID{};
    TextureType myType{};
    int myWidth{};
    int myHeight{};
    char myChannelCount{};
    TextureFlagBits myFlags{};
    int myGeneration{};
    std::string myName{};
    std::vector<char> myPixels{};
};

enum class TextureUsage : char
{
    UNKNOWN = 0x00,
    DIFFUSE_MAP = 0x01,
    SPECULAR_MAP = 0x02,
    NORMAL_MAP = 0x03,
    CUBEMAP_MAP = 0x04
};

enum class TextureFilter : char
{
    NEAREST_FILTER = 0x0,
    LINEAR_FILTER = 0x1
};

enum class TextureRepeat : char
{
    REPEAT = 0x1,
    MIRRORED_REPEAT = 0x2,
    CLAMP_TO_EDGE = 0x3,
    CLAMP_TO_BORDER = 0x4
};

struct TextureMap
{
    Texture* myTexture{};
    TextureUsage myUsage{};
    TextureFilter myFilterMinify{};
    TextureFilter myFilterMagnify{};
    TextureRepeat myRepeatU{};
    TextureRepeat myRepeatV{};
    TextureRepeat myRepeatW{};
    void* myAPIData{};
};