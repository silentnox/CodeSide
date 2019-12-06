#include "Item.hpp"


//Item::HealthPack::HealthPack() {
//	type = 0;
//}
//Item::HealthPack::HealthPack(int health) : health(health) { }
//Item::HealthPack Item::HealthPack::readFrom(InputStream& stream) {
//    Item::HealthPack result;
//    result.health = stream.readInt();
//    return result;
//}
//void Item::HealthPack::writeTo(OutputStream& stream) const {
//    stream.write(TAG);
//    stream.write(health);
//}
//std::string Item::HealthPack::toString() const {
//    return std::string("Item::HealthPack") + "(" +
//        std::to_string(health) +
//        ")";
//}
//
//Item::Weapon::Weapon() {
//	type = 1;
//}
//Item::Weapon::Weapon(WeaponType weaponType) : weaponType(weaponType) { }
//Item::Weapon Item::Weapon::readFrom(InputStream& stream) {
//    Item::Weapon result;
//    switch (stream.readInt()) {
//    case 0:
//        result.weaponType = WeaponType::PISTOL;
//        break;
//    case 1:
//        result.weaponType = WeaponType::ASSAULT_RIFLE;
//        break;
//    case 2:
//        result.weaponType = WeaponType::ROCKET_LAUNCHER;
//        break;
//    default:
//        throw std::runtime_error("Unexpected discriminant value");
//    }
//    return result;
//}
//void Item::Weapon::writeTo(OutputStream& stream) const {
//    stream.write(TAG);
//    stream.write((int)(weaponType));
//}
//std::string Item::Weapon::toString() const {
//    return std::string("Item::Weapon") + "(" +
//        "TODO" + 
//        ")";
//}
//
//Item::Mine::Mine() {
//	type = 2;
//}
//Item::Mine Item::Mine::readFrom(InputStream& stream) {
//    Item::Mine result;
//    return result;
//}
//void Item::Mine::writeTo(OutputStream& stream) const {
//    stream.write(TAG);
//}
//std::string Item::Mine::toString() const {
//    return std::string("Item::Mine") + "(" +
//        ")";
//}
std::shared_ptr<Item> Item::readFrom(InputStream& stream) {
	std::shared_ptr<Item> item = std::shared_ptr<Item>( new Item() );
	item->type = stream.readInt();
    switch (item->type) {
    case 0:
		item->health = stream.readInt();
        //return std::shared_ptr<Item::HealthPack>(new Item::HealthPack(Item::HealthPack::readFrom(stream)));
		break;
    case 1:
		item->weaponType = (WeaponType)stream.readInt();
        //return std::shared_ptr<Item::Weapon>(new Item::Weapon(Item::Weapon::readFrom(stream)));
		break;
    case 2:
        //return std::shared_ptr<Item::Mine>(new Item::Mine(Item::Mine::readFrom(stream)));
		break;
    default:
        throw std::runtime_error("Unexpected discriminant value");
    }
	return item;
}
void Item::writeTo( OutputStream & stream ) const{
}
std::string Item::toString() const{
	return std::string();
};
