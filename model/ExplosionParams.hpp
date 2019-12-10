#ifndef _MODEL_EXPLOSION_PARAMS_HPP_
#define _MODEL_EXPLOSION_PARAMS_HPP_

#include "../Stream.hpp"
#include <string>

class ExplosionParams {
public:
    double radius = 0.;
    int damage = 0;
    ExplosionParams();
    ExplosionParams(double radius, int damage);
    static ExplosionParams readFrom(InputStream& stream);
    void writeTo(OutputStream& stream) const;
    std::string toString() const;
};

#endif
