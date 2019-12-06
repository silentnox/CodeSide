#include "MyStrategy.hpp"
#include "Helpers.h"

using namespace std;

MyStrategy::MyStrategy() {}

double distanceSqr(Vec2 a, Vec2 b) {
  return (a.x - b.x) * (a.x - b.x) + (a.y - b.y) * (a.y - b.y);
}

Properties props;
Level level;

int selfPlayer;

Tile Tiles[40][30];
Rect TileRects[40][30];
std::vector<Rect> TilesByType[5];

inline Rect GetUnitRect( const Unit & u ) {
	return Rect( Vec2( u.position.x - u.size.x * 0.5, u.position.y ), Vec2( u.position.x + u.size.x * 0.5, u.position.y + u.size.y ) );
}

//inline bool IsOnTile( const Rect & rect, Tile tile ) {
//	for (const Rect & r : TilesByType[tile]) {
//		//Vec2 c( v.first, v.second );
//		//Rect r( c, c + Vec2( 1, 1 ) );
//		if (rect.Intersects( r )) return true;
//	}
//	//if (tile == WALL) {
//	//	if (rect.Intersects( Rect( 0,0,1,30))) return true;
//	//	if (rect.Intersects( Rect( 0,0,40,1))) return true;
//	//	if (rect.Intersects( Rect( 39,0,40,30))) return true;
//	//	if (rect.Intersects( Rect( 0,29,40,30))) return true;
//	//}
//	return false;
//}

inline bool IsOnTile( const Rect & rect, Tile tile ) {
	int minx = (int)floor( rect.Min.x );
	int miny = (int)floor( rect.Min.y );
	int maxx = (int)floor( rect.Max.x );
	int maxy = (int)floor( rect.Max.y );
	for (int i = minx; i <= maxx; i++) {
		for (int j = miny; j <= maxy; j++) {
			if (Tiles[i][j] == tile && TileRects[i][j].Intersects( rect )) return true;
		}
	}
	return false;
}

inline Tile GetTileAt( const Vec2 & p ) {
	int x = (int)floor( p.x );
	int y = (int)floor( p.y );
	return Tiles[x][y];
}

void InitStrat( const Game &game, const Unit & self ) {
	if (game.currentTick != 0) return;

	props = game.properties;
	level = game.level;

	selfPlayer = self.playerId;

	TilesByType[0].reserve( 100 );
	TilesByType[1].reserve( 100 );
	TilesByType[2].reserve( 100 );
	TilesByType[3].reserve( 100 );
	TilesByType[4].reserve( 100 );

	for (int i = 0; i < 40; i++) {
		for (int j = 0; j < 30; j++) {
			Tile tile = level.tiles[i][j];
			Tiles[i][j] = tile;
			TileRects[i][j] = Rect( i, j, i + 1, j + 1 );
			if (tile != WALL || (i != 0 && j != 0 && i != 39 && j != 29 ) ) {
				//TilesByType[tile].emplace_back( ipair( i, j ) );
				TilesByType[tile].emplace_back( Rect( i,j,i+1.,j+1. ) );
			}
		}
	}

	TilesByType[WALL].emplace_back( Rect( 0, 0, 1, 30 ) );
	TilesByType[WALL].emplace_back( Rect( 0, 0, 40, 1 ) );
	TilesByType[WALL].emplace_back( Rect( 39, 0, 40, 30 ) );
	TilesByType[WALL].emplace_back( Rect( 0, 29, 40, 30 ) );
}

pair<bool,Vec2> TraceRect( Rect r, Vec2 dir, Rect target = Rect( 0, 0, 0, 0 ) ) {
	dir = dir.Normalized();
	double delta = 0.05;
	double offset = 0;
	while (!IsOnTile( r, WALL )) {
		if(!target.IsZero() && r.Intersects( target )) return pair<bool, Vec2>( true, r.Center() );
		if( !Rect(0,0,40,30).Contains(r.Center()) ) return pair<bool, Vec2>( false, r.Center() );
		r += dir * delta;
		offset += delta;
	}
	return pair<bool, Vec2>(false,r.Center());
}

