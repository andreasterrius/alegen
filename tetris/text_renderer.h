#pragma once

#include <glad/glad.h>
#include <iostream>
#include <string>
#include <stdexcept>
#include <unordered_map>
#include <vector>
#include <ft2build.h>
#include FT_FREETYPE_H

#include "sprite_renderer.h"

using namespace std;

struct FontKey {
    string fontName;

    bool operator==(const FontKey& other) const {
        return (fontName == other.fontName);
    }
};

namespace std {
    template <>
    struct hash<FontKey> {
        std::size_t operator()(const FontKey& k) const {
            std::size_t h1 = std::hash<string>()(k.fontName);
            return h1;
        }
    };
}

struct FontCharacter {
    int width;
    int height;
    int advanceX;
    int advanceY;
    int bearingX;
    int bearingY;
    GLuint textureId;
};

class TextRenderer {
public:
    FT_Library library;
    FT_Face face;

    /*FontKey => {
        'A' => FontCharacter{ width, height, blabla },
        'B' => ...
    }*/
    unordered_map<FontKey, unordered_map<string, FontCharacter>> fonts;

    FontKey defaultFontKey;

    TextRenderer(string path) {
        if(FT_Init_FreeType(&library)){
            throw runtime_error("unable to load FT_Init_FreeType");
        }
        if(FT_New_Face(library, path.c_str(), 0 , &face)) {
            throw runtime_error("unable to load FT_New_Face");
        }

        defaultFontKey = FontKey{path};

        fonts[defaultFontKey] = {};
    }

    vector<Sprite> layoutText(vec3 originPos, string text) {
        if(fonts.find(defaultFontKey) == fonts.end()) {
            return {};
        }
        //gives 0 shit about unicode for now
        vector<Sprite> sprites;
        for(int i = 0; i < text.size(); ++i) {
            if(fonts[defaultFontKey].find(string(&text[i])) == fonts[defaultFontKey].end()){
                if(FT_Load_Char(face, text[i], FT_LOAD_RENDER)){
                    cout << "unable to render " << text[i] << " glyph" << endl;
                    continue;
                }
                FT_Bitmap bitmap = face->glyph->bitmap;
                int width = bitmap.width;
                int height = bitmap.rows;
                unsigned char* pixels = bitmap.buffer;

                // Create an OpenGL texture object from the glyph bitmap
                GLuint textureId;
                glGenTextures(1, &textureId);
                glBindTexture(GL_TEXTURE_2D, textureId);
                glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, width, height, 0, GL_RED, GL_UNSIGNED_BYTE, pixels);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                
                fonts[defaultFontKey][string(&text[i])] = FontCharacter {
                    width, height, 
                    (int) face->glyph->advance.x, (int) face->glyph->advance.y,
                    (int) face->glyph->bitmap_left, (int) face->glyph->bitmap_top,
                };
            }
            FontCharacter fc = fonts[defaultFontKey][string(&text[i])];
            int advanceX = fc.advanceX >> 6;
            int advanceY = fc.advanceY >> 6;
            int bitmapWidth = fc.width;
            int bitmapHeight = fc.height;
            int bearingX = fc.bearingX;
            int bearingY = fc.bearingY;

            originPos.x += bearingX;
            originPos.y -= bearingY;
            sprites.push_back(Sprite{originPos, vec2(bitmapWidth, bitmapHeight), vec3(1.0), fc.textureId});
            
            originPos.x += advanceX;
            originPos.y += advanceY;
        }

        return sprites;
    }

    ~TextRenderer() {
        FT_Done_Face(face);
        FT_Done_FreeType(library);
    }

};