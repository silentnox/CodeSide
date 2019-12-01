#ifndef _MODEL_VEC2_FLOAT_HPP_
#define _MODEL_VEC2_FLOAT_HPP_

#include "../Stream.hpp"
#include <string>

#include "../Helpers.h"

class Vec2Float {
public:
    float x;
    float y;
    Vec2Float();
    Vec2Float(float x, float y);
	Vec2Float( Vec2 in );
    static Vec2Float readFrom(InputStream& stream);
    void writeTo(OutputStream& stream) const;
    std::string toString() const;
};

#endif