bv2pair RaycastLevel( Vec2 from, Vec2 dir, Rect target = Rect( 0, 0, 0, 0 ) ) {
	dir = dir.Normalized();
	double mindist = DBL_MAX;
	for (Rect r : TilesByType[WALL]) {
		bfpair res = r.Raycast( from, dir );
		if (res.first) {
			if (res.second < mindist) {
				mindist = res.second;
			}
		}
	}
	if (!target.IsZero()) {
		bfpair res = target.Raycast( from, dir );
		if (res.first && res.second < mindist) {
			return bv2pair( true, from + dir*res.second );
		}
	}
	return bv2pair(false,from + dir*mindist);
}

double HitChance( Rect target, Vec2 from, Vec2 dir, const Weapon & weapon, int numRays = 30 ) {
	double spread = weapon.spread;

	dir = dir.Normalized();

	Vec2 l = dir.Rotate( spread ).Normalized();
	Vec2 r = dir.Rotate( -spread ).Normalized();

	int hits = 0;

	for (int i = 0; i < numRays; i++) {
		Vec2 d = l.Slerped( i / (double)numRays, r );
		pair<bool, Vec2> res;
		//pair<bool, Vec2> res = TraceRect( Rect( from, weapon.params.bullet.size ), d, target );
		if (i == 0) {
			//res = TraceRect( Rect( from, weapon.params.bullet.size ), d, target );
			res = RaycastLevel( from + d.Perp() * weapon.params.bullet.size, d, target );
		}
		else if (i == numRays - 1) {
			res = RaycastLevel( from - d.Perp() * weapon.params.bullet.size, d, target );
		}
		else {
			res = RaycastLevel( from, d, target );
		}
		if (res.first) {
			hits++;
			continue;
		}
		else {
			if (weapon.params.explosion) {
				Rect expl( res.second - d * weapon.params.explosion->radius, weapon.params.explosion->radius );
				if (i == 0) expl += -d.Perp() * weapon.params.bullet.size;
				else if (i == numRays-1) expl += d.Perp() * weapon.params.bullet.size;
				if (expl.Intersects( target )) hits++;
			}
		}
	}
	return hits/(double)numRays;
}

pair<double, const Unit*> NearestUnit( Vec2 from, const vector<Unit> & items, bool enemy, bool raycast = false ) {
	const Unit * closest = nullptr;
	double mindist = DBL_MAX;
	for (const Unit & it : items) {
		if (enemy && it.playerId == selfPlayer) continue;
		if (raycast && RaycastLevel( from, from.DirTo( it.position + Vec2( 0, it.size.y*0.5 ) ), GetUnitRect(it) ).first == false) continue;
		double dist = from.Dist2( it.position + Vec2(0,it.size.y*0.5) );
		if (dist < mindist) {
			mindist = dist;
			closest = &it;
		}
	}
	return pair<double,const Unit*>( sqrt( mindist ),closest);
}

pair< double, const LootBox*> NearestLootbox( Vec2 from, const vector<LootBox> & items, int type, int weapType = -1, bool raycast = false ) {
	const LootBox * closest = nullptr;
	double mindist = DBL_MAX;
	for (const LootBox & it : items) {
		if (!it.item || it.item->type != type) continue;
		if (it.item->type == 1 && weapType != -1 && it.item->weaponType != weapType) continue;
		Vec2 center = it.position + Vec2( 0, it.size.y*0.5 );
		if (raycast && RaycastLevel( from, from.DirTo( center ), Rect(center- it.size*0.5, center+ it.size*0.5) ).first == false) continue;
		double dist = from.Dist2( it.position );
		if (dist < mindist) {
			mindist = dist;
			closest = &it;
		}
	}
	return pair<double, const LootBox*>( sqrt(mindist), closest );
}

pair< double, const Mine*> NearestMine( Vec2 from, const vector<Mine> & items, int type, bool raycast = false ) {
	const Mine * closest = nullptr;
	double mindist = DBL_MAX;
	for (const Mine & it : items) {
		Vec2 center = it.position + Vec2( 0, it.size.y*0.5 );
		if (raycast && RaycastLevel( from, from.DirTo( center ), Rect( center - it.size*0.5, center + it.size*0.5 ) ).first == false) continue;
		double dist = from.Dist2( it.position );
		if (dist < mindist) {
			mindist = dist;
			closest = &it;
		}
	}
	return pair<double, const Mine*>( sqrt( mindist ), closest );
}

class Sim {
public:

