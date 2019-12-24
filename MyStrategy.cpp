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

Vec2 GetActionDir( const UnitAction & a ) {
	Vec2 dir = Vec2( 0, 0 );
	if (a.jump) {
		dir += Vec2( 0, 1 );
	}
	else if (a.jumpDown) {
		dir += Vec2( 0, -1 );
	}
	dir += Vec2( Sign( a.velocity, 1e-9 ), 0 );
	return dir.IsZero() ? dir : dir.Normalized();
}

UnitAction GetMoveAction( Vec2 dir ) {
	UnitAction a;
	a.velocity = Sign( dir.x, 0.1 ) * 10;
	a.jump = dir.y > 0;
	a.jumpDown = dir.y < 0;
	return a;
}

int GetActionIndex( const UnitAction & a ) {
	for (int i = 0; i < 9; i++) {
		UnitAction a2 = GetAction( i );
		if (a2.velocity == a.velocity && a.jump == a2.jump && a.jumpDown == a2.jumpDown) return i;
	}
	return -1;
}


int WeapScore( WeaponType weap ) {
	switch (weap) {
	case WeaponType::PISTOL:
		return 2;
	case WeaponType::ASSAULT_RIFLE:
		return 1;
	case WeaponType::ROCKET_LAUNCHER:
		return 3;
	default:
		return 0;
	}
}

bool IsBetterWeapon( WeaponType current, WeaponType newweapon ) {
	return WeapScore( newweapon ) > WeapScore( current );
}

struct Persist {
	int lastDamaged = -1;
	int lastTarget = -1;
};
map<int, Persist> persist;

Properties props;
Level level;

Debug debug;
Game game,prevGame;

int selfPlayer;
bool mode2x2;

int selfScore = 0, enemyScore = 0;
int selfScoreTick = 0, enemyScoreTick = 0;
bool isStuck = false;

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

template <typename Item>
inline Vec2 GetCenter( const Item & item ) {
	return item.position + Vec2(0,item.size.y*0.5);
}

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


class Navigate {
public:
	Navigate() {

	}

	inline static int Tid( int x, int y ) {
		return y * 40 + x;
	}
	inline static ipair Tcd( int id ) {
		return ipair( id % 40, id / 40 );
	}

	vector<int> preds;
	vector<int> dists;
	vector<int> bfs;
	Vec2 origin;

	void InitPathing() {
		bfs.resize( 40 * 30 );
		std::fill( bfs.begin(), bfs.end(), 0 );
		for (int i = 0; i < 40; i++) {
			for (int j = 0; j < 30; j++) {
				Tile t = Tiles[i][j];
				//bfs[Tid( i, j )] = 0;
				if(t == Tile::WALL) {
					bfs[Tid( i, j )] = 1;
				}
				if(t == Tile::WALL || t == Tile::PLATFORM) {
					int h = 1;
					while (Tiles[i][j + h] == Tile::EMPTY) {
						if (j + h > 29) break;
						if (h > 6) {
							bfs[Tid( i, j + h )] = 1;
							h = 1;
						}
						h++;
					}
				}
				if (t == Tile::LADDER) {
					if(Tiles[i-1][j] == PLATFORM) bfs[Tid( i-1, j )] = 1;
					if(Tiles[i+1][j] == PLATFORM) bfs[Tid( i+1, j )] = 1;
				}
				if(t == Tile::JUMP_PAD) bfs[Tid( i, j )] = 1;
			}
		}
	}

	void InitPath( Vec2 from ) {
		origin = from;
		int id = Tid( (int)floor( from.x ), (int)floor( from.y ) );
		preds.clear();
		dists.clear();
		if (!Rect( 0, 0, 40, 30 ).Contains( from )) return;
		Graph::WaveFill2<30 * 40, 40>( id, bfs, preds, dists );

		for (int i = 0; i < 40; i++) {
			for (int j = 0; j < 30; j++) {
				if (bfs[Tid(i,j)]) debug.drawRect( TileRects[i][j], ColorFloat( 1, 0, 0, 0.2 ) );
			}
		}
	}

