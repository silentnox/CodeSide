#include "MyStrategy.hpp"
#include "Helpers.h"

MyStrategy::MyStrategy() {}

double distanceSqr(Vec2 a, Vec2 b) {
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
	return action;
}

//Vec2 fv2d( const Vec2 & v ) {
//	return Vec2( v.x, v.y );
//}
//Vec2 tv2d( const Vec2 & v ) {
//	return Vec2( v.x, v.y );
//}

Properties props;
Level level;

int selfPlayer;

Tile Tiles[40][30];
std::vector<ipair> TilesByType[5];

Rect GetUnitRect( const Unit & u ) {
	return Rect( Vec2( u.position.x - u.size.x * 0.5, u.position.y ), Vec2( u.position.x + u.size.x * 0.5, u.position.y + u.size.y ) );
}

//bool IsOnTile( const Rect & rect, Tile tile ) {
//	for (int i = 0; i < 40; i++) {
//		for (int j = 0; j < 30; j++) {
//			Tile t = level.tiles[i][j];
//			if (t == tile) {
//				Vec2 c( i, j );
//				Rect r( c, c + Vec2( 1, 1 ) );
//				if (rect.Intersects( r )) return true;
//			}
//		}
//	}
//	return false;
//}

bool IsOnTile( const Rect & rect, Tile tile ) {
	for (ipair v : TilesByType[tile]) {
		Vec2 c( v.first, v.second );
		Rect r( c, c + Vec2( 1, 1 ) );
		if (rect.Intersects( r )) return true;
	}
	if (tile == WALL) {
		if (rect.Intersects( Rect( 0,0,1,30))) return true;
		if (rect.Intersects( Rect( 0,0,40,1))) return true;
		if (rect.Intersects( Rect( 39,0,40,30))) return true;
		if (rect.Intersects( Rect( 0,29,40,30))) return true;
	}
	return false;
}

//Tile GetTileAt( const Vec2 & p ) {
//	int x = (int)floor( p.x );
//	int y = (int)floor( p.y );
//	return level.tiles[x][y];
//}

Tile GetTileAt( const Vec2 & p ) {
	int x = (int)floor( p.x );
	int y = (int)floor( p.y );
	return Tiles[x][y];
}

void InitStrat( const Game &game, const Unit & self ) {
	if (game.currentTick != 0) return;

	props = game.properties;
	level = game.level;

	selfPlayer = self.playerId;

	for (int i = 0; i < 40; i++) {
		for (int j = 0; j < 30; j++) {
			Tile tile = level.tiles[i][j];
			Tiles[i][j] = tile;
			if (tile != WALL || (i != 0 && j != 0 && i != 39 && j != 29 ) ) {
				TilesByType[tile].emplace_back( ipair( i, j ) );
			}
		}
	}
}

class Sim {
public:

	//class Unit {
	//public:
	//	int playerId;
	//	int id;
	//	int health;
	//	Vec2 position;
	//	Vec2 size;
	//	bool onGround;
	//	bool onLadder;
	//	JumpState jumpState;
	//	UnitAction action;

	//	//float jumpTime = 0;
	//	int jumpTicks = 0;

	//	Unit( const ::Unit & u ) {
	//		playerId = u.playerId;
	//		id = u.id;
	//		health = u.health;
	//		position = fv2d( u.position );
	//		size = fv2d( u.size );
	//		onGround = u.onGround;
	//		onLadder = u.onLadder;
	//		jumpState = u.jumpState;
	//	}
	//};

	//class Bullet {
	//public:
	//	WeaponType weaponType;
	//	int unitId;
	//	int playerId;
	//	Vec2 position;
	//	Vec2 velocity;
	//	int damage;
	//	double size;
	//	bool explosive;
	//	float explRange;
	//	int explDamage;

	//	Bullet( const ::Bullet & b ) {
	//		weaponType = b.weaponType;
	//		unitId = b.unitId;
	//		playerId = b.playerId;
	//		position = fv2d( b.position );
	//		velocity = fv2d( b.velocity );
	//		damage = b.damage;
	//		size = b.size;
	//	}
	//};

	int currentTick;
	//std::vector<Player> players;
	std::vector<::Unit> units;
	std::vector<::Bullet> bullets;
	std::vector<::Mine> mines;
	std::vector<::LootBox> lootBoxes;

	int microticks = 100;

	Sim() {
	}

	Vec2 TraceBullet( const Bullet & b ) const {

	}

	void AdjustAim( Unit & u ) {

	}