	int currentTick = 0;
	std::vector<::Unit> units;
	std::vector<::Bullet> bullets;
	std::vector<::Mine> mines;
	std::vector<::LootBox> lootBoxes;

private:
	int microticks;
	double deltaTime;
public:

	bool useAim = false;

	Sim( int num = 100 ) {
		microticks = num;
		deltaTime = 1. / (60. * (double)microticks);
	}

	
	inline vint UnitsInRect( Rect rect, int excludeId = -1 ) const {
		vint res;
		for (int i = 0; i < units.size(); i++) {
			if (rect.Intersects( GetUnitRect( units[i] ) ) && units[i].id != excludeId ) {
				res.emplace_back( i );
			}
		}
		return res;
	}

	inline bool HasUnitsInRect( Rect rect, int excludeId = -1 ) const {
		for (int i = 0; i < units.size(); i++) {
			if (units[i].id != excludeId && rect.Intersects( GetUnitRect( units[i] ) )) {
				return true;
			}
		}
		return false;
	}


	void AdjustAim( Unit & u ) {

		if (!u.weapon || !useAim ) return;

		//double delta = 1.0 / (props.ticksPerSecond * microticks);

		if (u.weapon->fireTimer > 0) {
			u.weapon->fireTimer -= deltaTime;
		}
		else {
			if (!u.weapon->magazine) {
				u.weapon->magazine = u.weapon->params.magazineSize;
			}
			if (u.action.shoot) {
				u.weapon->magazine -= 1;
				u.weapon->fireTimer = u.weapon->magazine > 0 ? u.weapon->params.fireRate : u.weapon->params.reloadTime;
				u.weapon->spread += u.weapon->params.recoil;
			}
		}

		Vec2 aim = u.action.aim.Normalized();

		//double angle = Vec2( 1, 0 ).Angle( u.action.aim.Normalized() );
		double angle = atan2( aim.x, aim.y );
		double angleDelta = abs( angle - u.weapon->lastAngle );

		u.weapon->spread += angleDelta;
		u.weapon->lastAngle = angle;

		//u.weapon->lastAngle = abs( u.prevAim.Angle( u.action.aim ) );
		//u.prevAim = u.action.aim;

		u.weapon->spread -= u.weapon->params.aimSpeed * deltaTime;

		u.weapon->spread = stdh::minmax( u.weapon->spread, u.weapon->params.minSpread, u.weapon->params.maxSpread );
	}

	void PickLoot( Unit & u ) {
		for (LootBox & lb : lootBoxes) {
			if (GetUnitRect( u ).Intersects( Rect( lb.position, lb.size ) )) {
				switch (lb.item->type) {
				case 0:
					if (u.health < props.unitMaxHealth) {
						int newHealth = max( u.health + props.healthPackHealth, props.unitMaxHealth );
						u.numHealed += newHealth - u.health;
						u.health = newHealth;
						stdh::erase( lootBoxes, lb, stdh::pointer_equal<LootBox> );
					}
					break;
				case 1:
					if (u.action.swapWeapon) {
						if (!u.weapon) {
							u.weapon = std::shared_ptr<Weapon>( new Weapon() );
						}
						u.weapon->type = lb.item->weaponType;
						u.weapon->params = props.weaponParams[u.weapon->type];
						stdh::erase( lootBoxes, lb, stdh::pointer_equal<LootBox> );
					}
					break;
				case 2:
					u.mines += 1;
					stdh::erase( lootBoxes, lb, stdh::pointer_equal<LootBox> );
					break;
				}
			}
		}
	}

