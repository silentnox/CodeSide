#include "MyStrategy.hpp"
#include "Helpers.h"
#include <assert.h>

using namespace std;

#ifdef  DEBUG
#	define DBG(x) x
#else
#	define DBG(x)
#endif //  DEBUG

Clock moveClock,dodgeClock,hitClock,mineClock,navClock,totalClock;

MyStrategy::MyStrategy() {}

//struct Persist {
//	int lastDamaged = -1;
//	int lastTarget = -1;
//};
//map<int, Persist> persist;

Properties props;
Level level;

Debug debug;
Game game, prevGame;

int selfPlayer;
bool mode2x2 = false;
bool simpleMap = false;

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
	return item.position + Vec2( 0, item.size.y*0.5 );
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

int WeapScore( WeaponType weap ) {
	switch (weap) {
	case WeaponType::PISTOL:
		return mode2x2?2:4;
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
	return bfpair( false, mindist );
}

bool IsVisible( Vec2 from, Vec2 target ) {
	return RaycastLevel( from, from.DirTo( target ) ).second > from.Dist(target);
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
	vector<int> bfs,bfs2;
	Vec2 origin;
	Rect rect;
	JumpState jumpState;

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
					int o = j;
					while (Tiles[i][o + h] == Tile::EMPTY) {
						if (o + h > 29) break;
						if (h > 6) {
							bfs[Tid( i, o + h )] = 1;
							o += h;
							h = 1;
						}
						h++;
					}
				}
				//if (t == Tile::LADDER) {
				//	//bfs[Tid( i - 1, j )] = 1;
				//	//bfs[Tid( i + 1, j )] = 1;
				//	if (Tiles[i - 1][j] == PLATFORM) {
				//		bfs[Tid( i - 1, j )] = 1;
				//		bfs[Tid( i - 1, j-1 )] = 1;
				//		bfs[Tid( i - 1, j-2 )] = 1;
				//	}
				//	if (Tiles[i + 1][j] == PLATFORM) {
				//		bfs[Tid( i + 1, j )] = 1;
				//		bfs[Tid( i + 1, j-1 )] = 1;
				//		bfs[Tid( i + 1, j-2 )] = 1;
				//	}
				//}
				//if (t == Tile::JUMP_PAD) {
				//	bfs[Tid( i, j )] = 1;
				//	bfs[Tid( i, j+1 )] = 1;
				//}
			}
		}
	}

	void InitTemp( const Unit & unit ) {
		bfs2 = bfs;
		for (const Unit & u : game.units) {
			if (u.id != unit.id) {
				bfs2[Tid( (int)floor( u.position.x ), (int)floor( u.position.y ) ) ] = 1;
				bfs2[Tid( (int)floor( u.position.x ), (int)floor( u.position.y )+1 )] = 1;
			}
		}
	}

	void InitPath( const Unit & u ) {
		navClock.Begin();

		Vec2 from = u.position;
		origin = from;
		rect = GetUnitRect( u );
		jumpState = u.jumpState;
		int id = Tid( (int)floor( from.x ), (int)floor( from.y ) );
		
		preds.clear();
		dists.clear();
		
		if (!Rect( 0, 0, 40, 30 ).Contains( from )) return;

		//InitTemp( u );

		Graph::WaveFill2<30 * 40, 40>( id, bfs, preds, dists );

#ifdef  DEBUG
		for (int i = 0; i < 40; i++) {
			for (int j = 0; j < 30; j++) {
				if (bfs[Tid(i,j)]) debug.drawRect( TileRects[i][j], ColorFloat( 1, 0, 0, 0.1 ) );
			}
		}
#endif //  DEBUG

		navClock.End();
	}

	int GetDist( Vec2 target ) const {
		int id = Tid( (int)floor( target.x ), (int)floor( target.y ) );
		int dist = dists[id] * 6;
		return dist;
	}

	pair<int, UnitAction> FindPath( Vec2 target, bool draw = false ) {
		if (!Rect( 0, 0, 40, 30 ).Contains( target )) return pair<int, UnitAction>( -1, UnitAction() );

		static const double moveDelta = 10. * (1. / 60.);

		int id = Tid( (int)floor( target.x ), (int)floor( target.y ) );
		Tile tile = GetTileAt( target );
		vint path = Graph::PredcessorToPath( id, preds );

		if (path.size() < 2) return pair<int, UnitAction>( -1, UnitAction() );

		navClock.Begin();

		int dist = dists[id] * 6;
		UnitAction action;
		ipair node1 = Tcd( path[0] );
		ipair node2 = Tcd( path[1] );

		int idx = 0;
		Vec2 a( rect.Min.x, rect.Max.y );
		Vec2 b( rect.Max.x, rect.Max.y );
		Rect r( rect.Center(), 6 );
		while (idx < path.size() - 1) {
			ipair node = Tcd( path[idx] );
			Rect tr = TileRects[node.first][node.second];
			Vec2 cent = tr.Center()/*-Vec2(0,0.45)*/;
			//if (!tr.Intersects( r ) || !IsVisible( a, cent ) || !IsVisible( b, cent ) || !IsVisible( origin/*-Vec2(0,moveDelta*2)*/, cent ) ) {
			if (!tr.Intersects( r ) || !IsVisible(rect.Center(),cent) || GetTileAt(cent) == JUMP_PAD ) {
				if(idx>1)idx--;
				break;
			}
			idx++;
		}
		node2 = Tcd( path[idx] );

		Vec2 center = TileRects[node2.first][node2.second].Center() - Vec2(0,0.5);

		DBG( debug.drawLine( rect.Center(), center ) );
		DBG( debug.drawWireRect( TileRects[node2.first][node2.second] ) );
		//DBG( debug.drawLine( origin, origin + origin.DirTo(center) * RaycastLevel(origin,origin.DirTo(center) ).second, 0.05, ColorFloat(1,0,0,1) ) );
		//DBG( debug.drawLine( a, a + a.DirTo(center) * RaycastLevel(a,a.DirTo(center) ).second, 0.05, ColorFloat(1,0,0,1) ) );
		//DBG( debug.drawLine( b, b + b.DirTo(center) * RaycastLevel(b,b.DirTo(center) ).second, 0.05, ColorFloat(1,0,0,1) ) );

		double groundDist = RaycastLevel( origin, Vec2( 0, -1 ) ).second;

		//double dx = node2.first - node1.first;
		//double dy = node2.second - node1.second;
		double dx = center.x - origin.x;
		double dy = center.y - origin.y;
		int ox = Sign( dx, 0.05 );
		int oy = Sign( dy, 0.1 );

		//ox = Sign( center.x - origin.x, 0.05 );
		//oy = Sign( center.y - origin.y, 0.1 );

		//bool jump = GetTileAt( origin ) == EMPTY && jumpState.maxTime > 1e-3;
		bool jump = false;
		jump |= ox < 0 && GetTileAt( origin + Vec2( -1, 0 ) ) == WALL;
		jump |= ox > 0 && GetTileAt( origin + Vec2( 1, 0 ) ) == WALL;
		jump |= ox < 0 && GetTileAt( origin + Vec2( -1, -1 ) ) == EMPTY && min(groundDist,abs(dx) ) < abs(dx);
		jump |= ox > 0 && GetTileAt( origin + Vec2( 1, -1 ) ) == EMPTY && min( groundDist, abs( dx ) ) < abs( dx );
		jump |= oy >= 0 && GetTileAt( center + Vec2( 0, -1 ) ) == EMPTY;

		if (ox > 0) action.velocity = 10;
		else if (ox < 0) action.velocity = -10;
		else if (ox == 0) action.velocity = 0;

		if (oy > 0 || jump) action.jump = true;
		else if (oy < 0) action.jumpDown = true;

		//if (oy != 0) {
		//	int n = 0;
		//	while (n < path.size() - 1 && Tcd( path[n] ).first == node1.first) {
		//		n++;
		//	}
		//	ipair node = Tcd( path[n] );
		//	Rect r = TileRects[node1.first][node1.second];
		//	double snap = r.Center().x - origin.x;
		//	if (abs( snap ) > 0) {
		//		action.velocity = Sign( snap ) * 10;
		//	}
		//	if(node.first > node1.first && GetTileAt(origin+Vec2(1,0)) == WALL) action.velocity = 10;
		//	if(node.first < node1.first && GetTileAt(origin+Vec2(-1,0)) == WALL) action.velocity = -10;
		//	//if(oy < 0 && ( GetTileAt(origin+Vec2(0.1,-1)) == JUMP_PAD || GetTileAt( origin + Vec2( 0.1, 0 ) ) == JUMP_PAD) ) action.velocity = -10;
		//	//if(oy < 0 && (GetTileAt( origin + Vec2( -0.1, -1 ) ) == JUMP_PAD || GetTileAt( origin + Vec2( -0.1, 0 ) ) == JUMP_PAD)) action.velocity = 10;
		//	if(oy < 0 && IsOnTile( rect.Expand(Vec2(0,0.2)) + Vec2(0.1,0.), JUMP_PAD ) ) action.velocity = -10;
		//	if(oy < 0 && IsOnTile( rect.Expand( Vec2( 0, 0.2 ) ) + Vec2( -0.1, 0. ), JUMP_PAD )) action.velocity = 10;
		//}

#ifdef  DEBUG
		if (draw) {
			for (int id : path) {
				ipair coord = Navigate::Tcd( id );
				debug.drawRect( TileRects[coord.first][coord.second], ColorFloat( 0, 1, 0, 0.5 ) );
			}
		}
#endif //  DEBUG

		navClock.End();

		return pair<int, UnitAction>( dist, action );
	}

	tuple<int, const LootBox*, UnitAction> NearestLootbox( const vector<LootBox> & items, Item::Type type, int weaponType = -1 ) {
		tuple<int, const LootBox*, UnitAction> nearest;
		get<0>( nearest ) = 1e8;
		get<1>( nearest ) = nullptr;
		for (const LootBox & l : items) {
			if (l.item.type != type) continue;
			if (l.item.type == Item::WEAPON && weaponType != -1 && !IsBetterWeapon((WeaponType)weaponType, l.item.weaponType)) continue;
			//pair<int, UnitAction> reach = FindPath( GetCenter( l ) );
			int dist = GetDist( GetCenter( l ) );
			if (dist < get<0>( nearest )) {
				get<0>( nearest ) = dist;
				get<1>( nearest ) = &l;
				//get<2>( nearest ) = reach.second;
			}
		}
		if (get<1>( nearest )) {
			get<2>( nearest ) = FindPath( GetCenter( *get<1>( nearest ) ) ).second;
		}
		return nearest;
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
			if (tile != WALL || (i != 0 && j != 0 && i != 39 && j != 29)) {
				//TilesByType[tile].emplace_back( ipair( i, j ) );
				TilesByType[tile].emplace_back( Rect( i, j, i + 1., j + 1. ) );
			}
		}
	}

	for (int i = 0; i < 40; i++) {
		if (Tiles[i][0] == EMPTY) {
			simpleMap = true;
			break;
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

	int score[2];

private:
	int microticks;
	double deltaTime,deltaTime2;
public:

	bool useAim = false;
	//bool pierce = false;

	//double explMargin = 0;
	//double bulletMargin = 0;

	Sim( int num = 100 ) {
		microticks = num;
		deltaTime = 1. / (60. * (double)microticks);
		deltaTime2 = 1. / 60.;

		score[0] = 0;
		score[1] = 0;
	}

	Sim( const Game & _game, int num = 100 ) {
		units = _game.units;
		bullets = _game.bullets;
		mines = _game.mines;
		lootBoxes = _game.lootBoxes;

		microticks = num;
		deltaTime = 1. / (60. * (double)microticks);

		score[0] = 0;
		score[1] = 0;
	}

	inline void SetQuality( int num ) {
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
			u.weapon.fireTimer -= deltaTime2;
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

		u.weapon.spread -= u.weapon.params.aimSpeed * deltaTime2;

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
						//u.numHealed += newHealth - u.health;
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
		if (u.onGround && u.jumpState.maxTime == 0 && !u.action.IsMove()) return;

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

		if (ox != 0) {
			Rect test = r + Vec2( ox, 0 );

			if (IsOnTile( test, WALL ) || HasUnitsInRect( test, u.id )) {
				ox = 0;
			}
		}

		if (oy != 0) {
			Rect test = r + Vec2( 0, oy );

			if (IsOnTile( test, WALL ) || HasUnitsInRect( test, u.id )) {
				oy = 0;
			}
		}

		u.position.x += ox;
		u.position.y += oy;
	}

	inline void Damage( Unit & u, int amount, int playerId ) {
		u.health -= amount;

		if (playerId != u.playerId) {
			score[playerId == selfPlayer ? 0 : 1] += min( u.health, amount );
		}
		if (u.health <= 0 && u.health + amount > 0) score[u.playerId != selfPlayer ? 0 : 1] += 1000;
	}

	void ExplodeMine( Mine & m ) {
		if (m.state == EXPLODED) return;

		Rect r( m.position + Vec2(0,m.size.y*0.5), m.explosionParams.radius /*+ explMargin*/ );

		vint ids = UnitsInRect( r );
		for (int i : ids) {
			//units[i].health -= m.explosionParams.damage;
			Damage( units[i], m.explosionParams.damage, m.playerId );
		}
		m.state = EXPLODED;

		vint ids2 = MinesInRect( r );
		for (int i : ids2) {
			ExplodeMine( mines[i] );
		}
	}

	void MoveBullet( Bullet & b ) {
		b.position += b.velocity * deltaTime;

		Rect r( b.position, b.size*0.5 );

		vint hit = UnitsInRect( r, b.unitId );
		vint hit2 = MinesInRect( r );

		bool inWall = IsOnTile( r, WALL );
		if ( inWall || !hit.empty() || !hit2.empty() ) {
			if (!hit.empty()) {
				Unit & u = units[hit[0]];
				if (u.ignoreBullet && !inWall && hit2.empty()) return;
				//u.health -= b.damage;
				Damage( u, b.damage, b.playerId );
			}
			if (!hit2.empty()) {
				ExplodeMine( mines[hit2[0]] );
			}
			if (b.explosionParams.damage) {
				Rect expl( b.position, b.explosionParams.radius + 1e-3/* + b.size*0.5*/ );
				hit = UnitsInRect( expl );
				hit2 = MinesInRect( expl );
				for (int i : hit) {
					//units[i].health -= b.explosionParams.damage;
					Damage( units[i], b.explosionParams.damage, b.playerId );
				}
				for (int i : hit2) {
					ExplodeMine( mines[i] );
				}
			}
			stdh::erase( bullets, b );
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
				//units[ids[0]].numTriggered++;
			}
		}


	}

	inline void Microtick() {
		for (Unit & u : units) {
			MoveUnit( u );
			//AdjustAim( u );
			//PickLoot( u );
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
		for (Unit & u : units) {
			AdjustAim( u );
			PickLoot( u );
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
	int health[9];
	int ticks[9];
	int hitTick[9];
	int score0[9];
	int score1[9];
	int numSafeMoves = 0;
	int action = -1;
};

DodgeRes Dodge( int idx, const Sim & env, const int quality, const int maxTicks ) {

	DodgeRes res;

	const Unit & unit = env.units[idx];

	if (env.bullets.empty() && env.mines.empty()) {
		res.maxHP = unit.health;
		res.minHP = unit.health;
		res.action = 0;
		res.numSafeMoves = 9;
		return res;
	}

	dodgeClock.Begin();
	
	static Sim sim( quality );

	res.maxHP = 0;
	res.minHP = 100;
	res.action = 0;

	for (int i = 0; i < 9; i++) {
		sim.SetQuality( quality );
		//sim.currentTick = env.currentTick;
		sim.currentTick = 0;
		sim.bullets = env.bullets;
		sim.mines = env.mines;
		sim.units = env.units;
		//sim.units[0] = unit;

		//sim.pierce = true;

		sim.score[0] = 0;
		sim.score[1] = 0;

		sim.units[idx].action = GetAction( i );

		int startHp = sim.units[idx].health;

		res.hitTick[i] = -1;
		
		int ticks = 0;
		for (ticks = 0; ticks < maxTicks; ticks++) {
			if (sim.units[idx].health < startHp && res.hitTick[i] == -1) {
				res.hitTick[i] = ticks;
			}
			if (sim.units[idx].health <= 0) break;
			if (sim.bullets.empty()) {
				if (sim.mines.empty()) {
					break;
				}
				sim.SetQuality( min(5,quality) );
			}

			sim.Tick();
		}

		res.health[i] = sim.units[idx].health;
		res.ticks[i] = ticks;

		res.score0[i] = sim.score[0];
		res.score1[i] = sim.score[1];

		res.minHP = min( res.minHP, sim.units[idx].health );
		res.maxHP = max( res.maxHP, sim.units[idx].health );

		if (sim.units[idx].health >= unit.health) {
			res.numSafeMoves++;
		}
		if (res.health[i] > res.health[res.action] || (res.health[i] == res.health[res.action] && res.hitTick[i] > res.hitTick[res.action] &&  res.hitTick[i] != -1)) {
			res.action = i;
		}
	}

	dodgeClock.End();

	return res;
}


struct predictRes {
	int numTraces = 0;
	int numHits = 0;
	double hitChance = 0.;
	double hitChance2 = 0.;
	double avgHealth = 0.;
	double avgHealth2 = 0.;
	int numKills = 0;
};

predictRes HitPredict( const Unit & shooter, Vec2 aim, const Unit & target, bool checkSelf, int numTraces, bool draw = false ) {
	predictRes res;
	res.numTraces = numTraces;
	res.avgHealth = checkSelf ? shooter.health : target.health;

	if (!shooter.hasWeapon) return res;
	if (checkSelf && shooter.weapon.type != ROCKET_LAUNCHER && game.mines.empty()) return res;

	res.avgHealth = 0;

	hitClock.Begin();

	double spread = shooter.weapon.spread;

	Vec2 center = GetUnitRect( shooter ).Center();

	aim = aim.Normalized();

	Vec2 l = aim.Rotate( spread );
	Vec2 r = aim.Rotate( -spread );

	int numSafeMoves = 0;

	const Weapon & weapon = shooter.weapon;

	for (int i = 0; i < numTraces; i++) {
		Vec2 d = l.Slerped( i / (double)(numTraces - 1), r );

		static Sim sim( 5 );
		sim.units.clear();
		sim.bullets.clear();
		//sim.units.emplace_back( target );
		//sim.units.emplace_back( shooter );
		if (!checkSelf) {
			sim.units.emplace_back( target );
			sim.units.emplace_back( shooter );
		}
		else {
			sim.units = game.units;
		}
		sim.mines = game.mines;

		for (const Bullet & b : game.bullets) {
			if (b.unitId == shooter.id) {
				sim.bullets.emplace_back( b );
			}
		}

		//if (checkSelf) {
		//	sim.units[0].action = GetMoveAction( sim.units[0].position.DirTo( sim.units[1].position ) );
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

		//DodgeRes dodge = Dodge( checkSelf?1:0, sim, 5, 30 );
		DodgeRes dodge = Dodge( checkSelf?FindUnit(shooter,sim.units):0, sim, 5, 30 );

#ifdef DEBUG
		if (draw) {
			ColorFloat c = dodge.numSafeMoves ? ColorFloat( 1, 1, 1, 1. - dodge.numSafeMoves / 9. ) : ColorFloat( 1, 0, 0, 1 );
			DBG( debug.drawLine( center, center + d * RaycastLevel( center, d ).second, 0.05, c ) );
		}
#endif

		if (dodge.numSafeMoves == 0) {
			res.numHits++;
			if (dodge.maxHP < 0) res.numKills++;
		}
		numSafeMoves += dodge.numSafeMoves;

		res.avgHealth += dodge.maxHP;

		double avgHealth2 = 0;
		for (int i = 0; i < 9; i++) avgHealth2 += dodge.health[i];
		avgHealth2 /= 9;
		res.avgHealth2 += avgHealth2;
	}

	res.hitChance = res.numHits / (double)numTraces;
	res.hitChance2 = 1. - numSafeMoves / (double)(9 * numTraces);
	res.avgHealth /= numTraces;
	res.avgHealth2 /= numTraces;

	hitClock.End();

	return res;
}

bool TryPlantMine( const Unit & unit ) {
	pair<double, const Unit *> enemy = NearestUnit( GetCenter( unit ), game.units, unit.id, true, false );
	if (!enemy.second) return false;

	return enemy.first < 1.5 || (abs( enemy.second->position.x - unit.position.x ) < 2 && enemy.second->position.y < unit.position.y);
}

bool CanPlantMine( Vec2 pos ) {
	Tile t = GetTileAt( pos + Vec2( 0, -1 / 60. * 10 ) );
	return t == WALL || t == PLATFORM;
}

bpair TryPlantMine2( const Unit & unit ) {
	const double deltaTime = 1 / 60.;

	if (!CanPlantMine(unit.position) || !unit.mines || !unit.hasWeapon || unit.weapon.fireTimer > deltaTime*3) return bpair(false,false);

	mineClock.Begin();

	Mine m;
	m.explosionParams = props.mineExplosionParams;
	m.playerId = unit.playerId;
	m.position = unit.position;
	m.size = props.mineSize;
	m.state = MineState::IDLE;
	//m.timer = deltaTime* unit.mines > 1?2:1;
	m.timer = 0;

	Bullet b;
	b.damage = unit.weapon.params.bullet.damage;
	b.size = unit.weapon.params.bullet.size;
	b.playerId = unit.playerId;
	b.unitId = unit.id;
	b.position = GetCenter(unit);
	b.velocity = Vec2(0,-1) * unit.weapon.params.bullet.speed;
	b.explosionParams = unit.weapon.params.explosion;

	static Sim sim;
	sim.mines = game.mines;
	//if(unit.mines > 1) sim.mines.emplace_back( m );
	//sim.bullets = game.bullets;
	sim.bullets.clear();
	sim.units = game.units;

	for (Unit & u : sim.units) u.ignoreBullet = true;

	sim.bullets.emplace_back( b );

	//sim.units[FindUnit( unit, sim.units )].action.aim = Vec2( 0, -1 );
	
	int totalScore[2];
	totalScore[0] = 0;
	totalScore[1] = 0;

	int numPlant = min( 2, unit.mines );

	for (int num = 1; num <= numPlant; num++) {

		//m.timer = deltaTime * numPlant;
		sim.mines.emplace_back( m );

		for (int i = 0; i < sim.units.size(); i++) {
			DodgeRes res = Dodge( i, sim, 1, 8 );
			//int hp = sim.units[i].health - res.maxHP;
			int score = res.maxHP > 0 ? sim.units[i].health - res.maxHP : sim.units[i].health + 1000;
			if (sim.units[i].playerId == unit.playerId) score *= -1;
			totalScore[num-1] += score;
		}
	}

	mineClock.End();

	DBG( debug.print(VARDUMP( totalScore[0] ) + VARDUMP( totalScore[1] ) ) );

	return bpair(totalScore[0] > 0, totalScore[0] >= totalScore[1]);
}

UnitAction AimHelper( const Unit &unit, const Unit & enemy ) {
	if (!unit.hasWeapon) return UnitAction();

	//pair<float, const Unit*> ally = NearestUnit( unit.position, game.units, unit.id, false );

	const double moveDelta = 10. * (1. / 60.);

	Rect r = GetUnitRect( unit );
	//Rect er = GetUnitRect( enemy );
	Vec2 center = r.Center();
	//double dist = center.Dist( er.Center() );

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

	const double aimSpeed = unit.weapon.params.aimSpeed * (1. / 60.);

	bool smooth = false;
	if (abs(angleDelta) < Rad(20) && abs(angleDelta) > aimSpeed ) {
		aim = lastAim.Rotate( Sign( angleDelta ) * aimSpeed );
		smooth = true;
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

	predictRes hcSelf, hcAlly, hcEnemy1, hcEnemy2;
	bool scdEnemy = false;
	double avgHp = 0;
	for (const Unit & u : game.units) {
		bool isSelf = unit.id == u.id;
		bool isAlly = u.playerId == unit.playerId;
		predictRes pred = HitPredict( unit, aim, u, isSelf, 9, !isAlly );
		if (isSelf) hcSelf = pred;
		else if (isAlly) hcAlly = pred;
		else {
			if (!scdEnemy) hcEnemy1 = pred; else hcEnemy2 = pred;
			scdEnemy = true;
		}
		double deltaHp = (u.health - pred.avgHealth) + (pred.numKills/(double)pred.numTraces)*1000;
		avgHp += isAlly ? -deltaHp*1.4 : deltaHp;
	}
	bool shoot = unit.weapon.type == ROCKET_LAUNCHER ? avgHp > 0:avgHp >= 0 && hc > 0;
	//bool shoot = true;
	//if (unit.weapon.type == ROCKET_LAUNCHER) {
	//	if (hcEnemy1.hitChance + hcEnemy2.hitChance < 0.15 + DBL_EPSILON) shoot = false;
	//}
	//else if (unit.weapon.type == PISTOL) {
	//	if (hcEnemy1.hitChance + hcEnemy2.hitChance < 0 + DBL_EPSILON) shoot = false;
	//	//if (hc < 0 + DBL_EPSILON) shoot = false;
	//}
	//else if (unit.weapon.type == ASSAULT_RIFLE) {
	//	if (hc < 0 + DBL_EPSILON) shoot = false;
	//}
	//if (avgHp < 0) shoot = false;

	UnitAction action;

	action.aim = aim;
	action.shoot = shoot;

#ifdef DEBUG
	debug.drawRect( Rect( predictPos, 0.15 ), ColorFloat( 1, 1, 1, 1 ) );
	debug.drawLine( enemy.position, predictPos );
	//debug.print( "weapon: hc: " + str( _hc ) + " hcs: " + str( hcSelf ) + " hcl: " + str(hcLast) + VARDUMP(hcAlly) + " ang: " + str( Deg( angle ) ) + " lang: " + str(Deg(lastAngle)) + " angd: " + str( Deg( angleDelta ) ) + " spr: " + str( Deg(self.weapon.spread)) + " timer: " + str(self.weapon.fireTimer) );
	//debug.print( VARDUMP(shoot) + "hc2: " + str( hc2.hitChance ) + " " + str( hcSelf2.hitChance ) + " hc2m2: " + str( hc2.hitChance2 ) + " " + str( hcSelf2.hitChance2 ) );
	debug.print( VARDUMP( avgHp ) + VARDUMP( shoot ) + " hc2: " + str( hcEnemy1.hitChance ) + " " + str( hcSelf.hitChance ) + VARDUMP( Deg( angleDelta ) ) + VARDUMP2( "ft",unit.weapon.fireTimer ) );
#endif

	return action;
}

UnitAction MoveHelper( const Unit & unit ) {
	moveClock.Begin();

	double score[9];

	int best = 0;
	int worst = 0;
	
	static Sim sim;

	int idx = FindUnit( unit, game.units );

	navigate.InitPath( unit );

	Vec2 nearestEnemyPos = NearestUnit( unit.position, game.units, unit.id, true ).second->position + Vec2(0,0.9);

	vector<LootBox> lootBoxes;
	for (const LootBox & l : game.lootBoxes) {
		if (l.item.type == Item::MINE) continue;
		Vec2 enemyPos = NearestUnit( GetCenter( l ), game.units, unit.id, true ).second->position + Vec2(0,0.9);
		bfpair ray1 = RaycastLevel( GetCenter( unit ), GetCenter( unit ).DirTo( GetCenter( l ) ), GetRect(l) );
		bfpair ray2 = RaycastLevel( enemyPos, enemyPos.DirTo( GetCenter( l ) ), GetRect(l) );

		if (ray1.first >= ray2.first && ray2.first == false) {
			lootBoxes.emplace_back( l );
		}
		//else if(ray1.second < ray2.second){
		else if(GetCenter( unit ).Dist2( GetCenter( l ) ) < enemyPos.Dist2( GetCenter( l ) )){
			lootBoxes.emplace_back( l );
		}
	}

	tuple<int, const LootBox*, UnitAction> health = navigate.NearestLootbox( lootBoxes, Item::HEALTH_PACK );
	tuple<int, const LootBox*, UnitAction> weapon = navigate.NearestLootbox( lootBoxes, Item::WEAPON, unit.hasWeapon?unit.weapon.type:-1 );
	tuple<int, const LootBox*, UnitAction> mine = navigate.NearestLootbox( game.lootBoxes, Item::MINE );
	//pair<double, const LootBox*> mine = NearestLootbox( unit.position, unit.position, game.lootBoxes, Item::MINE );
	pair<int, UnitAction> enemyReach = navigate.FindPath( nearestEnemyPos, true );

	for (int i = 0; i < 9; i++) {
		sim.currentTick = 0;
		sim.units = game.units;
		sim.bullets = game.bullets;
		sim.mines = game.mines;
		//sim.lootBoxes = game.lootBoxes;
		//sim.pierce = true;

		for (Unit & u : sim.units) {
			if (u.id != unit.id) u.ignoreBullet = true;
		}

		UnitAction action = GetAction( i );

		Vec2 actionDir = GetActionDir( action );

		Unit & self = sim.units[idx];
		self.action = action;

		score[i] = 0;

		sim.SetQuality( 100 );

		double & s = score[i];

		bool spotEnemy = false;

		const int maxTicks = 30;

		for (int tick = 0; tick < maxTicks; tick++) {
			if (sim.bullets.empty()) {
				sim.SetQuality( 5 );
			}

			sim.Tick();

			Rect rect = GetRect( self );
			Vec2 center = GetCenter( self );
			double wallDist1 = RaycastLevel( center, Vec2( 1, 0 ) ).second;
			double wallDist2 = RaycastLevel( center, Vec2( -1, 0 ) ).second;
			double floorDist = RaycastLevel( unit.position, Vec2( 0, -1 ) ).second;
			double ceilDist = RaycastLevel( unit.position, Vec2( 0, 1 ) ).second;
			double wallDistX = min( wallDist1, wallDist2 );
			double wallDistY = min( floorDist, ceilDist );
			double wallDist = min( wallDistX, wallDistY );

			bool isFloat = !self.onGround && (self.jumpState.maxTime < 1e-5 || !self.jumpState.canCancel);

			s += self.health * 10000;

			double maxDist = 50;

			int numVisible = 0;

			if (self.onLadder) s += 200;
			if (self.onGround && GetTileAt(self.position-Vec2(0,-1))==PLATFORM ) s += 200;

			for (const Unit & u : sim.units) {
				if (u.id == self.id) continue;

				bool isEnemy = u.playerId != unit.playerId;
				
				double dist = center.Dist( GetCenter(u) );
				bool visible = RaycastLevel( center, center.DirTo( GetCenter( u ) ), GetRect( u ) ).first;

				double f = dist / maxDist;
				double f2 = 1. - dist / maxDist;
				if (visible) {
					f *= 2;
					f2 *= 2;
				}

				if (visible && isEnemy && u.hasWeapon) {
					bool rl = u.weapon.type == ROCKET_LAUNCHER;
					if (rl) {
						if (wallDistX < 2) s -= (1. - wallDistX / 2.) * 300 * f2;
					}
					if (isFloat) s -= rl?2000:500 * f2;
					if (self.position.y < u.position.y) {
						s -= (u.position.y - self.position.y) * 100;
					}
					else {
						s += 300;
					}
				}

				if (isEnemy && u.mines > 0 && u.onGround) {
					Rect expl( u.position + Vec2( 0, props.mineSize.y*0.5 ), props.mineExplosionParams.radius );
					int dmg = 50 * (min( 2, u.mines ) + (u.hasWeapon && u.weapon.type == ROCKET_LAUNCHER ? 1 : 0));
					if (expl.Intersects( GetUnitRect( self ) ) /*&& dmg >= self.health*/ ) {
						double f = dmg >= self.health ? 10000 : 1000;
						s -= f;
					}
				}

				double range = 0;
				if (u.playerId != self.playerId) {
					if (visible) numVisible++;
					if (u.hasWeapon) {
						double weapRange[] = { 5, 5, 5 };
						range = weapRange[u.weapon.type];
						//if (u.weapon.fireTimer > 0.3) {
						range *= 1 - (u.weapon.fireTimer / u.weapon.params.reloadTime);
						//}
					}
					else {
						range = 0;
					}
					if (self.weapon.params.explosion.damage) {
						range = max( range, self.weapon.params.explosion.radius + 1 );
					}
					if (selfScore > enemyScore && !simpleMap) range = 20;
					if (dist > range) {
						if (visible) {
							s += (1 - dist / maxDist) * 2300;
						}
					}
					else {
						//s -= dist / range * 20;
						//if (visible) {
						s -= (1 - dist / max( range, 1. )) * 2000;
						if (dist < 2 + DBL_EPSILON && u.hasWeapon) s -= 1000;
						//}
					}
				}
				else {
					range = 4;
					if (dist < range /*&& (self.position.y - u.position.y) < 1.8*/) {
						s -= (1 - dist / max( range, 1. )) * 2000;
						if (dist < 2 + DBL_EPSILON) s -= 1000;
					}
					else {
						s += (1 - dist / maxDist) * 1000;
					}
				}
			}
			//if (numVisible) spotEnemy = true;

			for (const Mine & m : sim.mines) {
				Rect expl( m.position + Vec2( 0, m.size.y*0.5 ), m.explosionParams.radius /*+ explMargin*/ );
				Rect trigger = Rect( m.position + Vec2( 0, m.size.y *0.5 ), m.triggerRadius ).Expand( props.mineSize*0.5 );
				if (expl.Intersects( rect )) s -= 500;
				if (trigger.Intersects( rect )) s -= 500;
			}
		}

		s /= sim.currentTick;
		//s = 0;

		if (GetActionDir(enemyReach.second) == actionDir && unit.hasWeapon) {
			s += selfScore <= enemyScore?(isStuck?2000:500):0;
		}
		if (get<1>( health ) && GetActionDir(get<2>( health )) == actionDir) {
			s += 100 * (100-unit.health) + 100;
		}
		if (get<1>( weapon ) && GetActionDir(get<2>( weapon )) == actionDir) {
			s += unit.hasWeapon?1500:4000;
		}
		if (get<1>( mine ) && unit.mines < 2 && (get<1>( mine ))->position.Dist( GetCenter(unit) ) < 5 /*&& GetCenter(unit).y > GetCenter(*mine.second).y*/ && GetActionDir( get<2>( mine ) ) == actionDir) {
			s += 2000;
			DBG( debug.drawLine( GetCenter( unit ), GetCenter( *get<1>( mine ) ), 0.1, ColorFloat( 1, 1, 0, 0.5 ) ) );
		}

		if (score[i] > score[best]) {
			best = i;
		}
		if (score[i] < score[worst]) worst = i;
	}

#ifdef  DEBUG
	for (int i = 0; i < 9; i++) {
		debug.drawLine( GetCenter( unit ), GetCenter( unit ) + GetActionDir( GetAction( i ) ) * 3, 0.1, ColorFloat( 1, 1, 1, (score[i]-score[worst]) / (score[best]-score[worst]) ) );
	}
	if (get<1>( health )) {
		debug.drawLine( GetCenter( unit ), GetCenter( *get<1>( health ) ), 0.1, ColorFloat( 0, 1, 0, 0.5 ) );
	}
#endif //  DEBUG

	moveClock.End();

	return GetAction( best );
}

UnitAction MoveHelperOld( const Unit & unit ) {
	moveClock.Begin();
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

	double range = 0;
	if (enemy.second->hasWeapon) {
		range += 5 * (1 - enemy.second->weapon.fireTimer / enemy.second->weapon.params.reloadTime);
	}
	//if (unit.hasWeapon) {
	//	range += 3 * (unit.weapon.fireTimer / unit.weapon.params.reloadTime);
	//	if (unit.weapon.params.explosion.damage) {
	//		range = max( range, unit.weapon.params.explosion.radius + r.MaxRadius() * 2 );
	//	}
	//}
	//else {
	//	range += 4;
	//}

	double enemyHpDist = health.second ? enemy.second->position.Dist( health.second->position ) : 10000;

	Vec2 targetPos = unit.position;
	if (!unit.hasWeapon && weapon.second) {
		target = 0;
		targetPos = weapon.second->position;
	}
	else if (health.second && unit.health < props.unitMaxHealth) {
		target = 1;
		targetPos = health.second->position;
	}
	else if (enemy.first < range && enemyVisible) {
		target = 4;
		targetPos.x = unit.position.x - Sign( enemyDir.x ) * 5;
		if (!Rect( 1, 1, 39, 29 ).Contains( targetPos )) {
			targetPos.x = unit.position.x + Sign( enemyDir.x ) * 5;
		}
		targetPos.y = max( unit.position.y, enemy.second->position.y + 5 );
	}
	else if (enemy.second->health < 100 && health.second && enemyHpDist < 10) {
		target = 8;
		targetPos = health2.second->position;
	}
	else if (health2.second && unit.health < props.unitMaxHealth) {
		target = 1;
		targetPos = health2.second->position;
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
	else if (ally.second && ally.first < 3) {
		target = 6;
		targetPos = unit.position - unit.position.DirTo( ally.second->position ) * 3;
	}

	UnitAction action;

	navigate.InitPath( unit );
	pair<int, UnitAction> path = navigate.FindPath( targetPos, true );
	if (path.first != -1) {
		action = path.second;
	}
	else {
		action = GetMoveAction( center.DirTo( targetPos ) );
	}

	Sim dodgeSim;
	dodgeSim.units.emplace_back( unit );
	dodgeSim.bullets = game.bullets;
	dodgeSim.mines = game.mines;
	DodgeRes dodge = Dodge( 0, dodgeSim, 100, 60 );
	int actionId = GetActionIndex( action );
	int hitTick = dodge.hitTick[actionId];
	//DodgeRes dodge2 = Dodge( FindUnit(unit,game.units), game, 100 );
	//int hitTick2 = dodge2.hitTick[actionId];
	bool shouldDodge = dodge.numSafeMoves < 9 && hitTick != -1;

	if (/*unit.health > 10 &&*/ unit.health - dodge.minHP < 10 && dodge.minHP > 0 && target == 1) shouldDodge = false;

	if (shouldDodge) {
		action.SetMove( GetAction( dodge.action ) );
	}

	if (!shouldDodge && action.jump && unit.jumpState.maxTime < props.unitJumpTime *0.5 && GetTileAt( unit.position - Vec2( 0, moveDelta ) ) == PLATFORM && GetTileAt( unit.position ) == EMPTY) {
		action.jump = false;
		action.jumpDown = false;
	}

	if (ally.second) {
		bfpair allyRay = RaycastLevel( unit.position - Vec2( 0, DBL_EPSILON ), GetActionDir( action ), GetUnitRect( *ally.second ) );
		bool blockedByAlly = allyRay.first && allyRay.second < 2;

		if (!shouldDodge && blockedByAlly && ally.second->onGround) action.jump = true;
	}

#ifdef DEBUG
	debug.print( VARDUMP( range ) + VARDUMP( actionId ) + VARDUMP( hitTick ) + VARDUMP( enemy.first ) + VARDUMP( enemy.second->weapon.magazine ) + VARDUMP( enemy.second->weapon.fireTimer ) );
	debug.drawLine( center, targetPos, 0.05, ColorFloat( 0, 1, 0, 1 ) );
	//debug.drawWireRect( r, 0.05 );

	DBG( DrawPath( PredictPath2( unit, action, 100, game.units ) ) );
#endif

	moveClock.End();

	return action;
}

UnitAction Quickstart( const Unit &unit ) {
	static const double moveDelta = 10. * (1. / 60.);

	Vec2 center = unit.position + Vec2( 0, unit.size.y*0.5 );
	Rect r = GetUnitRect( unit );

	pair<double, const Unit *> enemy = NearestUnit( center, game.units, unit.id, true, true );
	if (!enemy.second) enemy = NearestUnit( center, game.units, unit.id, true );
	pair<double, const LootBox *> weapon = NearestLootbox( unit.position, unit.position, game.lootBoxes, Item::WEAPON, unit.hasWeapon ? unit.weapon.type : -1 );

	UnitAction action,aim;
	//action = MoveHelperOld( unit );
	action = MoveHelper( unit );
	aim = AimHelper( unit, *enemy.second );
	action.aim = aim.aim;
	action.shoot = aim.shoot;
	//action.plantMine = aim.plantMine;

	//action.plantMine = TryPlantMine( unit );
	//if (action.plantMine) action.shoot = false;

	bpair plant = TryPlantMine2( unit );
	if (plant.first) {
		action.plantMine = true;
		action.shoot = false;
		action.jump = false;
		action.jumpDown = false;
		action.velocity = 0;
		if (plant.second) {
			action.aim = Vec2( 0, -1 );
			action.shoot = true;
		}
	}

	if (weapon.second && r.Intersects( GetRect( *weapon.second ) ) && enemy.first > 3) {
		action.swapWeapon = true;
	}

	if (action.jump && unit.jumpState.maxTime < props.unitJumpTime *0.5 && GetTileAt( unit.position - Vec2( 0, moveDelta ) ) == PLATFORM && GetTileAt( unit.position ) == EMPTY) {
		action.jump = false;
		action.jumpDown = false;
	}

	DBG( debug.print( action.toString() + VARDUMP(simpleMap) + VARDUMP(unit.onGround) + VARDUMP(unit.mines)));

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

int skipUntil = 0;

UnitAction MyStrategy::getAction(const Unit &unit, const Game & _game, Debug & _debug) {

	if (_game.currentTick < skipUntil) return UnitAction();

	totalClock.Begin();

	InitLevel( _game, unit );

	static int prevTick = -1;

	if (prevTick != _game.currentTick) {
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

		if (game.currentTick % 200 == 0) {
			std::cout << std::endl;
			std::cout << "Tick " << game.currentTick;
			std::cout << " Pathfinding: " << navClock.GetTotalMsec();
			std::cout << " Movement: " << moveClock.GetTotalMsec();
			std::cout << " Dodge: " << dodgeClock.GetTotalMsec();
			std::cout << " HitPredict: " << hitClock.GetTotalMsec();
			std::cout << " Mines: " << mineClock.GetTotalMsec();
			std::cout << " Total: " << totalClock.GetTotalMsec();
			std::cout << std::endl;
		}

		prevTick = _game.currentTick;
	}

	//benchmark( unit, game, debug );
	UnitAction action = Quickstart( unit );
	//UnitAction action;

	//navigate.InitPath( unit );
	//UnitAction action = navigate.FindPath()

	//if (prevGame.currentTick != game.currentTick) {
	//	prevGame = game;
	//}
	totalClock.End();

	return action;
}