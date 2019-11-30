#include "MyStrategy.hpp"
#include "Helpers.h"

using namespace std;

MyStrategy::MyStrategy() {}

double distanceSqr(Vec2 a, Vec2 b) {
  return (a.x - b.x) * (a.x - b.x) + (a.y - b.y) * (a.y - b.y);
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

pair<bool,Vec2> TraceRect( Rect r, Vec2 dir, Rect target = Rect( 0, 0, 0, 0 ) ) {
	while (!IsOnTile( r, WALL )) {
		if(r.Intersects( target )) return pair<bool, Vec2>( true, r.Center() );
		if( !Rect(0,0,40,30).Contains(r.Center()) ) return pair<bool, Vec2>( true, r.Center() );
		r += dir * 0.1;
	}
	return pair<bool, Vec2>(false,r.Center());
}

float HitChance( Rect target, Vec2 from, Vec2 dir, float radius, float spread, float explRange ) {
	Vec2 l = dir.Rotate( spread ).Normalized();
	Vec2 r = dir.Rotate( -spread ).Normalized();
	for (int i = 0; i < 20; i++) {
		Vec2 d = l.Slerped( i / 20., r );
		TraceRect( Rect( from, radius ), d, target );
	}
	return 0;
}

class Sim {
public:

	int currentTick;
	//std::vector<Player> players;
	std::vector<::Unit> units;
	std::vector<::Bullet> bullets;
	std::vector<::Mine> mines;
	std::vector<::LootBox> lootBoxes;

	int microticks = 1;

	Sim() {
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


	void AdjustAim( Unit & u ) {

		if (!u.weapon) return;

		float delta = 1.0 / (props.ticksPerSecond * microticks);

		WeaponParams params = props.weaponParams[u.weapon->type];
		u.weapon->spread -= params.aimSpeed * delta;

		u.weapon->spread = stdh::minmax( u.weapon->spread, params.minSpread, params.maxSpread );
	}

	void PickLoot( Unit & u ) {
		for (LootBox & lb : lootBoxes) {
			if (GetUnitRect( u ).Intersects( Rect( lb.position, props.lootBoxSize ) )) {
				switch (lb.item->type) {
				case 0:
					if (u.health < props.unitMaxHealth) {
						u.health = max( u.health + props.healthPackHealth, props.unitMaxHealth );
						stdh::erase( lootBoxes, lb, stdh::pointer_equal<LootBox> );
					}
					break;
				case 1:
					if (u.action.swapWeapon) {

					}
					break;
				case 2:
					u.mines += 1;
					break;
				}
			}
		}
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

		if (IsOnTile( r, WALL ) || !UnitsInRect( r, u.id).empty() ) {
			oy = 0;
			special = true;
		}

		test = r;
		r += Vec2( ox, 0 );

		if (IsOnTile( r, WALL ) || !UnitsInRect( r, u.id ).empty() ) {
			ox = 0;
			special = true;
		}

		u.position.x += ox;
		u.position.y += oy;

		return special;
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
				if (!hit.empty()) {
					Unit & u = units[hit[0]];
					u.health -= b.damage;
				}
			}
			stdh::erase( bullets, b );
		}
	}

	void UpdateMine( Mine & m ) {

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

	void Tick() {
		for (int i = 0; i < microticks; i++) {
			Microtick();
		}
		currentTick++;
	}

	void Simulate( int numTicks ) {
		for (int i = 0; i < numTicks; i++) {
			Tick();
		}
	}

};

int GetScore( const Unit & u, const Sim & s ) {
	return u.health;
}

struct AiAction {
	int id;
	int ticks;
};

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


bool IsBetterWeapon( WeaponType current, WeaponType newweapon ) {
	return newweapon > current;
}

pair<bool,UnitAction> Dodge( const Unit &unit, const Game &game, Debug &debug ) {
	Sim sim;
	sim.currentTick = game.currentTick;
	sim.bullets = game.bullets;
	sim.units.resize( 1 );
	sim.units[0] = unit;

	std::vector<scoreAction> actions;

	for (int i = 0; i < 9; i++) {
		Sim sim2 = sim;
		UnitAction a = GetAction( i );
		sim2.units[0].action = a;
		sim2.Simulate( 100 );
		actions.emplace_back( sim2.units[0].health, a );
	}

	std::sort( actions.begin(), actions.end(), []( const scoreAction & a, const scoreAction & b ) { return a.first < b.first; } );

	if (!game.bullets.empty()) {
		int bp = 0;
	}

	return pair<bool, UnitAction>( unit.health > actions.front().first, actions.back().second );
}

UnitAction Quickstart( const Unit &unit, const Game &game, Debug &debug ) {

	const Unit *nearestEnemy = nullptr;
	double enemyDist = 10E10;
	for (const Unit &other : game.units) {
		if (other.playerId != unit.playerId) {
			double dist = distanceSqr( unit.position, other.position );
			if (nearestEnemy == nullptr || dist < enemyDist) {
				enemyDist = dist;
				nearestEnemy = &other;
			}
		}
	}

	const LootBox *nearestWeapon = nullptr;
	const LootBox *nearestHealth = nullptr;
	bool betterWeapon = false;
	double weaponDist = 10E10;
	for (const LootBox &lootBox : game.lootBoxes) {
		if (lootBox.item->type == 1) {
			Item::Weapon weap = *std::dynamic_pointer_cast<Item::Weapon>(lootBox.item);
			betterWeapon = !unit.weapon || (unit.weapon && IsBetterWeapon( unit.weapon->type, weap.weaponType ));
			double dist = distanceSqr( unit.position, lootBox.position );
			if (betterWeapon && dist < weaponDist) {
				weaponDist = dist;
				nearestWeapon = &lootBox;
			}
		}
		if (lootBox.item->type == 0) {
			if (nearestHealth == nullptr || distanceSqr( unit.position, lootBox.position ) < distanceSqr( unit.position, nearestHealth->position )) {
				nearestHealth = &lootBox;
			}
		}
	}

	bool enemy = false;

	Vec2Double targetPos = unit.position;
	if ( unit.weapon == nullptr && nearestWeapon != nullptr) {
		targetPos = nearestWeapon->position;
	}
	else if (unit.health < props.unitMaxHealth && nearestHealth != nullptr) {
		targetPos = nearestHealth->position;
	}
	else if (betterWeapon && nearestWeapon != nullptr) {
		targetPos = nearestWeapon->position;
	}
	else if ( sqrt(enemyDist) > 15 ) {
		enemy = true;
		targetPos = nearestEnemy->position;
	}

	int tile = game.level.tiles[floor( unit.position.x )][floor( unit.position.y - 0.1 )];

	Vec2Double aim = Vec2Double( 0, 0 );
	if (nearestEnemy != nullptr) {
		aim = Vec2Double( nearestEnemy->position.x - unit.position.x, nearestEnemy->position.y - unit.position.y );
	}

	bool jump = targetPos.y > unit.position.y;
	if (targetPos.x > unit.position.x && game.level.tiles[size_t( unit.position.x + 1 )][size_t( unit.position.y )] ==	Tile::WALL) {
		jump = true;
	}
	if (targetPos.x < unit.position.x && game.level.tiles[size_t( unit.position.x - 1 )][size_t( unit.position.y )] ==	Tile::WALL) {
		jump = true;
	}

	UnitAction action;
	//action.velocity = !(targetPos == unit.position)? Sign(targetPos.x - unit.position.x) * 10:action.velocity=0;
	action.velocity = Sign(targetPos.x - unit.position.x,0.1) * 10;
	action.jump = jump;
	action.jumpDown = !action.jump;
	action.aim = aim;
	action.shoot = true;
	action.swapWeapon = false;
	action.plantMine = false;

	if (unit.weapon && unit.weapon->type == ROCKET_LAUNCHER && enemyDist > unit.weapon->params.explosion->radius*2 && TraceRect( Rect( unit.position + Vec2(0,unit.size.y*0.5), unit.weapon->params.bullet.size ), aim ).second.Dist(unit.position) < unit.weapon->params.explosion->radius+0.5  ) {
		action.shoot = false;
	}

	if (sqrt(weaponDist) < 1.5 && betterWeapon) {
		action.swapWeapon = true;
	}

	pair<bool, UnitAction> dodge = Dodge( unit, game, debug );

	if (dodge.first) {
		action.jump = dodge.second.jump;
		action.jumpDown = dodge.second.jumpDown;
		action.velocity = dodge.second.velocity;
	}

	debug.draw( CustomData::Log( std::string( "Target pos: " ) + targetPos.toString() ) );
	debug.draw( CustomData::Log( std::string( "enemydist: " ) + std::to_string( sqrt(enemyDist) ) ) );
	debug.draw( CustomData::Log( std::string( "enemy: " ) + std::to_string( enemy ) ) );
	debug.draw( CustomData::Log( std::string( "dodge: " ) + std::to_string( dodge.first ) ) );

	return action;
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
	Sim sim;
	sim.currentTick = game.currentTick;
	sim.units.emplace_back( unit );

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
	//return DebugAction( unit, game, debug );
	return Quickstart( unit, game,debug );
}