	void MoveUnit( Unit & u ) {
		//double delta = 1.0 / (props.ticksPerSecond * microticks);
		double moveDelta = props.unitMaxHorizontalSpeed * deltaTime;

		Tile feetTile1 = GetTileAt( u.position + Vec2( -u.size.x * 0.5, -moveDelta ) );
		Tile feetTile2 = GetTileAt( u.position + Vec2( u.size.x * 0.5, -moveDelta ) );
		Tile headTile1 = GetTileAt( u.position + Vec2( -u.size.x * 0.5, u.size.y + moveDelta ) );
		Tile headTile2 = GetTileAt( u.position + Vec2( u.size.x * 0.5, u.size.y + moveDelta ) );

		const Rect r = GetUnitRect( u );

		bool onWall = (feetTile1 == WALL || feetTile2 == WALL);
		bool onLadder = GetTileAt( u.position + Vec2( 0, -moveDelta ) ) == LADDER || GetTileAt( u.position + Vec2( 0, u.size.y * 0.5 ) ) == LADDER;
		bool underPlatform = (GetTileAt( u.position + Vec2( -u.size.x * 0.5, 0 ) ) == PLATFORM || GetTileAt( u.position + Vec2( u.size.x * 0.5, 0 ) ) == PLATFORM);
		bool onPlatform = (feetTile1 == PLATFORM || feetTile2 == PLATFORM) && !onWall && !onLadder && !underPlatform && u.jumpState.maxTime <= 0 + DBL_EPSILON;
		bool inAir = !onWall && !onPlatform && !onLadder && !HasUnitsInRect( r + Vec2( 0, -moveDelta ), u.id );
		bool isBumped = headTile1 == WALL || headTile2 == WALL || HasUnitsInRect( r + Vec2(0,moveDelta) , u.id );
		bool touchJumppad = IsOnTile( r, JUMP_PAD );

		u.onLadder = onLadder && !onWall;
		u.onGround = !inAir;

		double ox = u.action.velocity * deltaTime;
		double oy = 0;

		u.jumpState.canJump = !inAir;

		bool cancelJump = false;

		if (touchJumppad) {
			u.jumpState.canCancel = false;
			u.jumpState.speed = props.jumpPadJumpSpeed;
			u.jumpState.maxTime = props.jumpPadJumpTime;
		}
		else {
			if (u.action.jump) {
				if (onLadder) {
					oy = props.unitJumpSpeed;
				}
				else {
					if (u.jumpState.canJump) {
						u.jumpState.canCancel = true;
						u.jumpState.speed = props.unitJumpSpeed;
						u.jumpState.maxTime = props.unitJumpTime;
					}
				}
			}
			else if (u.action.jumpDown) {	
				if (onLadder || onPlatform) {
					oy = -props.unitFallSpeed;
				}
				else {
					cancelJump = true;
				}
			}
		}

		if (!u.action.jump) {
			cancelJump = true;
		}

		if (isBumped || (cancelJump && u.jumpState.canCancel)) {
			u.jumpState.speed = 0;
			u.jumpState.maxTime = 0;
			u.jumpState.canCancel = true;
		}

		if (u.jumpState.maxTime > 0) {
			oy = u.jumpState.speed;
			u.jumpState.maxTime -= deltaTime;
		}
		else {
			if (inAir) {
				oy = -props.unitFallSpeed;
			}
			u.jumpState.maxTime = 0;
		}

		oy *= deltaTime;

		if (oy != 0) {
			Rect test = r + Vec2( 0, oy );

			if (IsOnTile( test, WALL ) || HasUnitsInRect( test, u.id )) {
				oy = 0;
			}
		}

		if (ox != 0) {
			Rect test = r + Vec2( ox, 0 );

			if (IsOnTile( test, WALL ) || HasUnitsInRect( test, u.id )) {
				ox = 0;
			}
		}

		u.position.x += ox;
		u.position.y += oy;
	}

	void MoveBullet( Bullet & b ) {
		//double delta = 1.0 / (props.ticksPerSecond * microticks);

		Rect r( b.position, b.size );

		Vec2 pos = b.position;
		b.position += b.velocity * deltaTime;

		vint hit = UnitsInRect( r, b.unitId );

		if (IsOnTile( r, WALL ) || !hit.empty() ) {
			if (!hit.empty()) {
				Unit & u = units[hit[0]];
				u.health -= b.damage;
			}
			if (b.explosionParams) {
				hit = UnitsInRect( Rect( pos, b.explosionParams->radius ) );
				for (int i : hit) {
					Unit & u = units[i];
					u.health -= b.explosionParams->damage;
				}
			}

			stdh::erase( bullets, b );
			return;
		}
	}

