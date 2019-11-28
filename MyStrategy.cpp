#include "MyStrategy.hpp"
#include "Helpers.h"

MyStrategy::MyStrategy() {}

double distanceSqr(Vec2Double a, Vec2Double b) {
  return (a.x - b.x) * (a.x - b.x) + (a.y - b.y) * (a.y - b.y);
}

UnitAction Quickstart( const Unit &unit, const Game &game, Debug &debug ) {
	const Unit *nearestEnemy = nullptr;
	for (const Unit &other : game.units) {
		if (other.playerId != unit.playerId) {
			if (nearestEnemy == nullptr ||
				distanceSqr( unit.position, other.position ) <
				distanceSqr( unit.position, nearestEnemy->position )) {
				nearestEnemy = &other;
			}
		}
	}
	const LootBox *nearestWeapon = nullptr;
	for (const LootBox &lootBox : game.lootBoxes) {
		if (std::dynamic_pointer_cast<Item::Weapon>(lootBox.item)) {
			if (nearestWeapon == nullptr ||
				distanceSqr( unit.position, lootBox.position ) <
				distanceSqr( unit.position, nearestWeapon->position )) {
				nearestWeapon = &lootBox;
			}
		}
	}
	Vec2Double targetPos = unit.position;
	if (unit.weapon == nullptr && nearestWeapon != nullptr) {
		targetPos = nearestWeapon->position;
	}
	else if (nearestEnemy != nullptr) {
		targetPos = nearestEnemy->position;
	}
	int tile = game.level.tiles[floor( unit.position.x )][floor( unit.position.y - 0.1 )];
	debug.draw( CustomData::Log(
		std::string( "Target pos: " ) + targetPos.toString() ) );
	debug.draw( CustomData::Log(
		std::string( "gnd: " ) + std::to_string(unit.onGround) ));
	debug.draw( CustomData::Log(
		std::string( "stand: " ) + std::to_string( unit.stand ) ) );
	debug.draw( CustomData::Log(
		std::string( "tile: " ) + std::to_string( tile ) ) );
	Vec2Double aim = Vec2Double( 0, 0 );
	if (nearestEnemy != nullptr) {
		aim = Vec2Double( nearestEnemy->position.x - unit.position.x,
			nearestEnemy->position.y - unit.position.y );
	}
	bool jump = targetPos.y > unit.position.y;
	if (targetPos.x > unit.position.x &&
		game.level.tiles[size_t( unit.position.x + 1 )][size_t( unit.position.y )] ==
		Tile::WALL) {
		jump = true;
	}
	if (targetPos.x < unit.position.x &&
		game.level.tiles[size_t( unit.position.x - 1 )][size_t( unit.position.y )] ==
		Tile::WALL) {
		jump = true;
	}
	UnitAction action;
	action.velocity = targetPos.x - unit.position.x;
	action.jump = jump;
	action.jumpDown = !action.jump;
	action.aim = aim;
	action.shoot = true;
	action.swapWeapon = false;
	action.plantMine = false;
	action.jumpDown = false;
	return action;
}

Vec2 fv2d( const Vec2Double & v ) {
	return Vec2( v.x, v.y );
}
Vec2Double tv2d( const Vec2 & v ) {
	return Vec2Double( v.x, v.y );
}

class Sim {
public:

	class Unit {
	public:
		int playerId;
		int id;
		int health;
		Vec2 position;
		Vec2 size;
		bool onGround;
		bool onLadder;
		JumpState jumpState;
		UnitAction action;

		//float jumpTime = 0;
		int jumpTicks = 0;

		Unit( const ::Unit & u ) {
			playerId = u.playerId;
			id = u.id;
			health = u.health;
			position = fv2d( u.position );
			size = fv2d( u.size );
			onGround = u.onGround;
			onLadder = u.onLadder;
			jumpState = u.jumpState;
		}
	};

	Properties props;
	Level level;

	int currentTick;
	std::vector<Player> players;
	std::vector<Unit> units;
	std::vector<Bullet> bullets;
	std::vector<Mine> mines;
	std::vector<LootBox> lootBoxes;

	Sim() {
	}

	Rect GetUnitRect( const Unit & u ) const {
		return Rect( Vec2( u.position.x - u.size.x * 0.5, u.position.y ), Vec2( u.position.x + u.size.x * 0.5, u.position.y + u.size.y ) );
	}

	bool IsOnTile( const Rect & rect, Tile tile ) {
		for (int i = 0; i < 40; i++) {
			for (int j = 0; j < 30; j++) {
				Tile t = level.tiles[i][j];
				if (t == tile) {
					Vec2 c( i, j );
					Rect r( c, c + Vec2( 1, 1 ) );
					if (rect.Intersects( r )) return true;
				}
			}
		}
		return false;
	}

	Tile GetTileAt( const Vec2 & p ) const {
		int x = (int)floor( p.x );
		int y = (int)floor( p.y );
		return level.tiles[x][y];
	}

