#include "MyStrategy.hpp"
#include "Helpers.h"
#include <assert.h>

using namespace std;

#ifdef  DEBUG
#	define DBG(x) x
#else
#	define DBG(x)
#endif //  DEBUG


MyStrategy::MyStrategy() {}

struct Persist {
	Vec2 prevPos;
	Vec2 delta;
	int prevHP;
};

Properties props;
Level level;

int selfPlayer;

Tile Tiles[40][30];
Rect TileRects[40][30];
std::vector<Rect> TilesByType[5];

inline Rect GetUnitRect( const Unit & u ) {
	return Rect( Vec2( u.position.x - u.size.x * 0.5, u.position.y ), Vec2( u.position.x + u.size.x * 0.5, u.position.y + u.size.y ) );
}

inline Rect GetUnitRect( const Vec2 p ) {
	return Rect( Vec2( p.x - props.unitSize.x * 0.5, p.y ), Vec2( p.x + props.unitSize.x * 0.5, p.y + props.unitSize.y ) );
}

template <typename Item>
inline Rect GetRect( const Item & item ) {
	return Rect( Vec2( item.position.x - item.size.x * 0.5, item.position.y ), Vec2( item.position.x + item.size.x * 0.5, item.position.y + item.size.y ) );
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

void OptimizeLevel() {
	TilesByType[WALL].clear();

	//bool used[40][30];

	//memset( used, false, sizeof( used ) );

	for (int row = 1; row < 29; row++) {
		int s = 1;
		int e = 1;

		while (s < 38) {
			while (Tiles[s][row] != WALL && s < 38) s++;
			if (Tiles[s][row] != WALL) break;				
			e = s;
			while (Tiles[e][row] == WALL && e < 38) e++;

			TilesByType[WALL].emplace_back( s, row, e, row + 1 );

			s = e + 1;
		}

	}

	//for (int col = 1; col < 39; col++) {
	//	int s = 1;
	//	int e = 1;

	//	while (s < 28) {
	//		while (Tiles[col][s] != WALL && s < 28) s++;
	//		if (Tiles[col][s] != WALL) break;
	//		e = s;
	//		while (Tiles[col][e] == WALL && e < 28) e++;

	//		TilesByType[WALL].emplace_back( col, s, col+1, e );

	//		s = e + 1;
	//	}

	//}

	TilesByType[WALL].emplace_back( Rect( 0, 0, 1, 30 ) );
	TilesByType[WALL].emplace_back( Rect( 0, 0, 40, 1 ) );
	TilesByType[WALL].emplace_back( Rect( 39, 0, 40, 30 ) );
	TilesByType[WALL].emplace_back( Rect( 0, 29, 40, 30 ) );
}

void InitLevel( const Game &game, const Unit & self ) {
	if (game.currentTick != 0) return;

	props = game.properties;
	level = game.level;

	selfPlayer = self.playerId;

	TilesByType[0].reserve( 1000 );
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

	OptimizeLevel();

	//TilesByType[WALL].emplace_back( Rect( 0, 0, 1, 30 ) );
	//TilesByType[WALL].emplace_back( Rect( 0, 0, 40, 1 ) );
	//TilesByType[WALL].emplace_back( Rect( 39, 0, 40, 30 ) );
	//TilesByType[WALL].emplace_back( Rect( 0, 29, 40, 30 ) );
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

//bfpair RaycastFast( Vec2 from, Vec2 dir ) {
//	int x = (int)floor( from.x );
//	int y = (int)floor( from.y );
//	int sx = Sign( dir.x );
//	int sy = Sign( dir.y );
//
//	while (true) {
//
//	}
//}

bfpair RaycastLevel( Vec2 from, Vec2 dir, Rect target = Rect( 0, 0, 0, 0 ) ) {
	//assert( dir.Len() < 1.0 + DBL_EPSILON );

	double mindist = DBL_MAX;
	for (const Rect & r : TilesByType[WALL]) {
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
			return bfpair( true, res.second );
		}
	}
	return bfpair(false,mindist);
}

double HitChance( Rect target, Vec2 from, Vec2 dir, const Weapon & weapon, int numRays = 30 ) {
	double spread = weapon.spread;

	dir = dir.Normalized();

	Vec2 l = dir.Rotate( spread );
	Vec2 r = dir.Rotate( -spread );

	//assert( l.Len() < 1.0 + DBL_EPSILON && r.Len() < 1.0 + DBL_EPSILON );

	int hits = 0;

	for (int i = 0; i < numRays; i++) {
		Vec2 d = l.Nlerped( i / (double)numRays, r );
		bfpair res = RaycastLevel( from, d, target );
		if (res.first) {
			hits++;
			continue;
		}
		else {
			if (weapon.params.explosion.damage) {
				Rect expl( from + d*(res.second - weapon.params.explosion.radius) , weapon.params.explosion.radius );
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

pair< double, const LootBox*> NearestLootbox( Vec2 from, Vec2 enemyPos, const vector<LootBox> & items, int type, int weapType = -1, bool raycast = false ) {
	const LootBox * closest = nullptr;
	double mindist = DBL_MAX;
	for (const LootBox & it : items) {
		if (!it.item || it.item->type != type) continue;
		if (it.item->type == 1 && weapType != -1 && it.item->weaponType != weapType) continue;
		Vec2 center = it.position + Vec2( 0, it.size.y*0.5 );
		if (raycast && RaycastLevel( from, from.DirTo( center ), Rect(center- it.size*0.5, center+ it.size*0.5) ).first == false) continue;
		double dist = from.Dist2( it.position );
		double dist2 = enemyPos.Dist2( it.position );
		if (dist < mindist && (dist < dist2 || from == enemyPos)) {
			mindist = dist;
			closest = &it;
		}
	}
	return pair<double, const LootBox*>( sqrt(mindist), closest );
}

pair< double, const Mine*> NearestMine( Vec2 from, const vector<Mine> & items, bool raycast = false ) {
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

	double explMargin = 0;
	double bulletMargin = 0;

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

	inline vint MinesInRect( Rect rect, bool trigger = false ) const {
		vint res;
		for (int i = 0; i < mines.size(); i++) {
			Rect r = trigger ? Rect( mines[i].position, mines[i].triggerRadius ) : GetRect( mines[i] );
			if (rect.Intersects( r )) {
				res.emplace_back( i );
			}
		}
		return res;
	}


	void AdjustAim( Unit & u ) {

		if (!u.hasWeapon || !useAim ) return;

		if (u.weapon.fireTimer > 0) {
			u.weapon.fireTimer -= deltaTime;
		}
		else {
			if (!u.weapon.magazine) {
				u.weapon.magazine = u.weapon.params.magazineSize;
			}
			if (u.action.shoot) {
				u.weapon.magazine -= 1;
				u.weapon.fireTimer = u.weapon.magazine > 0 ? u.weapon.params.fireRate : u.weapon.params.reloadTime;
				u.weapon.spread += u.weapon.params.recoil;
			}
		}

		Vec2 aim = u.action.aim.Normalized();

		double angle = atan2( aim.y, aim.x );
		double lastAngle = isnan( u.weapon.lastAngle ) ? angle : u.weapon.lastAngle;
		double angleDelta = AngleDiff( angle, lastAngle );

		u.weapon.spread += angleDelta;
		u.weapon.lastAngle = angle;

		u.weapon.spread -= u.weapon.params.aimSpeed * deltaTime;

		u.weapon.spread = stdh::minmax( u.weapon.spread, u.weapon.params.minSpread, u.weapon.params.maxSpread );
	}

	void PickLoot( Unit & u ) {
		Rect r = GetUnitRect( u );
		for (LootBox & lb : lootBoxes) {
			if (!lb.item) continue;
			if (r.Intersects( GetRect( lb ) )) {
				switch (lb.item->type) {
				case Item::HEALTH_PACK:
					if (u.health < props.unitMaxHealth) {
						int newHealth = min( u.health + props.healthPackHealth, props.unitMaxHealth );
						u.numHealed += newHealth - u.health;
						u.health = newHealth;
						stdh::erase( lootBoxes, lb, stdh::pointer_equal<LootBox> );
					}
					break;
				case Item::WEAPON:
					if (!u.hasWeapon || u.action.swapWeapon) {
						u.weapon.type = lb.item->weaponType;
						u.weapon.params = props.weaponParams[u.weapon.type];
						stdh::erase( lootBoxes, lb, stdh::pointer_equal<LootBox> );
					}
					break;
				case Item::MINE:
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
		bool onPlatform = (feetTile1 == PLATFORM || feetTile2 == PLATFORM) && !onWall && !onLadder && !underPlatform && ( !u.action.jump || u.jumpState.maxTime < 0 + DBL_EPSILON );
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

	void ExplodeMine( Mine & m ) {
		if (m.state == EXPLODED) return;

		Rect r( m.position + Vec2(0,m.size.y*0.5), m.explosionParams.radius + explMargin );

		vint ids = UnitsInRect( r );
		for (int i : ids) {
			units[i].health -= m.explosionParams.damage;
		}
		m.state = EXPLODED;

		vint ids2 = MinesInRect( r );
		for (int i : ids2) {
			ExplodeMine( mines[i] );
		}
	}

	void MoveBullet( Bullet & b ) {
		Rect r( b.position, b.size + bulletMargin );

		//Vec2 pos = b.position;
		b.position += b.velocity * deltaTime;

		vint hit = UnitsInRect( r, b.unitId );
		vint hit2 = MinesInRect( r );

		if (IsOnTile( r, WALL ) || !hit.empty() ) {
			if (!hit.empty()) {
				Unit & u = units[hit[0]];
				u.health -= b.damage;
			}
			if (b.explosionParams) {
				Rect expl( b.position, b.explosionParams->radius + b.size + explMargin );
				hit = UnitsInRect( expl );
				hit2 = MinesInRect( expl );
				for (int i : hit) {
					units[i].health -= b.explosionParams->damage;
				}
				for (int i : hit2) {
					ExplodeMine( mines[i] );
				}
			}

			stdh::erase( bullets, b );
		}
		if (!hit2.empty()) {
			ExplodeMine( mines[hit2[0]] );
		}
	}

	void UpdateMine( Mine & m ) {
		if (m.state == EXPLODED) {
			stdh::erase( mines, m, stdh::pointer_equal<Mine> );
			return;
		}

		if (m.state == PREPARING) {
			m.timer -= deltaTime;
			if (m.timer < 0.) {
				m.state = MineState::IDLE;
			}
		}

		if (m.state == TRIGGERED) {
			m.timer -= deltaTime;
			if (m.timer < 0. + DBL_EPSILON) {
				ExplodeMine( m );
				//vint ids = UnitsInRect( Rect( m.position, m.explosionParams.radius ) );
				//if (!ids.empty()) {
				//	for (int i : ids) {
				//		units[i].health -= m.explosionParams.damage;
				//	}
				//}
				//m.state = EXPLODED;
			}
			return;
		}

		if (m.state == MineState::IDLE) {
			Rect trigger = Rect( m.position + Vec2(0,m.size.y *0.5), m.triggerRadius ).Expand( props.mineSize*0.5 );
			vint ids = UnitsInRect( trigger );
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

vector<Vec2> PredictPath2( const Unit & u, const UnitAction & a, int ticks, const vector<Unit> & other ) {
	Sim sim( 1 );
	vector<Vec2> path;
	path.reserve( ticks );
	sim.units.emplace_back( u );
	sim.units[0].action = a;

	for (const Unit & u2 : other) {
		if (u2.id != u.id) {
			sim.units.emplace_back( u2 );
		}
	}

	for (int i = 0; i < ticks; i++) {
		sim.Tick();
		path.emplace_back( sim.units[0].position );
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
	case 3: // left
		return UnitAction( -10., false, false );
	case 4: // down
		return UnitAction( 0., false, true );
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
	score[WeaponType::PISTOL]			= 1;
	score[WeaponType::ASSAULT_RIFLE]	= 1;
	score[WeaponType::ROCKET_LAUNCHER]	= 2;
	return score[newweapon] > score[current];
}

UnitAction GetSimpleMove( const Unit & u, Vec2 target ) {
	UnitAction action;
	Rect r = GetUnitRect( u );
	bool blockedLeft = IsOnTile( r + Vec2( -1, 0 ), WALL );
	bool blockedRight = IsOnTile( r + Vec2( 1, 0 ), WALL );

	action.jump = target.y > u.position.y;
	if (target.x > u.position.x && blockedRight) action.jump = true;
	if (target.x < u.position.x && blockedLeft) action.jump = true;
	action.jumpDown = !action.jump;
	action.velocity = Sign( target.x - u.position.x, 0.1 ) * props.unitMaxHorizontalSpeed;
	return action;
}

pair<bool, UnitAction> Dodge( int ticks, const Unit &unit, const Game &game, Debug &debug ) {
	Sim sim( 5 );
	sim.currentTick = game.currentTick;
	sim.bullets = game.bullets;
	sim.units.resize( 1 );
	sim.units[0] = unit;

	sim.mines = game.mines;
	
	//sim.explMargin = 10. * (1. / 60.);
	//sim.bulletMargin = 10. * (1. / 60.);

	std::vector<ipair> actions;
	actions.resize( 9 );

	bool dodge = false;

	for (int i = 0; i < 9; i++) {
		Sim sim2 = sim;
		UnitAction a = GetAction( i );
		sim2.units[0].action = a;
		sim2.Simulate( ticks );
		actions[i].first = sim2.units[0].health;
		actions[i].second = i;
		if (sim2.units[0].health < unit.health) dodge = true;

		ColorFloat c = sim2.units[0].health < unit.health ? ColorFloat( 1, 0, 0, 1 ) : ColorFloat( 0, 0, 1, 1 );

		DBG( DrawPath( PredictPath2( unit, GetAction( i ), 100, game.units ), debug, c ) );
	}

	ipair best = stdh::best( actions, []( const ipair & a, const ipair & b ) { return a.first > b.first; } );

	//dodge = actions[0].first < best.first;

	debug.print( "dodge: " + str( actions[0].first ) + " " + str( best.first ) + " " + str(best.second) );

	return pair<bool, UnitAction>( dodge, GetAction(best.second) );
}

//Vec2 PredictAim( Vec2 from, const Weapon & w, const Unit & target ) {
//	if (!target.onGround) {
//		Sim sim( 1 );
//		sim.units.emplace_back( target );
//		for (int i = 0; i < 60; i++) {
//			if (sim.units[0].onGround) break;
//			if (from.Dist( sim.units[0].position + Vec2( 0, .9 )) < w.params.bullet.speed*i ) break;
//			sim.Tick();
//		}
//		return from.DirTo( sim.units[0].position + Vec2( 0, 0.9 ) );
//	}
//	else {
//		return from.DirTo( target.position + Vec2( 0, 0.9 ) );
//	}
//}

struct actionScore {
	int action = 0;
	int health = 0;
	double hcOwn = 0.;
	double hcSelf = 0.;
	double hcEnemy = 0.;
	int score = 0;
};

bool asAvoid( const actionScore & a, const actionScore & b ) {
	if (a.health > b.health) {
		return true;
	}
	else if (a.health == b.health) {
		return a.hcEnemy < b.hcEnemy;
	}
	return false;
}

bool asAttack( const actionScore & a, const actionScore & b ) {
	if (a.health > b.health) {
		return true;
	}
	else if (a.health == b.health) {
		return a.hcOwn-a.hcEnemy > b.hcOwn-a.hcEnemy;
	}
	return false;
}

pair<bool, UnitAction> Avoid( const Unit &unit, const Unit & enemy, Vec2 target, bool attack, const Game &game, Debug &debug ) {

	vector<actionScore> actions;
	actions.resize( 9 );

	Sim sim( 5 );
	sim.units.resize( 2 );

	bool avoid = false;

	int numTicks = 80;

	for (int i = 0; i < 9; i++) {
		actions[i].action = i;

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

		Unit & self = sim.units[0];
		Unit & en = sim.units[1];

		for (int tick = 0; tick < numTicks; tick++) {
			Vec2 dir = self.position.DirTo( en.position + Vec2( 0, 0.9 ) );
			Vec2 dir2 = en.position.DirTo( self.position + Vec2( 0, 0.9 ) );
			self.action.aim = dir;
			//en.action = GetSimpleMove( en, self.position );
			en.action.aim = dir2;

			sim.Tick();
			
			if (self.hasWeapon && self.weapon.fireTimer <= 0 + DBL_EPSILON) {
				actions[i].hcOwn += HitChance( GetUnitRect( en ), self.position + Vec2(0,0.9), dir, self.weapon, 10 );
				if (unit.weapon.params.explosion.damage) {
					actions[i].hcSelf += HitChance( GetUnitRect( self ), self.position + Vec2( 0, 0.9 ), dir, self.weapon, 6 );
				}
			}
			if (en.hasWeapon && en.weapon.fireTimer <= 0 + DBL_EPSILON) {
				actions[i].hcEnemy += HitChance( GetUnitRect( self), sim.units[1].position + Vec2( 0, 0.9 ), dir2, en.weapon, 10 );
			}
		}
		actions[i].health = self.health;
		actions[i].hcOwn /= numTicks;
		actions[i].hcSelf /= numTicks;
		actions[i].hcEnemy /= numTicks;
		
		actions[i].score = self.health * 100;
	}

	actionScore best;

	attack = false;

	if (attack) {
		best = stdh::best( actions, asAttack );
		avoid = actions[0].health < best.health || (actions[0].hcOwn < best.hcOwn);
	}
	else {
		best = stdh::best( actions, asAvoid );
		avoid = actions[0].health < best.health || (actions[0].hcEnemy > 0.25 && actions[0].hcEnemy > best.hcEnemy);
	}

	debug.print( "hitchance: " + str( actions[0].hcEnemy) + " " + str( actions[0].hcOwn ) + " " + str( best.hcEnemy ) + " " + str( best.hcOwn ) );
	debug.print( "hp: " + str( attack ) + " "+ str( best.action) + " " + str( actions[0].health ) + " " + str( best.health ));

	return pair<bool, UnitAction>( avoid, GetAction( best.action ) );
}

UnitAction AimHelper( const Unit &unit, const Unit & enemy, const Game &game, Debug &debug ) {
	if (!unit.hasWeapon) return UnitAction();

	const double moveDelta = 10. * (1. / 60.);

	double hc = 0;
	double hcSelf = 0;

	Rect r = GetUnitRect( unit );
	Rect er = GetUnitRect( enemy );
	Vec2 center = r.Center();

	//Vec2 enemyDir = center.DirTo( er.Center() );
	//Vec2 enemyDir2 = center.DirTo( enemy.position );
	bool vis1 = RaycastLevel( center, center.DirTo( enemy.position ), er ).first;
	bool vis2 = RaycastLevel( center, center.DirTo( enemy.position + Vec2(0,0.9) ), er ).first;
	bool vis3 = RaycastLevel( center, center.DirTo( enemy.position + Vec2( 0, 1.8 ) ), er ).first;

	Vec2 aim;
	//Vec2 aim = center.DirTo( enemy.position + Vec2( 0, 0.9 ) );
	//Vec2 aim = PredictAim( center, unit.weapon, enemy );

	Vec2 predictPos;

	if (!enemy.onGround && (enemy.jumpState.maxTime < 1e-5 || !enemy.jumpState.canCancel) ) {
		Sim sim( 1 );
		sim.units.emplace_back( enemy );
		for (int i = 0; i < 60; i++) {
			if (sim.units[0].onGround) break;
			if (center.Dist( sim.units[0].position + Vec2( 0, .9 ) ) < enemy.weapon.params.bullet.speed/60.*i) break;
			sim.Tick();
		}
		predictPos = sim.units[0].position;
	}
	else {
		predictPos = enemy.position;
	}

	aim = center.DirTo( predictPos + Vec2(0,0.9) );

	double angle = atan2( aim.y, aim.x );
	double angleDelta = AngleDiff( angle, unit.weapon.lastAngle );

	Vec2 lastAim = Vec2( 1, 0 ).Rotate( unit.weapon.lastAngle );

	Weapon w = unit.weapon;
	w.spread += angleDelta;

	//hc = HitChance( GetUnitRect( enemy ), center, aim, w );
	hc = HitChance( GetUnitRect( predictPos ), center, aim, w );
	hcSelf = HitChance( GetUnitRect( unit ), center, aim, w );

	bool shoot = true;

	if (unit.weapon.type == ROCKET_LAUNCHER) {
		//if (hcSelf > hc) shoot = false;
		if (hc < 0.4) shoot = false;
		if (hcSelf > 0.2) shoot = false;
	}

	if (hc < 0 + DBL_EPSILON) shoot = false;

#ifdef DEBUG
	debug.draw( CustomData::Rect( predictPos, Vec2( 0.3, 0.3 ), ColorFloat( 1, 1, 1, 1 ) ) );
	debug.drawLine( enemy.position, predictPos );
	debug.print( "weapon: hc: " + str( hc ) + " hcs: " + str( hcSelf ) + " ang: " + str( Deg( angle ) ) + " angd: " + str( Deg( angleDelta ) ) + " spr: " + str( Deg(w.spread)) + " timer: " + str(w.fireTimer) );
	double rc = RaycastLevel( center, aim ).second;
	debug.draw( CustomData::Line( center, center + aim * rc, 0.1, ColorFloat( 1, 0, 0, 1 ) ) );
	debug.print( "tracedist: " + str( rc ) + " " + str(enemy.onGround) );
#endif

	UnitAction action;

	action.aim = aim;
	action.shoot = shoot;

	return action;
}

UnitAction Quickstart( const Unit &unit, const Game &game, Debug &debug ) {
	static const double moveDelta = 10. * (1. / 60.);

	Vec2 center = unit.position + Vec2( 0, unit.size.y*0.5 );
	Rect r = GetUnitRect( unit );

	pair<double, const Unit *> enemy = NearestUnit( center, game.units, true );
	pair<double, const LootBox *> weapon = NearestLootbox( center, center, game.lootBoxes, Item::WEAPON );
	pair<double, const LootBox *> health = NearestLootbox( center, enemy.second->position, game.lootBoxes, Item::HEALTH_PACK );
	pair<double, const LootBox *> mine = NearestLootbox( center, enemy.second->position, game.lootBoxes, Item::MINE );
	pair<double, const Mine *> mineTrap = NearestMine( center, game.mines );

	bool betterWeapon = !unit.hasWeapon || IsBetterWeapon( unit.weapon.type, weapon.second->item->weaponType );

	Vec2 enemyDir = center.DirTo( enemy.second->position );
	bool enemyVisible = RaycastLevel( center, enemyDir, GetUnitRect( *enemy.second ) ).first;

	//bool blockedLeft = IsOnTile( r + Vec2(-1,0), WALL );
	//bool blockedRight = IsOnTile( r + Vec2(1,0), WALL );

	bool attack = false;

	int target = -1;

	Vec2 targetPos = unit.position;
	if ( !unit.hasWeapon ) {
		target = 0;
		targetPos = weapon.second->position;
	}
	else if (unit.health < props.unitMaxHealth && health.second) {
		target = 1;
		targetPos = health.second->position;
	}
	else if (unit.hasWeapon && betterWeapon ) {
		target = 2;
		targetPos = weapon.second->position;
	}
	//else if (mine.second && mine.first < 2) {
	//	target = 5;
	//}
	else {
		attack = true;
		if (enemy.first > 9) {
			target = 3;
			targetPos = enemy.second->position;
		}
		else {
			if (abs( enemy.second->position.x - unit.position.x ) < 3 || enemy.second->position.y > unit.position.y || !enemyVisible) {
				target = 4;
				targetPos.x = unit.position.x + -enemyDir.x * 10;
				if (!Rect( 1, 1, 39, 29 ).Contains( targetPos )) {
					targetPos.x = unit.position.x + enemyDir.x * 10;
				}
				targetPos.y = enemy.second->position.y + 2;
			}
		}
	}

	//bool blockedByEnemy = RaycastLevel( center, (targetPos-center).Normalized(), GetUnitRect( *enemy.second ) ).first;

	//Vec2 aim = center.DirTo( enemy.second->position + Vec2(0,0.9) );

	//bool jump = targetPos.y > unit.position.y;
	//if (targetPos.x > unit.position.x && blockedRight) jump = true;
	//if (targetPos.x < unit.position.x && blockedLeft) jump = true;
	//jump |= blockedByEnemy;

	UnitAction action;
	action = GetSimpleMove( unit, targetPos );
	//action.velocity = Sign(targetPos.x - unit.position.x,0.1) * props.unitMaxHorizontalSpeed;
	//action.jump = jump;
	//action.jumpDown = !action.jump;
	//action.aim = aim;
	//action.shoot = true;
	//action.swapWeapon = false;
	//action.plantMine = false;

	//double hc = 0;
	//double hcSelf = 0;

	//if (unit.hasWeapon) {
	//	hc = HitChance( GetUnitRect( *enemy.second ), center, aim, unit.weapon );
	//	hcSelf = HitChance( GetUnitRect( unit ), center, aim, unit.weapon );

	//	if (unit.weapon.type == ROCKET_LAUNCHER) {
	//		if (hcSelf > hc) action.shoot = false;
	//	}
	//}

	//if (hc < 0 + DBL_EPSILON) action.shoot = false;

	if (r.Intersects(GetRect(*weapon.second)) && betterWeapon && enemy.first > 3) {
		action.swapWeapon = true;
	}

	//action = UnitAction();

	pair<bool, UnitAction> dodge = Dodge( 100, unit, game, debug );
	//pair<bool, UnitAction> dodge = Avoid( unit, *enemy.second, targetPos, attack, game, debug );

	if (dodge.first) {
		action.jump = dodge.second.jump;
		action.jumpDown = dodge.second.jumpDown;
		action.velocity = dodge.second.velocity;
	}

	UnitAction aim = AimHelper( unit, *enemy.second, game, debug );

	action.shoot = aim.shoot;
	action.aim = aim.aim;

	if ( !dodge.first && action.jump && unit.jumpState.maxTime < props.unitJumpTime *0.5 && GetTileAt( unit.position - Vec2( 0, moveDelta ) ) == PLATFORM && GetTileAt( unit.position ) == EMPTY) {
		action.jump = false;
		action.jumpDown = false;
	}

	if (enemy.first < 1.5 || (abs( enemy.second->position.x - unit.position.x ) < 2 && enemy.second->position.y < unit.position.y)/*&& unit.health > nearestEnemy->health*/) {
		action.plantMine = true;
	}

#ifdef DEBUG
	//debug.print( "ontop: " + str( r.Intersects( GetUnitRect( *enemy.second )  )) );
	debug.print( "target: " + str(target) + " " + action.toString() );
	//debug.print( "enemy: " + std::to_string( enemy.first ) + " " + std::to_string(enemyVisible) + " " + std::to_string(hc) + " " + std::to_string( hcSelf ) );
	debug.print( "dodge: " + str( dodge.first ) + " " + unit.jumpState.toString() );
	//debug.print( "enemy: " + enemy.second->toString() );
	//debug.draw( CustomData::Log( std::string( "lastangle: " ) + std::to_string( Deg( unit.weapon.lastAngle ) ) + " " + std::to_string( Deg( unit.weapon.spread ) ) ) )


	DBG( DrawPath( PredictPath( unit, action, 100 ), debug ) );
#endif

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
	InitLevel( game, unit );
	//benchmark( unit, game, debug );
	//exit(0);
	//return Dodge( 100, unit, game, debug ).second;
	//return game.currentTick == 0 ? UnitAction( 0, true, false ) : UnitAction();
	return Quickstart( unit, game,debug );
}