	pair<int, UnitAction> FindPath( Vec2 target, bool draw = false ) {
		static const double moveDelta = 10. * (1. / 60.);
		if (!Rect( 0, 0, 40, 30 ).Contains( target )) return pair<int, UnitAction>( -1, UnitAction() );

		int id = Tid( (int)floor( target.x ), (int)floor( target.y ) );
		Tile tile = GetTileAt( target );
		vint path = Graph::PredcessorToPath( id, preds );

		if (path.size() < 2) return pair<int, UnitAction>( -1, UnitAction() );

		int dist = dists[id] * 6;
		UnitAction action;
		ipair node1 = Tcd( path[0] );
		ipair node2 = Tcd( path[1] );

		int ox = node2.first - node1.first;
		int oy = node2.second - node1.second;

		////this to find nearest ladder or jumper in case height is tooo big
		//int height = 1;
		////while (Tiles[node1.first][node1.second + height] == Tile::EMPTY && node1.second + height < 29) height++;
		//ipair prev = Tcd( path[0] );
		//for (int v : path) {
		//	ipair c = Tcd( v );
		//	if (Tiles[c.first][c.second] != EMPTY) break;
		//	if (c.first == prev.first && c.second > prev.second) {
		//		height++;
		//	}
		//	else {
		//		height = 1;
		//	}
		//	if (height > 6) break;
		//	prev = c;
		//}
		//if (height > 6) {
		//	int loj1 = 0, loj2 = 0;
		//	Tile t1, t2;
		//	bool f1 = false, f2 = false;
		//	while (node1.first - loj1 > 1) {
		//		t1 = Tiles[node1.first - loj1][node1.second];
		//		if (t1 == Tile::WALL) break;
		//		if (t1 == Tile::JUMP_PAD || t1 == Tile::LADDER) {
		//			f1 = true;
		//			break;
		//		}
		//		loj1++;
		//	}
		//	while (node1.first + loj2 < 39) {
		//		t2 = Tiles[node1.first + loj2][node1.second];
		//		if (t2 == Tile::WALL) break;
		//		if (t2 == Tile::JUMP_PAD || t2 == Tile::LADDER) {
		//			f2 = true;			
		//			break;
		//		}
		//		loj2++;
		//	}
		//	if (f1 && f2) {
		//		ox = loj1 < loj2 ? -1 : 1;
		//		if (loj1 == loj2 && loj1 == 0) ox = 0;
		//	}
		//	else {
		//		if (f1) ox = -1;
		//		else if (f2) ox = 1;
		//	}
		//	if (ox != 0) oy = 0;
		//	if (tile == LADDER) {
		//		ox = 0;
		//	}
		//}

		if (ox > 0) action.velocity = 10;
		else if (ox < 0) action.velocity = -10;
		else if (ox == 0) action.velocity = 0;

		if (oy > 0) action.jump = true;
		else if (oy < 0) action.jumpDown = true;

		Rect r = TileRects[node1.first][node1.second];
		double snap = r.Center().x - origin.x;

		if ((action.jump || action.jumpDown) && abs( snap ) > 0.05) {
			action.velocity = Sign( snap ) * 10;
		}

#ifdef  DEBUG
		if (draw) {
			for (int id : path) {
				ipair coord = Navigate::Tcd( id );
				debug.drawRect( TileRects[coord.first][coord.second], ColorFloat( 0, 1, 0, 0.5 ) );
			}
		}
#endif //  DEBUG

		return pair<int, UnitAction>( dist, action );
	}
};
Navigate navigate;

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

void InitLevel( const Game & _game, const Unit & self ) {
	static bool init = false;
	if (init) return;

	props = _game.properties;
	level = _game.level;

	mode2x2 = _game.units.size() > 2;

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

	navigate.InitPathing();

	//TilesByType[WALL].emplace_back( Rect( 0, 0, 1, 30 ) );
	//TilesByType[WALL].emplace_back( Rect( 0, 0, 40, 1 ) );
	//TilesByType[WALL].emplace_back( Rect( 39, 0, 40, 30 ) );
	//TilesByType[WALL].emplace_back( Rect( 0, 29, 40, 30 ) );

	init = true;
}

void InitTick( const Unit & unit, const Game & _game, const Debug & _debug ) {
	static int tickNum = -1;
	if (tickNum == game.currentTick) return;

	InitLevel( _game, unit );

	for (const Player & p : game.players) {
		if (p.id == unit.playerId) selfScore = p.score;
		else if (p.id != unit.playerId) enemyScore = p.score;
	}

	::debug = _debug;
	::game.bullets = _game.bullets;
	::game.currentTick = _game.currentTick;
	::game.lootBoxes = _game.lootBoxes;
	::game.mines = _game.mines;
	::game.units = _game.units;

	tickNum = game.currentTick;
}

bfpair TraceRect( Rect r, Vec2 dir, Rect target = Rect( 0, 0, 0, 0 ) ) {
	double delta = 0.05;
	double offset = 0;
	while (!IsOnTile( r, WALL )) {
		if(!target.IsZero() && r.Intersects( target )) return bfpair( true, offset );
		if( !Rect(0,0,40,30).Contains(r.Center()) ) return bfpair( false, offset );
		r += dir * delta;
		offset += delta;
	}
	return bfpair(false,offset);
}

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
		Vec2 d = l.Nlerped( i / (double)(numRays-1), r );
		bfpair res = RaycastLevel( from, d, target );
		//if (!res.first) {
		//	if (i == 0) {
		//		res = RaycastLevel( from + d.Perp()*weapon.params.bullet.size, d, target );
		//	}
		//	else if (i == numRays - 1) {
		//		res = RaycastLevel( from - d.Perp()*weapon.params.bullet.size, d, target );
		//	}
		//}
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

int FindUnit( int id, const vector<Unit> & units ) {
	for (int i = 0; i < units.size(); i++) {
		if (units[i].id == id) return i;
	}
	return -1;
}

int FindUnit( const Unit & u, const vector<Unit> & units ) {
	for (int i = 0; i < units.size(); i++) {
		if (units[i].id == u.id) return i;
	}
	return -1;
}

pair<double, const Unit*> NearestUnit( Vec2 from, const vector<Unit> & items, int excludeId, bool enemy, bool raycast = false ) {
	const Unit * closest = nullptr;
	double mindist = DBL_MAX;
	for (const Unit & it : items) {
		if (enemy && it.playerId == selfPlayer) continue;
		if (!enemy && it.playerId != selfPlayer) continue;
		if (it.id == excludeId) continue;
		if (raycast && RaycastLevel( from, from.DirTo( it.position + Vec2( 0, it.size.y*0.5 ) ), GetUnitRect(it) ).first == false) continue;
		double dist = from.Dist2( it.position + Vec2(0,it.size.y*0.5) );
		if (dist < mindist) {
			mindist = dist;
			closest = &it;
		}
	}
	return pair<double,const Unit*>( sqrt( mindist ),closest);
}

