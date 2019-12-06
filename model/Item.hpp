#ifndef _MODEL_ITEM_HPP_
#define _MODEL_ITEM_HPP_

#include "../Stream.hpp"
#include <memory>
#include <string>
#include <stdexcept>
#include <stdexcept>
#include "WeaponType.hpp"

class Item {
public:
	enum Type {
		HEALTH_PACK = 0,
		WEAPON = 1,
		MINE = 2
	};
    //class HealthPack;
    //class Weapon;
    //class Mine;

	int type = -1;
	WeaponType weaponType = (WeaponType)-1;
	int health = 0;

    static std::shared_ptr<Item> readFrom(InputStream& stream);
    void writeTo(OutputStream& stream) const;
    std::string toString() const;
};

//class Item::HealthPack : public Item {
//public:
//    static const int TAG = 0;
//public:
//    int health;
//    HealthPack();
//    HealthPack(int health);
//    static HealthPack readFrom(InputStream& stream);
//    void writeTo(OutputStream& stream) const;
//    std::string toString() const override;
//};
//
//class Item::Weapon : public Item {
//public:
//    static const int TAG = 1;
//public:
//    WeaponType weaponType;
//    Weapon();
//    Weapon(WeaponType weaponType);
//    static Weapon readFrom(InputStream& stream);
//    void writeTo(OutputStream& stream) const;
//    std::string toString() const override;
//};
//
//class Item::Mine : public Item {
//public:
//    static const int TAG = 2;
//public:
//    Mine();
//    static Mine readFrom(InputStream& stream);
//    void writeTo(OutputStream& stream) const;
//    std::string toString() const override;
//};

#endif
