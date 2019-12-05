#ifndef _MODEL_UNIT_ACTION_HPP_
#define _MODEL_UNIT_ACTION_HPP_

#include "../Stream.hpp"
#include <string>
#include <stdexcept>
#include "Vec2Double.hpp"

class UnitAction {
public:
    double velocity = 0;
    bool jump = false;
    bool jumpDown = false;
    Vec2Double aim = Vec2(0,0);
	bool shoot = false;
    bool reload = false;
    bool swapWeapon = false;
    bool plantMine = false;
    UnitAction();
    UnitAction(double velocity, bool jump, bool jumpDown, Vec2Double aim = Vec2Double(0,0), bool shoot = false, bool reload = false, bool swapWeapon = false, bool plantMine = false);
    static UnitAction readFrom(InputStream& stream);
    void writeTo(OutputStream& stream) const;
    std::string toString() const;
};

#endif