	bool MoveUnit( Unit & u ) {
		UnitAction a = u.action;

		if (currentTick == 80) {
			int _bp = 0;
		}

		float delta = 1.0 / (props.ticksPerSecond * microticks);
		float moveDelta = props.unitMaxHorizontalSpeed * delta;

		Tile feetTile1 = GetTileAt( u.position + Vec2( -u.size.x * 0.5, -moveDelta ) );
		Tile feetTile2 = GetTileAt( u.position + Vec2( u.size.x * 0.5, -moveDelta ) );
		//Tile headTile = GetTileAt( u.position + Vec2( 0, u.size.y + moveDelta ) );

		Rect r = GetUnitRect( u );

		bool onWall = (feetTile1 == WALL || feetTile2 == WALL);
		bool onLadder = GetTileAt( u.position + Vec2( 0, -moveDelta ) ) == LADDER || GetTileAt( u.position + Vec2( 0, u.size.y ) ) == LADDER;
		bool underPlatform = (GetTileAt( u.position + Vec2( -u.size.x * 0.5, 0 ) ) == PLATFORM || GetTileAt( u.position + Vec2( u.size.x * 0.5, 0 ) ) == PLATFORM);
		bool onPlatform = (feetTile1 == PLATFORM || feetTile2 == PLATFORM) && !onWall && !onLadder && !underPlatform;
		//bool inAir = (feetTile1 == EMPTY && feetTile2 == EMPTY);
		//bool inAir = ((feetTile1 == EMPTY || feetTile1 == LADDER ) && (feetTile2 == EMPTY || feetTile2 == LADDER));
		bool inAir = !onWall && !onPlatform && !onLadder;
		bool isBumped = GetTileAt( u.position + Vec2( -u.size.x * 0.5, u.size.y + moveDelta ) ) == WALL || GetTileAt( u.position + Vec2( u.size.x * 0.5, u.size.y + moveDelta ) ) == WALL;
		bool touchJumppad = IsOnTile( r, JUMP_PAD );

		u.onLadder = onLadder;
		u.onGround = !inAir;

		//bool touchLadder = IsOnTile( r, LADDER );

		//if (onPlatform) {
		//	onPlatform = false;
		//	inAir = true;
		//}

		bool special = false;

		//Vec2 offset;
		float ox = a.velocity * delta;
		float oy = 0;

		//u.jumpState.canJump = (!inAir || onLadder) /*&& !u.jumpTicks*/;
		//u.jumpState.canJump = (!inAir && !u.jumpTicks) || onLadder;
		u.jumpState.canJump = !inAir;

		bool cancelJump = false;

		if (touchJumppad) {
			u.jumpState.canCancel = false;
			u.jumpState.speed = props.jumpPadJumpSpeed;
			u.jumpState.maxTime = props.jumpPadJumpTime;
			u.jumpTicks = u.jumpState.maxTime * (60 * microticks);
			special = true;
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
						u.jumpTicks = u.jumpState.maxTime * (60 * microticks);
						//u.jumpState.canJump = false;
					}
				}
			}
			else if (a.jumpDown) {	
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

		if (isBumped) {
			cancelJump = true;
		}

		if (cancelJump) {
			u.jumpState.speed = -props.unitFallSpeed;
			u.jumpState.maxTime = 0;
			u.jumpState.canCancel = true;
			//u.jumpTicks = 0;
		}

		if (u.jumpTicks) {
			oy = u.jumpState.speed;
			u.jumpTicks--;
		}
		else {
			if (inAir && !onLadder ) {
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
			special = true;
		}

		test = r;
		r += Vec2( ox, 0 );

		if (IsOnTile( r, WALL )) {
			ox = 0;
			special = true;
		}

		u.position.x += ox;
		u.position.y += oy;

		return special;
	}

	vint UnitsInRect( Rect rect, int excludeId = -1 ) const {
		vint res;
		for (int i = 0; i < units.size(); i++) {
			if (rect.Intersects( GetUnitRect( units[i] ) ) && units[i].id != excludeId ) {
				res.emplace_back( i );
			}
		}
		return res;
	}

	void MoveBullet( Bullet & b ) {
		float delta = 1.0 / (props.ticksPerSecond * microticks);
		b.position += b.velocity * delta;

		Rect r( b.position - Vec2( b.size*0.5, b.size*0.5 ), b.position + Vec2( b.size*0.5, b.size*0.5 ) );

		vint hit = UnitsInRect( r, b.unitId );

		if (IsOnTile( r, WALL ) || !hit.empty() ) {
			if (b.explosionParams) {
				hit = UnitsInRect( Rect( b.position, b.explosionParams->radius ) );
				for (int i : hit) {
					Unit & u = units[i];
					u.health -= b.explosionParams->damage;
				}
			}
			else {
				if (!units.empty()) {
					Unit & u = units[hit[0]];
					u.health -= b.damage;
				}
			}
			stdh::erase( bullets, b );
		}
	}

	void SetState( const Game & g ) {
		currentTick = g.currentTick;
		//players = g.players;
		bullets = g.bullets;
		mines = g.mines;
		lootBoxes = g.lootBoxes;

		for (const ::Unit & u : g.units) {
			units.emplace_back( u );
		}

		//std::sort( units.begin(), units.end(), []( const Unit & a, const Unit & b ) { return a.id < b.id;  } );
	}

	void Microtick() {
		for (Unit & u : units) {
			MoveUnit( u );
		}
		for (Bullet & b : bullets) {
			MoveBullet( b );
		}
	}

	//bool MicrotickSmart() {
	//	bool special = false;
	//	for (Unit & u : units) {
	//		Unit b = u;
	//		if (MoveUnit( u )) {
	//			u = b;

	//		}
	//	}
	//	return special;
	//}

	void Tick() {
		for (int i = 0; i < microticks; i++) {
			Microtick();
		}
		currentTick++;
	}

	//void TickSmart() {
	//	for (int i = 0; i < props.updatesPerTick; i++) {
	//		Microtick();
	//	}
	//	currentTick++;
	//}

	void Simulate( int numTicks ) {
		for (int i = 0; i < numTicks; i++) {
			Tick();
		}
	}

};
Sim sim;

