#include "UnitAction.hpp"

UnitAction::UnitAction() : velocity( 0 ), jump( false ), jumpDown( false ), aim( Vec2(0,0) ), shoot( false ), swapWeapon( false ), plantMine( false ) {
	
}
UnitAction::UnitAction(double velocity, bool jump, bool jumpDown, Vec2Double aim, bool shoot, bool swapWeapon, bool plantMine) : velocity(velocity), jump(jump), jumpDown(jumpDown), aim(aim), shoot(shoot), swapWeapon(swapWeapon), plantMine(plantMine) { }
UnitAction UnitAction::readFrom(InputStream& stream) {
    UnitAction result;
    result.velocity = stream.readDouble();
    result.jump = stream.readBool();
    result.jumpDown = stream.readBool();
    result.aim = Vec2Double::readFrom(stream);
    result.shoot = stream.readBool();
    result.swapWeapon = stream.readBool();
    result.plantMine = stream.readBool();
    return result;
}
void UnitAction::writeTo(OutputStream& stream) const {
    stream.write(velocity);
    stream.write(jump);
    stream.write(jumpDown);
    aim.writeTo(stream);
    stream.write(shoot);
    stream.write(swapWeapon);
    stream.write(plantMine);
}
std::string UnitAction::toString() const {
    return std::string("UnitAction") + "(v:" +
        std::to_string(velocity) + " j:" +
        (jump ? "true" : "false") + " jd:" +
        (jumpDown ? "true" : "false") + " a:" +
        aim.toString() + " shoot:" +
        (shoot ? "true" : "false") + " swap:" +
        (swapWeapon ? "true" : "false") + " plant:" +
        (plantMine ? "true" : "false") +
        ")";
}
