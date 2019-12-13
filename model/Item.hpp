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

	Type type;
	WeaponType weaponType;
	int health;

    static Item readFrom(InputStream& stream);
    void writeTo(OutputStream& stream) const;
    std::string toString() const;
};

#endif