pair< double, const LootBox*> NearestLootbox( Vec2 from, Vec2 enemyPos, const vector<LootBox> & items, Item::Type type, int weapType = -1, bool raycast = false ) {
	const LootBox * closest = nullptr;
	double mindist = DBL_MAX;
	for (const LootBox & it : items) {
		if (it.item.type != type) continue;
		if (it.item.type == Item::WEAPON && weapType != -1 && !IsBetterWeapon((WeaponType)weapType, it.item.weaponType)) continue;
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
	bool pierce = false;

	//double explMargin = 0;
	//double bulletMargin = 0;

	Sim( int num = 100 ) {
		microticks = num;
		deltaTime = 1. / (60. * (double)microticks);
	}

	Sim( const Game & _game, int num = 100 ) {
		units = _game.units;
		bullets = _game.bullets;
		mines = _game.mines;
		lootBoxes = _game.lootBoxes;

		microticks = num;
		deltaTime = 1. / (60. * (double)microticks);
	}

	void SetQuality( int num ) {
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

		u.weapon.spread += abs( angleDelta );
		u.weapon.lastAngle = angle;

		u.weapon.spread -= u.weapon.params.aimSpeed * deltaTime;

		u.weapon.spread = stdh::minmax( u.weapon.spread, u.weapon.params.minSpread, u.weapon.params.maxSpread );
	}

	void PickLoot( Unit & u ) {
		Rect r = GetUnitRect( u );
		for (LootBox & lb : lootBoxes) {
			//if (!lb.item) continue;
			if (r.Intersects( GetRect( lb ) )) {
				switch (lb.item.type) {
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
						u.weapon.type = lb.item.weaponType;
						u.weapon.params = props.weaponParams[u.weapon.type];
						u.weapon.fireTimer = u.weapon.params.reloadTime;
						u.hasWeapon = true;
						stdh::erase( lootBoxes, lb, stdh::pointer_equal<LootBox> );
					}
					break;
				case Item::MINE:
					u.mines += 1;
					stdh::erase( lootBoxes, lb, stdh::pointer_equal<LootBox> );
					break;
				default:
					throw std::runtime_error( "Unexpected discriminant value" );
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
		//u.onPlatform = onPlatform;

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
			u.jumpState.canCancel = false;
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

		Rect r( m.position + Vec2(0,m.size.y*0.5), m.explosionParams.radius /*+ explMargin*/ );

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
		Rect r( b.position, b.size*0.5/* + bulletMargin*/ );

		b.position += b.velocity * deltaTime;

		vint hit = UnitsInRect( r, b.unitId );
		vint hit2 = MinesInRect( r );

		bool inWall = IsOnTile( r, WALL );
		if ( inWall || !hit.empty() || !hit2.empty() ) {
			if (!hit.empty()) {
				Unit & u = units[hit[0]];
				u.health -= b.damage;
			}
			if (!hit2.empty()) {
				ExplodeMine( mines[hit2[0]] );
			}
			if (b.explosionParams.damage) {
				Rect expl( b.position, b.explosionParams.radius + b.size*0.5/* + explMargin*/ );
				hit = UnitsInRect( expl );
				hit2 = MinesInRect( expl );
				for (int i : hit) {
					units[i].health -= b.explosionParams.damage;
				}
				for (int i : hit2) {
					ExplodeMine( mines[i] );
				}
			}
			if (!pierce || inWall) {
				stdh::erase( bullets, b );
			}
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
			}
			return;
		}

		if (m.state == MineState::IDLE) {
			Rect trigger = Rect( m.position + Vec2(0,m.size.y *0.5), m.triggerRadius ).Expand( props.mineSize*0.5 );
			vint ids = UnitsInRect( trigger );
			if (!ids.empty()) {
				m.state = TRIGGERED;
				m.timer = props.mineTriggerTime;
				units[ids[0]].numTriggered++;
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

vector<Vec2> PredictPath( const Unit & u, const UnitAction & a, int ticks ) {
	Sim sim(1);
	vector<Vec2> path;
	path.reserve( ticks );
	sim.units.emplace_back( u );
	sim.units[0].action = a;
	path.emplace_back( u.position );
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

void DrawPath( const vector<Vec2> & path, const ColorFloat color = ColorFloat(1,1,1,1) ) {
	Vec2 prev = path[0];
	for (const Vec2 & p : path) {
		debug.draw( CustomData::Line( p, prev, 0.1f, color ) );
		prev = p;
	}
}

UnitAction GetSimpleMove( const Unit & u, Vec2 target ) {
	static const double moveDelta = 10. * (1. / 60.);
	UnitAction action;
	Rect r = GetUnitRect( u );
	bool blockedLeft = IsOnTile( r + Vec2( -moveDelta, 0 ), WALL );
	bool blockedRight = IsOnTile( r + Vec2( moveDelta, 0 ), WALL );

	action.jump = target.y > u.position.y + u.size.y*0.5;
	action.jumpDown = target.y < u.position.y;
	if (target.x > u.position.x && blockedRight) action.jump = true;
	if (target.x < u.position.x && blockedLeft) action.jump = true;
	
	//action.jumpDown = !action.jump;

	if (abs( target.x - u.position.x ) < 1. && target.y < u.position.y) {
		action.jump = false;
		action.jumpDown = true;
	}

	action.velocity = Sign( target.x - u.position.x, 0.1 ) * props.unitMaxHorizontalSpeed;
	return action;
}

pair<int, UnitAction> EstimateReach( const Unit & u, Vec2 target, bool draw = false ) {
	Sim sim( 1 );
	int num = 0;
	pair<int, UnitAction> res;
	sim.units.emplace_back( u );
	UnitAction a = GetSimpleMove( u, target );
	res.second = a;
	vector<Vec2> path;
	while (true) {
		if (draw) {
			DBG( path.emplace_back( sim.units[0].position ) );
		}
		if (num >= 360) break;
		if (GetUnitRect( sim.units[0] ).Intersects( Rect( target, 0.5 ))) break;
		sim.units[0].action = a;
		sim.Tick();
		a = GetSimpleMove( sim.units[0], target );

		num++;
	}
	if(draw) DBG( DrawPath( path ) );
	res.first = num;
	return res;
}

struct DodgeRes {
	int minHP = 0;
	int maxHP = 0;
	int score[9];
	int ticks[9];
	int hitTick[9];
	int numSafeMoves = 0;
	//bool dodge = false;
	int action = -1;
};

DodgeRes Dodge( int idx, const Sim & env, int quality = 5 ) {
	DodgeRes res;

	const Unit & unit = env.units[idx];

	if (env.bullets.empty() && env.mines.empty()) {
		res.maxHP = unit.health;
		res.minHP = unit.health;
		res.action = 0;
		res.numSafeMoves = 9;
		return res;
	}

	const int maxTicks = 60;
	
	Sim sim( quality );

	res.maxHP = 0;
	res.minHP = 100;
	res.action = 0;

	for (int i = 0; i < 9; i++) {
		sim.SetQuality( quality );
		sim.currentTick = env.currentTick;
		sim.bullets = env.bullets;
		sim.mines = env.mines;
		sim.units = env.units;
		//sim.units[0] = unit;

		sim.pierce = true;

		sim.units[idx].action = GetAction( i );

		int startHp = sim.units[idx].health;

		res.hitTick[i] = -1;
		
		int ticks = 0;
		for (ticks = 0; ticks < maxTicks; ticks++) {
			if (sim.bullets.empty()) {
				if (sim.mines.empty()) {
					break;
				}
				sim.SetQuality( 5 );
			}
			
			if (sim.units[idx].health <= 0) break;
			if (sim.units[idx].health < startHp && res.hitTick[i] == -1) res.hitTick[i] = ticks;

			sim.Tick();
		}

		res.score[i] = sim.units[idx].health;
		res.ticks[i] = ticks;

		res.minHP = min( res.minHP, sim.units[idx].health );
		res.maxHP = max( res.maxHP, sim.units[idx].health );

		if (sim.units[idx].health >= unit.health) {
			res.numSafeMoves++;
		}
		if (res.score[i] > res.score[res.action] || (res.score[i] == res.score[res.action] && res.hitTick[i] > res.hitTick[res.action] &&  res.hitTick[i] != -1)) {
			res.action = i;
		}
	}

	return res;
}


struct predictRes {
	int numTraces = 0;
	int numHits = 0;
	double hitChance = 0.;
	double hitChance2 = 0.;
};

predictRes HitPredict( const Unit & shooter, Vec2 aim, const Unit & target, bool checkSelf, int numTraces, bool draw = false ) {
	if (!shooter.hasWeapon) return predictRes();

	double spread = shooter.weapon.spread;

	Vec2 center = GetUnitRect( shooter ).Center();

	aim = aim.Normalized();

	Vec2 l = aim.Rotate( spread );
	Vec2 r = aim.Rotate( -spread );

	predictRes res;

	res.numTraces = numTraces;
	int numSafeMoves = 0;

	const Weapon & weapon = shooter.weapon;

	for (int i = 0; i < numTraces; i++) {
		Vec2 d = l.Slerped( i / (double)(numTraces - 1), r );

		Sim sim( 5 );
		sim.units.emplace_back( target );
		sim.units.emplace_back( shooter );
		//sim.bullets = game.bullets;
		sim.mines = game.mines;

		for (const Bullet & b : game.bullets) {
			if (b.unitId == shooter.id) {
				sim.bullets.emplace_back( b );
			}
		}

		//if (checkSelf) {
		//	sim.units[0].action = GetSimpleMove( sim.units[0], sim.units[1].position );
		//}

		Bullet b;
		b.damage = weapon.params.bullet.damage;
		b.size = weapon.params.bullet.size;
		b.playerId = shooter.playerId;
		b.unitId = shooter.id;
		b.position = center;
		b.velocity = d * weapon.params.bullet.speed;
		b.explosionParams = weapon.params.explosion;

		sim.bullets.emplace_back( b );

		DodgeRes dodge = Dodge( checkSelf?1:0, sim );

		if (draw) {
			ColorFloat c = dodge.numSafeMoves ? ColorFloat( 1, 1, 1, 1. - dodge.numSafeMoves / 9. ) : ColorFloat( 1, 0, 0, 1 );
			DBG( debug.drawLine( center, center + d * RaycastLevel( center, d ).second, 0.05, c ) );
		}

		//if (dodge.maxHP < checkSelf?shooter.health:target.health) {
		if (dodge.numSafeMoves == 0) {
			res.numHits++;
		}
		numSafeMoves += dodge.numSafeMoves;
	}

	res.hitChance = res.numHits / (double)numTraces;
	res.hitChance2 = 1. - numSafeMoves / (double)(9 * numTraces);

	return res;
}

UnitAction AimHelper( const Unit &unit, const Unit & enemy ) {
	if (!unit.hasWeapon) return UnitAction();

	pair<float, const Unit*> ally = NearestUnit( unit.position, game.units, unit.id, false );

	const double moveDelta = 10. * (1. / 60.);

	Rect r = GetUnitRect( unit );
	Rect er = GetUnitRect( enemy );
	Vec2 center = r.Center();
	double dist = center.Dist( er.Center() );

	//bfpair vis1 = RaycastLevel( center, center.DirTo( enemy.position ), er );
	//bfpair vis2 = RaycastLevel( center, center.DirTo( enemy.position + Vec2(0,0.9) ), er );
	//bfpair vis3 = RaycastLevel( center, center.DirTo( enemy.position + Vec2( 0, 1.8 ) ), er );

	Vec2 aim;
	Vec2 predictPos;

	if (!enemy.onGround && (enemy.jumpState.maxTime < 1e-5 || !enemy.jumpState.canCancel) ) {
		Sim sim( 1 );
		sim.units.emplace_back( enemy );
		for (int i = 0; i < 60; i++) {
			if (sim.units[0].onGround) break;
			if (center.Dist( sim.units[0].position + Vec2( 0, .9 ) ) < unit.weapon.params.bullet.speed/60.*i) break;
			sim.Tick();
		}
		predictPos = sim.units[0].position;
	}
	else {
		predictPos = enemy.position;
	}

	aim = center.DirTo( predictPos + Vec2(0,0.9) );

	double angle = atan2( aim.y, aim.x );
	double lastAngle = isnan( unit.weapon.lastAngle ) ? angle : unit.weapon.lastAngle;
	double angleDelta = AngleDiff( angle, lastAngle );

	Vec2 lastAim = Vec2( 1, 0 ).Rotate( lastAngle );

	const double aimSpeed = Rad( max( 3., 24. / dist ) );

	if (abs(angleDelta) < M_PI && abs(angleDelta) > aimSpeed ) {
		//aim = lastAim.Rotate( Sign( angleDelta ) * aimSpeed );
	}

	Unit self = unit;
	self.weapon.spread += abs( angleDelta );

	double hc = HitChance( GetUnitRect( predictPos ), center, aim, self.weapon );
	double hcLast = HitChance( GetUnitRect( predictPos ), center, lastAim, unit.weapon );

	double _hc = hc;
	Vec2 _aim = aim;
	if (hcLast > hc-DBL_EPSILON && abs(angleDelta) < Rad(6) ) {
		hc = hcLast;
		aim = lastAim;
	}

	double hcSelf = HitChance( GetUnitRect( unit ), center, aim, self.weapon );
	predictRes hc2 = HitPredict( self, aim, enemy, false, 11, true );
	predictRes hcSelf2 = HitPredict( self, aim, enemy, true, 11, false );

	double hcAlly = ally.second ? HitChance( GetUnitRect( *ally.second ), center, aim, self.weapon ):0.0;
	double hcAlly2 = ally.second ? HitPredict( self, aim, enemy, true, 5, false ).hitChance2:0.0;

	bool shoot = true;

	if (unit.weapon.type == ROCKET_LAUNCHER) {
		if (hc2.hitChance < 0.15) shoot = false;
		if (hcSelf2.hitChance > 0.1) shoot = false;
		if (selfScore >= enemyScore && hc2.hitChance > 0.7) shoot = true;
	}
	else if (unit.weapon.type == PISTOL) {
		if (hc < 0 + DBL_EPSILON) shoot = false;
	}
	else if (unit.weapon.type == ASSAULT_RIFLE) {
		if (hc < 0 + DBL_EPSILON) shoot = false;
	}
	if (hcAlly > 0.2) shoot = false;

#ifdef DEBUG
	debug.draw( CustomData::Rect( predictPos, Vec2( 0.3, 0.3 ), ColorFloat( 1, 1, 1, 1 ) ) );
	debug.drawLine( enemy.position, predictPos );
	debug.print( "weapon: hc: " + str( _hc ) + " hcs: " + str( hcSelf ) + " hcl: " + str(hcLast) + VARDUMP(hcAlly) + " ang: " + str( Deg( angle ) ) + " lang: " + str(Deg(lastAngle)) + " angd: " + str( Deg( angleDelta ) ) + " spr: " + str( Deg(self.weapon.spread)) + " timer: " + str(self.weapon.fireTimer) );
	debug.print( "hc2: " + str( hc2.hitChance ) + " " + str( hcSelf2.hitChance ) + " hc2m2: " + str( hc2.hitChance2 ) + " " + str( hcSelf2.hitChance2 ) );
	double rc = RaycastLevel( center, _aim ).second;
	//debug.drawLine( center, center + _aim * rc, 0.1f, ColorFloat( 1, 0, 0, 1 ) );
	//debug.drawLine( center, center + lastAim * RaycastLevel( center, lastAim ).second, 0.1f, ColorFloat( 0.5, 0, 0, 1 ) );
	//debug.print( "tracedist: " + str( rc ) + " " + str(enemy.onGround) );
#endif

	UnitAction action;

	action.aim = aim;
	action.shoot = shoot;

	return action;
}

bool TryPlantMine( const Unit & unit ) {
	return false;
}

UnitAction MoveHelper( const Unit & unit ) {

	double score[9];
	double lootScore[9];

	int best = 0;
	int worst = 0;
	
	Sim sim;

	int idx = FindUnit( unit, game.units );

	UnitAction botAction[4];

	//for (int i = 0; i < game.units.size(); i++) {
	//	if (game.units[i].id == unit.id) continue;
	//	if (game.units[i].playerId != unit.playerId) continue;
	//	DodgeRes dodge = Dodge( i, game, 5 );
	//	botAction[i] = GetAction( dodge.action );
	//}

	navigate.InitPath( unit.position );

	//pair<int, UnitAction> enemyReach = EstimateReach( unit, NearestUnit( unit.position, game.units, unit.id, true ).second->position );
	pair<int, UnitAction> enemyReach = navigate.FindPath( NearestUnit( unit.position, game.units, unit.id, true ).second->position, true );
	Vec2 enemyDir = GetActionDir( enemyReach.second );

	//pair<float, const LootBox*> health = NearestLootbox( unit.position, unit.position, game.lootBoxes, Item::HEALTH_PACK );

	for (int i = 0; i < 9; i++) lootScore[i] = 0;
	for (const LootBox & l : game.lootBoxes) {
		if (l.item.type == Item::MINE) continue;
		double dist = GetCenter( unit ).Dist( GetCenter( l ) );
		//pair<int, UnitAction> reach = EstimateReach( unit, GetCenter( l ) );
		pair<int, UnitAction> reach = navigate.FindPath( GetCenter( l ) );
		if (reach.first > 500) continue;
		double f = 0;
		switch (l.item.type) {
		case Item::HEALTH_PACK:
			f = (100 - unit.health) * 50 + 50;
			//if(health.second && )
			break;
		case Item::WEAPON:
			if (!unit.hasWeapon) {
				f = 2000;
			}
			else {
				f = max( 0, (WeapScore( l.item.weaponType ) - WeapScore( unit.weapon.type )) ) * 500;
			}
			break;
		case Item::MINE:
			if (dist < 3) {
				f = 5;
			}
			break;
		}
		lootScore[GetActionIndex( reach.second )] += (1. - (reach.first / 500.)) * f;
	}

	for (int i = 0; i < 9; i++) {
		sim.currentTick = 0;
		sim.units = game.units;
		sim.bullets = game.bullets;
		sim.mines = game.mines;
		//sim.lootBoxes = game.lootBoxes;
		sim.pierce = true;

		UnitAction action = GetAction( i );

		Vec2 actionDir = GetActionDir( action );

		Unit & self = sim.units[idx];
		self.action = action;

		//for (int i = 0; i < sim.units.size(); i++) {
		//	if (sim.units[i].id != unit.id) {
		//		sim.units[i].action = botAction[i];
		//	}
		//}

		score[i] = 0;

		sim.SetQuality( 100 );

		double & s = score[i];

		bool spotEnemy = false;

		for (int tick = 0; tick < 60; tick++) {
			if (sim.bullets.empty()) {
				sim.SetQuality( 5 );
			}

			sim.Tick();

			Vec2 center = GetCenter( self );
			double wallDist1 = RaycastLevel( center, Vec2( 1, 0 ) ).second;
			double wallDist2 = RaycastLevel( center, Vec2( -1, 0 ) ).second;
			double floorDist = RaycastLevel( unit.position, Vec2( 0, -1 ) ).second;
			double wallDist = min( wallDist1, wallDist2 );

			bool isFloat = !self.onGround && (self.jumpState.maxTime < 1e-5 || !self.jumpState.canCancel);

			s += self.health * 10000;

			double maxDist = 50;

			for (const LootBox & l : sim.lootBoxes) {
			}

			int numVisible = 0;

			if (isFloat) s -= 300;

			for (const Unit & u : sim.units) {
				if (u.id == self.id) continue;

				if (u.hasWeapon && u.weapon.type == ROCKET_LAUNCHER) {
					//if (wallDist < 2) s -= (1. - wallDist/2.)*200;
				}
				
				double dist = center.Dist( GetCenter(u) );
				bool visible = RaycastLevel( center, center.DirTo( GetCenter( u ) ), GetRect( u ) ).first;
				bool visible2 = RaycastLevel( self.position, self.position.DirTo( GetCenter( u ) ), GetRect( u ) ).first;

				double range = 0;
				if (u.playerId != self.playerId) {
					if (visible) numVisible++;
					//if (numVisible == 1) s += 5;
					//if (visible) {
					//	s += 30;
					//	if (dist < 9 && self.position.y > u.position.y) s += 60;
					//}
					//if (visible2 && unit.hasWeapon && unit.weapon.fireTimer < 0.1) s += 50;
					if (u.hasWeapon) {
						double weapRange[] = { 7, 7, 5 };
						range = weapRange[u.weapon.type];
						if (u.weapon.fireTimer > 0.3) {
							range *= 1 - (u.weapon.fireTimer / u.weapon.params.reloadTime);
						}
					}
					else {
						range = 0;
					}
					if (self.weapon.params.explosion.damage) {
						range = max( range, self.weapon.params.explosion.radius + 1 );
					}
					//range = 0;
					if (dist > range) {
						//if (visible) {
							s += (1 - dist / maxDist) * 1500;
						//}
					}
					else {
						//s -= dist / range * 20;
						//if (visible) {
							s -= (1 - dist / max( range, 1. )) * 1000;
							if (range < 1.5 && u.hasWeapon) s -= 100000000;
						//}
					}
				}
				else {
					range = 3;
					if (dist < range) {
						s -= (1 - dist / max( range, 1. )) * 1000;
					}
					else {
						s += (1 - dist / maxDist) * 100;
					}
				}
			}
			if (numVisible) spotEnemy = true;

			for (const Mine & m : sim.mines) {

			}
		}

		s /= sim.currentTick;

		if (/*!spotEnemy &&*/ enemyDir == actionDir && unit.hasWeapon) {
			s += spotEnemy?200:1000;
			//s += 5000;
		}

		s += lootScore[GetActionIndex( action )];

		if (score[i] > score[best]) {
			best = i;
		}
		if (score[i] < score[worst]) worst = i;
	}

#ifdef  DEBUG
	for (int i = 0; i < 9; i++) {
		debug.drawLine( GetCenter( unit ), GetCenter( unit ) + GetActionDir( GetAction( i ) ) * 3, 0.1, ColorFloat( 1, 1, 1, (score[i]-score[worst]) / (score[best]-score[worst]) ) );
	}
#endif //  DEBUG

	return GetAction( best );
}

UnitAction Quickstart( const Unit &unit ) {
	static const double moveDelta = 10. * (1. / 60.);

	Vec2 center = unit.position + Vec2( 0, unit.size.y*0.5 );
	Rect r = GetUnitRect( unit );

	pair<double, const Unit *> enemy = NearestUnit( center, game.units, unit.id, true, true );
	if (!enemy.second) enemy = NearestUnit( center, game.units, unit.id, true );
	pair<double, const LootBox *> weapon = NearestLootbox( unit.position, unit.position, game.lootBoxes, Item::WEAPON, unit.hasWeapon ? unit.weapon.type : -1 );

	pair<double, const LootBox *> health = NearestLootbox( unit.position, enemy.second->position, game.lootBoxes, Item::HEALTH_PACK );
	pair<double, const LootBox *> health2 = NearestLootbox( unit.position, unit.position, game.lootBoxes, Item::HEALTH_PACK );

	pair<double, const LootBox *> mine = NearestLootbox( unit.position, enemy.second->position, game.lootBoxes, Item::MINE );
	pair<double, const Mine *> mineTrap = NearestMine( unit.position, game.mines );

	pair<float, const Unit*> ally = NearestUnit( unit.position, game.units, unit.id, false );

	Vec2 enemyDir = center.DirTo( enemy.second->position );
	bool enemyVisible = RaycastLevel( center, enemyDir, GetUnitRect( *enemy.second ) ).first;

	bool attack = false;

	int target = -1;

	//bool pickHealth = health.second && unit.health < props.unitMaxHealth;
	//bool pickHealth = health.second && unit.health <= props.unitMaxHealth - props.healthPackHealth;

	double range = 0;
	if (enemy.second->hasWeapon) {
		range += 3 * (1 - enemy.second->weapon.fireTimer / enemy.second->weapon.params.reloadTime);
	}
	if (unit.hasWeapon) {
		range += 3 * (unit.weapon.fireTimer / unit.weapon.params.reloadTime);
		if (unit.weapon.params.explosion.damage) {
			range = max( range, unit.weapon.params.explosion.radius + r.MaxRadius() * 2 );
		}
	}
	else {
		range += 4;
	}

	double enemyHpDist = health2.second ? enemy.second->position.Dist( health2.second->position ) : 10000;

	Vec2 targetPos = unit.position;
	if (!unit.hasWeapon && weapon.second) {
		target = 0;
		targetPos = weapon.second->position;
	}
	else if (health.second && unit.health < props.unitMaxHealth) {
		target = 1;
		targetPos = health.second->position;
	}
	else if (enemy.second->health < 100 && health2.second && enemyHpDist < 10 && enemyHpDist < health2.first) {
		target = 8;
		targetPos = health2.second->position;
	}
	else if (enemy.first < range) {
		target = 4;
		//targetPos.x = unit.position.x -Sign(enemyDir.x) * 5;
		//if (!Rect( 1, 1, 39, 29 ).Contains( targetPos )) {
		//	targetPos.x = unit.position.x + Sign(enemyDir.x) * 5;
		//}
		//targetPos.y = max( unit.position.y, enemy.second->position.y + 5 );
		targetPos.x = unit.position.x + -enemyDir.x * 5;
		if (!Rect( 1, 1, 39, 29 ).Contains( targetPos )) {
			targetPos.x = unit.position.x + enemyDir.x * 5;
		}
		targetPos.y = max( unit.position.y, enemy.second->position.y + 5 );
	}
	else if (health2.second && unit.health < props.unitMaxHealth) {
		target = 1;
		targetPos = health2.second->position;
	}
	else if (ally.second && ally.first < 3) {
		target = 6;
		targetPos = unit.position - unit.position.DirTo( ally.second->position ) * 3;
	}
	//else if (mine.second && mine.first < 2) {
	//	target = 5;
	//	targetPos = mine.second->position;
	//}
	else if (unit.hasWeapon && weapon.second/*&& betterWeapon*/) {
		target = 2;
		targetPos = weapon.second->position;
	}
	else if (enemy.first > 9 || !enemyVisible) {
		attack = true;
		target = 3;
		targetPos = enemy.second->position;
	}

	UnitAction action;
	action = MoveHelper( unit );

	//navigate.InitPath( unit.position );
	//pair<int,UnitAction> path =  navigate.FindPath( targetPos, true );
	//if (path.first != -1) {
	//	action = path.second;
	//}
	//else {
	//	action = GetSimpleMove( unit, targetPos );
	//}

	////action = GetSimpleMove( unit, targetPos );

	//if (weapon.second && r.Intersects( GetRect( *weapon.second ) ) && enemy.first > 3) {
	//	action.swapWeapon = true;
	//}

	//DodgeRes dodge = Dodge( FindUnit( unit, game.units ), game, 100 );
	//int hitTick = dodge.hitTick[GetActionIndex( action )];
	//bool shouldDodge = dodge.numSafeMoves < 9 /*&& hitTick != -1*/;

	//if (/*unit.health > 10 &&*/ unit.health - dodge.minHP < 10 && dodge.minHP > 0 && target == 1) shouldDodge = false;

	//if (shouldDodge) {
	//	action.SetMove( GetAction( dodge.action ) );
	//}

	//UnitAction aim = AimHelper( unit, *enemy.second );

	//action.shoot = aim.shoot;
	//action.aim = aim.aim;

	//if (!shouldDodge && action.jump && unit.jumpState.maxTime < props.unitJumpTime *0.5 && GetTileAt( unit.position - Vec2( 0, moveDelta ) ) == PLATFORM && GetTileAt( unit.position ) == EMPTY) {
	//	action.jump = false;
	//	action.jumpDown = false;
	//}

	//if (!action.shoot && enemy.first < 1.5 || (abs( enemy.second->position.x - unit.position.x ) < 2 && enemy.second->position.y < unit.position.y)/*&& unit.health > nearestEnemy->health*/) {
	//	action.plantMine = true;
	//}

	//if (ally.second) {
	//	bfpair allyRay = RaycastLevel( unit.position - Vec2( 0, DBL_EPSILON ), GetActionDir( action ), GetUnitRect( *ally.second ) );
	//	bool blockedByAlly = allyRay.first && allyRay.second < 2;

	//	if (!shouldDodge && blockedByAlly && ally.second->onGround) action.jump = true;
	//}

	UnitAction aim = AimHelper( unit, *enemy.second );

	action.shoot = aim.shoot;
	action.aim = aim.aim;
	if (action.jump && unit.jumpState.maxTime < props.unitJumpTime *0.5 && GetTileAt( unit.position - Vec2( 0, moveDelta ) ) == PLATFORM && GetTileAt( unit.position ) == EMPTY) {
		action.jump = false;
		action.jumpDown = false;
	}
	if (!action.shoot && enemy.first < 1.5 || (abs( enemy.second->position.x - unit.position.x ) < 2 && enemy.second->position.y < unit.position.y)/*&& unit.health > nearestEnemy->health*/) {
		action.plantMine = true;
	}
	if (weapon.second && r.Intersects( GetRect( *weapon.second ) ) && enemy.first > 3) {
		action.swapWeapon = true;
	}

#ifdef DEBUG
	//debug.print( VARDUMP(hitTick) + VARDUMP( enemy.first ) + VARDUMP(enemy.second->weapon.magazine ) + VARDUMP(enemy.second->weapon.fireTimer ) );
	debug.print( "target: " + str( target ) + " " + action.toString() );
	debug.drawLine( center, targetPos, 0.05, ColorFloat( 0, 1, 0, 1 ) );
	debug.drawWireRect( r, 0.05 );

	DBG( DrawPath( PredictPath2( unit, action, 100, game.units ) ) );
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

UnitAction DebugAction( const Unit &unit ) {
	return GetTestCommand(unit,game.currentTick);
}

void benchmark( const Unit &unit ) {
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
	exit( 0 );
}

UnitAction MyStrategy::getAction(const Unit &unit, const Game & _game, Debug & _debug) {
	InitLevel( _game, unit );

	for (const Player & p : game.players) {
		if (p.id == unit.playerId) {
			if (p.score > selfScore) selfScoreTick = _game.currentTick;
			selfScore = p.score;
		}
		else if (p.id != unit.playerId) {
			if (p.score > enemyScore) enemyScoreTick = _game.currentTick;
			enemyScore = p.score;
		}
	}

	isStuck = _game.currentTick - selfScoreTick > 350;

	::debug = _debug;
	::game.bullets = _game.bullets;
	::game.currentTick = _game.currentTick;
	::game.lootBoxes = _game.lootBoxes;
	::game.mines = _game.mines;
	::game.units = _game.units;

	//benchmark( unit, game, debug );
	UnitAction action = Quickstart( unit );
	//UnitAction action;

	if (prevGame.currentTick != game.currentTick) {
		prevGame = game;
	}

	//navigate.InitPath( unit.position );
	//navigate.FindPath( NearestUnit( unit.position, game.units, -1, true ).second->position );
	//for (int id : navigate.preds) {
	//	ipair coord = Navigate::Tcd( id );
	//	debug.drawRect( TileRects[coord.first][coord.second], ColorFloat( 1, 1, 1, 0.5 ) );
	//}

	return action;
}