	void UpdateMine( Mine & m ) {
		//double delta = 1.0 / (props.ticksPerSecond * microticks);

		if (m.state == EXPLODED) return;

		if (m.state == PREPARING) {
			m.timer -= deltaTime;
			if (m.timer < 0) {
				m.state = MineState::IDLE;
			}
		}

		if (m.state == TRIGGERED) {
			m.timer -= deltaTime;
			if (m.timer < 0) {
				vint ids = UnitsInRect( Rect( m.position, m.explosionParams.radius ) );
				if (!ids.empty()) {
					for (int i : ids) {
						units[i].health -= m.explosionParams.damage;
					}
				}
				m.state = EXPLODED;
			}
			return;
		}

		if (m.state == MineState::IDLE) {
			vint ids = UnitsInRect( Rect( m.position, m.triggerRadius ) );
			if (!ids.empty()) {
				m.state = TRIGGERED;
				m.timer = props.mineTriggerTime;
			}
		}


	}

	void Microtick() {
		for (Unit & u : units) {
			MoveUnit( u );
			AdjustAim( u );
			PickLoot( u );
		}
		for (Bullet & b : bullets) {
			MoveBullet( b );
		}
		for (Mine & m : mines) {
			UpdateMine( m );
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

class Navigate {
public:

};

vector<Vec2> PredictPath( const Unit & u, const UnitAction & a, int ticks ) {
	Sim sim(1);
	vector<Vec2> path;
	path.reserve( ticks );
	sim.units.emplace_back( u );
	sim.units[0].action = a;
	for (int i = 0; i < ticks; i++) {
		sim.Tick();
		path.emplace_back( sim.units[0].position );
	}
	return path;
}

vector<Vec2> PredictPath2( int id, const UnitAction & a, int ticks, const Game & game ) {
	Sim sim( 1 );
	vector<Vec2> path;
	path.reserve( ticks );
	sim.units = game.units;
	sim.units[id].action = a;
	for (int i = 0; i < ticks; i++) {
		sim.Tick();
		path.emplace_back( sim.units[id].position );
	}
	return path;
}

vector<Vec2> PredictBullet( const Bullet & b, int ticks ) {
	Sim sim;
	//sim.microticks = 1;
	vector<Vec2> path;
	path.reserve( ticks );
	sim.bullets.emplace_back(b);
	for (int i = 0; i < ticks; i++) {
		sim.Tick();
		path.emplace_back( sim.bullets[0].position );
	}
	return path;
}

void DrawPath( const vector<Vec2> & path, Debug & debug, const ColorFloat color = ColorFloat(1,1,1,1) ) {
	Vec2 prev = path[0];
	for (const Vec2 & p : path) {
		debug.draw( CustomData::Line( p, prev, 0.1, color ) );
		prev = p;
	}
}

inline UnitAction GetAction( int dir ) {
	//UnitAction(double velocity, bool jump, bool jumpDown, Vec2Double aim, bool shoot, bool reload, bool swapWeapon, bool plantMine);
	switch (dir) {
	default:
	case 0: //stand
		return UnitAction( 0., false, false );
	case 1: // up 
		return UnitAction( 0., true, false );
	case 2: // right
		return UnitAction( 10., false, false );
	case 3: // down
		return UnitAction( 0., false, true );
	case 4: // left
		return UnitAction( -10., false, false );
	case 5: // up right
		return UnitAction( 10., true, false );
	case 6: // down right
		return UnitAction( 10., false, true );
	case 7: // down left
		return UnitAction( -10., false, true );
	case 8: // up left
		return UnitAction( -10., true, false );
	}
}

bool IsBetterWeapon( WeaponType current, WeaponType newweapon ) {
	int score[3];
	score[WeaponType::PISTOL] = 1;
	score[WeaponType::ASSAULT_RIFLE] = 2;
	score[WeaponType::ROCKET_LAUNCHER] = 3;
	return score[newweapon] > score[current];
}

typedef std::pair<int, UnitAction> scoreAction;

pair<bool, UnitAction> Dodge( int ticks, const Unit &unit, const Game &game, Debug &debug ) {
	Sim sim( 50 );
	sim.currentTick = game.currentTick;
	sim.bullets = game.bullets;
	sim.units.resize( 1 );
	sim.units[0] = unit;

	sim.mines = game.mines;

	std::vector<scoreAction> actions;
	actions.resize( 9 );

	bool dodge = false;

	for (int i = 0; i < 9; i++) {
		Sim sim2 = sim;
		UnitAction a = GetAction( i );
		sim2.units[0].action = a;
		sim2.Simulate( ticks );
		actions[i].first = sim2.units[0].health * 100;
		actions[i].second = a;
		if (sim2.units[0].health < unit.health) dodge = true;
	}

	UnitAction best = stdh::best( actions, []( const scoreAction & a, const scoreAction & b ) { return a.first > b.first; } ).second;

	return pair<bool, UnitAction>( dodge, best );
}

struct avoidScore {
	int action = 0;
	int health = 0;
	double hcOwn = 0.;
	double hcSelf = 0.;
	double hcEnemy = 0.;

	bool operator<( const avoidScore & other ) const {
		if (health < other.health) {
			return true;
		}
		else if (health == other.health) {
			return hcOwn - hcEnemy < other.hcOwn - other.hcEnemy;
		}
	}
};

pair<bool, UnitAction> Avoid( const Unit &unit, const Unit & enemy, Vec2 target, const Game &game, Debug &debug ) {
	int score[9];
	double hcEnemy[9];
	double hcOwn[9];

	int bestScore = -INT_MAX;
	int bestMove = -1;

	Sim sim( 50 );
	sim.units.resize( 2 );

	bool avoid = false;

	for (int i = 0; i < 9; i++) {
		sim.currentTick = game.currentTick;
		sim.mines = game.mines;
		sim.bullets = game.bullets;
		sim.lootBoxes = game.lootBoxes;
		sim.units[0] = unit;
		sim.units[1] = enemy;
		sim.units[0].action = GetAction( i );

		//sim.units[0].action.swapWeapon = true;

		sim.useAim = true;

		//sim.units[0].action.shoot = true;
		//sim.units[1].action.shoot = true;

		//sim.Simulate( 40 );
		hcOwn[i] = 0;
		hcEnemy[i] = 0;
		for (int tick = 0; tick < 100; tick++) {
			Vec2 dir = sim.units[0].position.DirTo( sim.units[1].position + Vec2( 0, 0.9 ) );
			Vec2 dir2 = sim.units[1].position.DirTo( sim.units[0].position + Vec2( 0, 0.9 ) );
			sim.units[0].action.aim = dir;
			sim.units[1].action.aim = dir2;
			sim.Tick();
			if (sim.units[0].weapon && sim.units[0].weapon->fireTimer <= 0 + DBL_EPSILON) {
				hcOwn[i] += HitChance( GetUnitRect( sim.units[1] ), sim.units[0].position + Vec2(0,0.9), dir, *sim.units[0].weapon, 10 );
			}
			if (sim.units[1].weapon && sim.units[1].weapon->fireTimer <= 0 + DBL_EPSILON) {
				hcEnemy[i] += HitChance( GetUnitRect( sim.units[0] ), sim.units[1].position + Vec2( 0, 0.9 ), dir2, *sim.units[1].weapon, 10 );
			}
		}
		hcOwn[i] /= 100;
		hcEnemy[i] /= 100;

		if (sim.units[0].health < unit.health) avoid = true;
		
		score[i] = sim.units[0].health * 100;

		if (score[i] > bestScore) {
			bestScore = score[i];
			bestMove = i;
		}
	}

	debug.print( "hit chance: " + str( hcEnemy[bestMove]) + " " + str( hcOwn[bestMove] ) );

	return pair<bool, UnitAction>( avoid, GetAction( bestMove ) );
}

UnitAction Quickstart( const Unit &unit, const Game &game, Debug &debug ) {

	Vec2 center = unit.position + Vec2( 0, unit.size.y*0.5 );
	Rect r = GetUnitRect( unit );

	//const Unit *nearestEnemy = nullptr;
	//double enemyDist = 10E10;
	//for (const Unit &other : game.units) {
	//	if (other.playerId != unit.playerId) {
	//		double dist = distanceSqr( unit.position, other.position );
	//		if (nearestEnemy == nullptr || dist < enemyDist) {
	//			enemyDist = dist;
	//			nearestEnemy = &other;
	//		}
	//	}
	//}

	//const LootBox *nearestWeapon = nullptr;
	//const LootBox *nearestHealth = nullptr;
	//bool betterWeapon = false;
	//double weaponDist = 10E10;

	//for (const LootBox &lootBox : game.lootBoxes) {
	//	if (lootBox.item->type == 1) {
	//		Item::Weapon weap = *std::dynamic_pointer_cast<Item::Weapon>(lootBox.item);
	//		betterWeapon = !unit.weapon || (unit.weapon && IsBetterWeapon( unit.weapon->type, weap.weaponType ));
	//		double dist = distanceSqr( unit.position, lootBox.position );
	//		if (betterWeapon && dist < weaponDist) {
	//			weaponDist = dist;
	//			nearestWeapon = &lootBox;
	//		}
	//	}
	//	if (lootBox.item->type == 0) {
	//		if (nearestHealth == nullptr || distanceSqr( unit.position, lootBox.position ) < distanceSqr( unit.position, nearestHealth->position )) {
	//			nearestHealth = &lootBox;
	//		}
	//	}
	//}

	//Vec2 enemyPos = nearestEnemy ? (Vec2)nearestEnemy->position : center;
	//Vec2 enemyDir = nearestEnemy ? (GetUnitRect( *nearestEnemy ).Center() - center).Normalized() : center;
	//bool enemyVisible = nearestEnemy && RaycastLevel( center, enemyDir, GetUnitRect(*nearestEnemy) ).first;

	pair<double, const Unit *> enemy = NearestUnit( center, game.units, true );
	pair<double, const LootBox *> weapon = NearestLootbox( center, game.lootBoxes, Item::WEAPON );
	pair<double, const LootBox *> health = NearestLootbox( center, game.lootBoxes, Item::HEALTH_PACK );

	bool betterWeapon = !unit.weapon || IsBetterWeapon( unit.weapon->type, weapon.second->item->weaponType );

	Vec2 enemyDir = center.DirTo( enemy.second->position );
	bool enemyVisible = RaycastLevel( center, enemyDir, GetUnitRect( *enemy.second ) ).first;

	bool blockedLeft = IsOnTile( r + Vec2(-1,0), WALL );
	bool blockedRight = IsOnTile( r + Vec2(1,0), WALL );

	Vec2 targetPos = unit.position;
	if ( !unit.weapon ) {
		targetPos = weapon.second->position;
	}
	else if (unit.health < props.unitMaxHealth && health.second) {
		targetPos = health.second->position;
	}
	else if (unit.weapon && betterWeapon ) {
		targetPos = weapon.second->position;
	}
	else {
		if ( enemy.first > 8 || (!enemyVisible && enemy.first > 3 ) ) {
			targetPos = enemy.second->position;
		}
		else {
			targetPos = center + -enemyDir * 3;
			if (!Rect( 1, 1, 39, 29 ).Contains( targetPos )) {
				targetPos = center + enemyDir * 3;
			}
		}
		if (abs( targetPos.x - center.x ) < 2) {
			targetPos.x = center.x + 5;
		}
		if (targetPos.y < enemy.second->position.y) targetPos.y = enemy.second->position.y + 2;
	}

	bool blockedByEnemy = RaycastLevel( center, (targetPos-center).Normalized(), GetUnitRect( *enemy.second ) ).first;

	Vec2 aim = center.DirTo( enemy.second->position );

	bool jump = targetPos.y > unit.position.y;
	if (targetPos.x > unit.position.x && blockedRight) jump = true;
	if (targetPos.x < unit.position.x && blockedLeft) jump = true;
	jump |= blockedByEnemy;

	UnitAction action;
	action.velocity = Sign(targetPos.x - unit.position.x,0.1) * 10;
	action.jump = jump;
	action.jumpDown = !action.jump;
	action.aim = aim;
	action.shoot = true;
	action.swapWeapon = false;
	action.plantMine = false;

	double hc = 0;
	double hcSelf = 0;
	if (unit.weapon) {
		hc = HitChance( GetUnitRect( *enemy.second ), center, aim, *unit.weapon );
		hcSelf = HitChance( GetUnitRect( unit ), center, aim, *unit.weapon );

		//if (unit.weapon->type == ROCKET_LAUNCHER && sqrt(enemyDist) < unit.weapon->params.explosion->radius) {
		//	hcSelf = 1.0;
		//}
		if ( hcSelf > 0.1 && enemy.first > 3 ) action.shoot = false;
		if ( hc < 0 + DBL_EPSILON ) action.shoot = false;
		if ( unit.weapon->type == ROCKET_LAUNCHER && hc < 0.3 && enemy.first < 7 ) action.shoot = false;

		//if (random01() > 1 - hcSelf) action.shoot = false;
		//if (random01() > hc) action.shoot = false;
		//if (hc < 0.001) action.shoot = false;
	}

	if (weapon.first < 2 && betterWeapon) {
		action.swapWeapon = true;
	}

	pair<bool, UnitAction> dodge = Dodge( 100, unit, game, debug );
	//pair<bool, UnitAction> dodge = Avoid( unit, *enemy.second, targetPos, game, debug );


	if (dodge.first) {
		action.jump = dodge.second.jump;
		action.jumpDown = dodge.second.jumpDown;
		action.velocity = dodge.second.velocity;
	}

	double moveDelta = 10. * (1. / 60.);

	if ( /*!dodge.first &&*/ action.jump && unit.jumpState.maxTime < props.unitJumpTime *0.5 && GetTileAt( unit.position - Vec2( 0, moveDelta ) ) == PLATFORM && GetTileAt( unit.position ) == EMPTY) {
		action.jump = false;
		action.jumpDown = false;
	}

	if (enemy.first < 1.5 || ( abs(enemy.second->position.x-unit.position.x) < 2 && enemy.second->position.y < unit.position.y)/*&& unit.health > nearestEnemy->health*/) {
		action.plantMine = true;
	}

	double rc = RaycastLevel( center, aim ).second.Dist( center );

	debug.draw( CustomData::Line( center, RaycastLevel( center, aim ).second, 0.1, ColorFloat( 1, 0, 0, 1 ) ) );

	//debug.draw( CustomData::Log( std::string( "Target pos: " ) + targetPos.toString() ) );
	//debug.print( str(GetTileAt(unit.position - Vec2( 0, 1 ))) + " " + str(GetTileAt(unit.position )) );
	debug.draw( CustomData::Log( action.toString() + " jss>0:" + std::to_string( unit.jumpState.speed > 0) ) );
	debug.draw( CustomData::Log( std::string( "enemydist: " ) + std::to_string( enemy.first ) + " " + std::to_string(enemyVisible) + " " + std::to_string(hc) + " " + std::to_string( hcSelf ) ) );
	//debug.draw( CustomData::Log( std::string( "enemy: " ) + std::to_string( enemy ) ) );
	debug.draw( CustomData::Log( std::string( "dodge: " ) + std::to_string( dodge.first ) + " " + unit.jumpState.toString() ) );
	if(unit.weapon) debug.draw( CustomData::Log( std::string( "lastangle: " ) + std::to_string( Deg( unit.weapon->lastAngle ) ) + " " + std::to_string( unit.weapon->spread ) ) );
	if(unit.weapon) debug.draw( CustomData::Log( std::string( "firetimer: " ) + std::to_string( unit.weapon->fireTimer ) ) );
	if(unit.weapon) debug.draw( CustomData::Log( std::string( "tracedist: " ) + std::to_string( rc ) ));

	DrawPath( PredictPath( unit, action, 100 ), debug );

	return action;
}

UnitAction GetTestCommand( const Unit &unit, int tick ) {
	UnitAction a;
	a.velocity = 10;
	a.jump = true;
	a.jumpDown = true;
	a.plantMine = false;
	a.swapWeapon = false;
	a.shoot = false;
	//if (unit.onLadder) a.jumpDown = true;
	a.jumpDown = false;
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
	return GetTestCommand(unit,game.currentTick);
}

void benchmark( const Unit &unit, const Game &game, Debug &debug ) {
	Sim sim( 1 );
	sim.units.emplace_back( unit );
	sim.units[0].weapon = std::shared_ptr<Weapon>( new Weapon() );
	Timer t;
	t.Begin();
	for (int i = 0; i < 300000; i++) {
		sim.units[0].action = GetAction( random( 0, 8 ) );
		//sim.Tick();
		sim.Microtick();
	}
	t.End();
	cout << t.GetTotalMsec() << endl;
}

UnitAction MyStrategy::getAction(const Unit &unit, const Game &game, Debug &debug) {
	InitStrat( game, unit );
	//benchmark( unit, game, debug );
	//exit(0);
	//return Dodge( 100, unit, game, debug ).second;
	//return game.currentTick == 0 ? UnitAction( 0, true, false ) : UnitAction();
	return Quickstart( unit, game,debug );
}