	void MoveUnit( Unit & u ) {
		UnitAction a = u.action;

		float delta = 1.0 / (props.ticksPerSecond * props.updatesPerTick);
		float moveDelta = props.unitMaxHorizontalSpeed * delta;

		Tile feetTile1 = GetTileAt( u.position + Vec2( -u.size.x * 0.5, -moveDelta ) );
		Tile feetTile2 = GetTileAt( u.position + Vec2( u.size.x * 0.5, -moveDelta ) );
		//Tile headTile = GetTileAt( u.position + Vec2( 0, u.size.y + moveDelta ) );

		Rect r = GetUnitRect( u );

		bool onPlatform = (feetTile1 == PLATFORM || feetTile2 == PLATFORM);
		bool inAir = feetTile1 == EMPTY && feetTile2 == EMPTY;
		bool isBumped = GetTileAt( u.position + Vec2( -u.size.x * 0.5, u.size.y + moveDelta ) ) == WALL || GetTileAt( u.position + Vec2( u.size.x * 0.5, u.size.y + moveDelta ) ) == WALL;
		bool touchJumppad = IsOnTile( r, JUMP_PAD );
		bool onLadder = GetTileAt( u.position + Vec2( 0, u.size.y * 0.5 ) ) == LADDER || GetTileAt( u.position + Vec2( 0, u.size.y ) ) == LADDER;


		//Vec2 offset;
		float ox = a.velocity * delta;
		float oy = 0;

		u.jumpState.canJump = !inAir && !u.jumpTicks;

		bool cancelJump = false;

		if (touchJumppad) {
			u.jumpState.canCancel = false;
			u.jumpState.speed = props.jumpPadJumpSpeed;
			u.jumpState.maxTime = props.jumpPadJumpTime;
			u.jumpTicks = u.jumpState.maxTime * (60 * 100);
			//u.jumpState.canJump = false;
		}
		else {
			if (a.jump) {
				if (onLadder) {
					oy = props.unitJumpSpeed;
				}
				else {
					if (u.jumpState.canJump) {
						u.jumpState.canCancel = true;
						u.jumpState.speed = props.unitJumpSpeed;
						u.jumpState.maxTime = props.unitJumpTime;
						u.jumpTicks = u.jumpState.maxTime * (60 * 100);
						//u.jumpState.canJump = false;
					}
				}
			}
			else {
				if (a.jumpDown) {
					if (onLadder || onPlatform) {
						oy = -props.unitFallSpeed;
					}
					else {
						if (u.jumpState.canCancel) {
							cancelJump = true;
						}
					}
				}
			}
		}

		if (isBumped) {
			cancelJump = true;
		}

		if (cancelJump) {
			u.jumpState.speed = -props.unitFallSpeed;
			u.jumpState.maxTime = 0;
			u.jumpTicks = 0;
		}

		if (u.jumpTicks) {
			oy = u.jumpState.speed;
			u.jumpTicks--;
		}
		else {
			if (inAir) {
				oy = -props.unitFallSpeed;
			}
			//u.jumpTime = 0;
			u.jumpTicks = 0;
		}

		oy *= delta;

		Rect test = r;
		r += Vec2( 0, oy );

		if (IsOnTile( r, WALL )) {
			oy = 0;
		}

		test = r;
		r += Vec2( ox, 0 );

		if (IsOnTile( r, WALL )) {
			ox = 0;
		}

		u.position.x += ox;
		u.position.y += oy;
	}

	void SetState( const Game & g ) {
		currentTick = g.currentTick;
		players = g.players;
		bullets = g.bullets;
		mines = g.mines;
		lootBoxes = g.lootBoxes;

		for (const ::Unit & u : g.units) {
			units.emplace_back( u );
		}
	}

	void Microtick() {
		for (Unit & u : units) {
			MoveUnit( u );
		}
	}

	void Tick() {
		for (int i = 0; i < props.updatesPerTick; i++) {
			Microtick();
		}
	}

	void Simulate( int numTicks ) {
		for (int i = 0; i < numTicks; i++) {
			Tick();
		}
	}

};

std::vector<CustomData::Rect> drawpath;
Sim sim;
void GenPath( const Unit &unit, const Game &game ) {
//	Sim sim;
	sim.level = game.level;
	sim.props = game.properties;
	sim.units.emplace_back( unit );
	sim.units[0].action.velocity = -10;
	sim.units[0].action.jump = true;
	sim.units[0].action.jumpDown = false;
	sim.units[0].action.shoot = false;
	sim.units[0].action.plantMine = false;
	sim.units[0].action.swapWeapon = false;

	for (int i = 0; i < 5 * 60; i++) {
		drawpath.emplace_back( CustomData::Rect( Vec2Float( sim.units[0].position.x, sim.units[0].position.y), Vec2Float( 0.1, 0.1 ), ColorFloat( 1, 1, 1, 1 )) );
		sim.Tick();
	}

}

UnitAction MyStrategy::getAction(const Unit &unit, const Game &game, Debug &debug) {
	if (game.currentTick == 0) {
		GenPath( unit, game );
	}
	for (CustomData::Rect r : drawpath) {
		debug.draw( r );
	}

	debug.draw( CustomData::Log( std::string( "Touch jumppad: " ) + std::to_string( sim.IsOnTile( sim.GetUnitRect(unit), JUMP_PAD ) ) ) );

	//return Quickstart( unit, game, debug );
	UnitAction a;
	a.velocity = -10;
	a.jump = true;
	a.jumpDown = false;
	a.plantMine = false;
	a.swapWeapon = false;
	a.shoot = false;

	return a;
}