UnitAction GetAction( int dir ) {
	UnitAction a;
	a.velocity = 0;
	a.jump = false;
	a.jumpDown = false;
	if(dir == 2 || dir == 3 || dir == 4 ) a.velocity = 10;
	else if(dir == 6 || dir == 7 || dir == 8) a.velocity = -10;
	a.jump = dir == 8 || dir == 1 || dir == 2;
	a.jumpDown = dir == 6 || dir == 5 || dir == 4;
	a.plantMine = false;
	a.swapWeapon = false;
	a.shoot = false;
	return a;
}

typedef std::pair<int, UnitAction> scoreAction;

int GetScore( const Sim & s ) {
	return s.units[0].health;
}

UnitAction Think( const Unit &unit, const Game &game ) {
	sim.currentTick = game.currentTick;
	sim.bullets = game.bullets;
	sim.units.resize( 1 );
	sim.units[0] = unit;

	std::vector<scoreAction> actions;

	for (int i = 0; i < 9; i++) {
		Sim sim2 = sim;
		UnitAction a = GetAction( i );
		sim.units[0].action = a;
		sim2.Simulate( 60 );
		actions.emplace_back( GetScore( sim2 ), a );
	}

	std::sort( actions.begin(), actions.end(), []( const scoreAction & a, const scoreAction & b ) { return a.first < b.first; } );

	return unit.health > actions.front().first?actions.back().second:UnitAction();
}

UnitAction GetTestCommand( const Unit &unit, int tick ) {
	UnitAction a;
	a.velocity = 0;
	a.jump = false;
	a.jumpDown = tick==0?true:false;
	a.plantMine = false;
	a.swapWeapon = false;
	a.shoot = false;
	//if (unit.onLadder) a.jumpDown = true;
	a.jumpDown = true;
	return a;
}

std::vector<CustomData::Rect> drawpath;
void GenTestPath( const Unit &unit, const Game &game ) {
//	Sim sim;
	//level = game.level;
	//props = game.properties;
	sim.currentTick = game.currentTick;
	sim.units.emplace_back( unit );
	//sim.units[0].action.velocity = -10;
	//sim.units[0].action.jump = true;
	//sim.units[0].action.jumpDown = false;
	//sim.units[0].action.shoot = false;
	//sim.units[0].action.plantMine = false;
	//sim.units[0].action.swapWeapon = false;

	for (int i = 0; i < 5 * 60; i++) {
		sim.units[0].action = GetTestCommand(unit,sim.currentTick);
		drawpath.emplace_back( CustomData::Rect( Vec2Float( sim.units[0].position.x, sim.units[0].position.y), Vec2Float( 0.1, 0.1 ), ColorFloat( 1, 1, 1, 1 )) );
		sim.Tick();
	}

}

UnitAction DebugAction( const Unit &unit, const Game &game, Debug &debug ) {
	if (game.currentTick == 0) {
		GenTestPath( unit, game );
	}
	for (CustomData::Rect r : drawpath) {
		debug.draw( r );
	}

	debug.draw( CustomData::Log( std::string( "Touch jumppad: " ) + std::to_string( IsOnTile( GetUnitRect(unit), JUMP_PAD ) ) ) );



	return GetTestCommand(unit,game.currentTick);
}

UnitAction MyStrategy::getAction(const Unit &unit, const Game &game, Debug &debug) {
	InitStrat( game, unit );
	return DebugAction( unit, game, debug );
}