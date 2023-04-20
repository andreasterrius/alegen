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
        FT_Set_Pixel_Sizes(face, 0, 48);  

        defaultFontKey = FontKey{path};

        fonts[defaultFontKey] = {};
    }

    vector<Sprite> layoutText(vec3 originPos, string text, vec3 color) {
        if(fonts.find(defaultFontKey) == fonts.end()) {
            return {};
        }

        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        
        //gives 0 shit about unicode for now
        vector<Sprite> sprites;
        for(int i = 0; i < text.size(); ++i) {
            if(fonts[defaultFontKey].find(string(&text[i])) == fonts[defaultFontKey].end()){
                if(FT_Load_Char(face, text[i], FT_LOAD_RENDER)){
                    cout << "unable to render " << text[i] << " glyph" << endl;
                    continue;
                }
                int width = face->glyph->bitmap.width;
                int height = face->glyph->bitmap.rows;

                // Create an OpenGL texture object from the glyph bitmap
                GLuint textureId;
                glGenTextures(1, &textureId);
                glBindTexture(GL_TEXTURE_2D, textureId);
                glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, width, height, 0, GL_RED, GL_UNSIGNED_BYTE, face->glyph->bitmap.buffer);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                
                fonts[defaultFontKey][string(&text[i])] = FontCharacter {
                    width, height, 
                    (int) face->glyph->advance.x, (int) face->glyph->advance.y,
                    (int) face->glyph->bitmap_left, (int) face->glyph->bitmap_top,
                    textureId,
                };
            }
            FontCharacter fc = fonts[defaultFontKey][string(&text[i])];    
            float x = originPos.x + fc.bearingX;
            float y = originPos.y - fc.bearingY;
        
            sprites.push_back(Sprite{vec3(x, y, originPos.z), vec2(fc.width, fc.height), vec4(color,SOLID), fc.textureId});
            
            originPos.x += (fc.advanceX >> 6);
        }

        return sprites;
    }

    ~TextRenderer() {
        FT_Done_Face(face);
        FT_Done_FreeType(library);
    }

};