
//
//struct actionScore {
//	int action = 0;
//	int health = 0;
//	Vec2 finalPos;
//	Unit finalState;
//
//	Vec2 enemyPos;
//
//	int hitTick = -1;
//
//	int floatTicks = 0;
//
//	bool enemyVisible = false;
//	double enemyDist = 0;
//
//	double hcOwn = 0.;
//	double hcSelf = 0.;
//	double hcEnemy = 0.;
//	double hcAlly = 0.;
//
//	bool isStuck = false;
//
//	double minTargetDist = INFINITY;
//	double minEnemyDist = INFINITY;
//
//	bool targetDir = false;
//	bool targetMove = false;
//
//	predictRes pred1, pred2;
//
//	double score = 0;
//	int ticks = 0;
//};
//
//enum {
//	TG_ENEMY,
//	TG_HEALTH,
//	TG_HEALTH2,
//	TG_WEAPON,
//	TG_WEAPON2,
//	TG_MINE,
//	TG_ALLY //???
//};
//
//enum {
//	SM_ATTACK,
//	SM_DODGE,
//	SM_AVOID,
//	SM_SUICIDE
//};
//
//UnitAction SmartAction( const Unit &unit, const Unit & en, Vec2 target, int targetType, int scoreMode ) {
//
//	array<actionScore, 9> actions;
//
//	Sim sim( 5 );
//	sim.units.resize( 2 );
//
//	const int numTicks = 45;
//
//	UnitAction targetMove = GetSimpleMove( unit, target );
//	Vec2 targetMoveDir = GetActionDir( targetMove );
//
//	predictRes hcEnemy = en.hasWeapon && !isnan( en.weapon.lastAngle ) ? HitPredict( en, Vec2( 1, 0 ).Rotate( en.weapon.lastAngle ), unit, false, 11, true ) : predictRes();
//
//	bool enemyVisible = RaycastLevel( GetCenter( unit ), unit.position.DirTo( en.position ), GetUnitRect( en ) ).first;
//
//	for (int i = 0; i < 9; i++) {
//
//		actionScore & a = actions[i];
//
//		a.action = i;
//
//		UnitAction action = GetAction( i );
//
//		sim.currentTick = 0;
//		sim.mines = game.mines;
//		sim.bullets = game.bullets;
//		//sim.lootBoxes = game.lootBoxes;
//		//sim.units[0] = unit;
//		//sim.units[1] = enemy;
//		//sim.units[0].action = action;
//
//		sim.units = game.units;
//
//		//sim.units[1].action = GetSimpleMove( enemy, unit.position );
//
//		//sim.units[0].action.swapWeapon = true;
//
//		sim.useAim = true;
//
//		//sim.units[0].action.shoot = true;
//		//sim.units[1].action.shoot = true;
//
//		//Unit & self = sim.units[0];
//		//Unit & enemy = sim.units[1];
//		Unit & self = sim.units[FindUnit( unit, sim.units )];
//		Unit & enemy = sim.units[FindUnit( en, sim.units )];
//
//		self.action = action;
//
//		//EvaluateAction( self, enemy, sim, a, numTicks );
//		vector<Vec2> path;
//		for (int tick = 0; tick < numTicks; tick++) {
//
//			if (self.health <= 0) break;
//
//			if (!sim.bullets.empty()) {
//				sim.SetQuality( 100 );
//			}
//			else {
//				sim.SetQuality( 5 );
//			}
//
//			Vec2 dirToEnemy = self.position.DirTo( enemy.position );
//			Vec2 dirToSelf = enemy.position.DirTo( self.position );
//			self.action.aim = dirToEnemy;
//			//enemy.action = GetSimpleMove( enemy, self.position );
//			enemy.action.aim = dirToSelf;
//
//			//if (!enemy.onGround && enemy.jumpState.maxTime > 0) enemy.action.jump = true;
//			if (enemy.position.y < self.position.y) enemy.action.jump = true;
//			enemy.action.jumpDown = !enemy.action.jump;
//
//			int hp = self.health;
//
//			sim.Tick();
//
//			DBG( path.emplace_back( self.position ) );
//
//			if (self.health < hp && a.hitTick == -1) {
//				a.hitTick = tick;
//			}
//
//			bool isFloat = !self.onGround && (self.jumpState.maxTime < 1e-5 || !self.jumpState.canCancel);
//
//			if (isFloat) a.floatTicks++;
//
//			Vec2 selfCenter = self.position + Vec2( 0, 0.9 );
//			Vec2 enemyCenter = enemy.position + Vec2( 0, 0.9 );
//			Rect r = GetUnitRect( self );
//			Rect er = GetUnitRect( enemy );
//
//			//Vec2 center = GetUnitRect( self ).Center();
//			double targetDist = selfCenter.Dist2( target );
//			double enemyDist = selfCenter.Dist2( enemy.position );
//
//			if (Sqr( RaycastLevel( selfCenter, selfCenter.DirTo( target ) ).second ) < Sqr( targetDist ) && a.minTargetDist > targetDist) {
//				actions[i].minTargetDist = targetDist;
//			}
//			if (a.minEnemyDist > enemyDist) a.minEnemyDist = enemyDist;
//
//			double hcOwn = 0, hcSelf = 0, hcEnemy = 0;
//
//			if (sim.currentTick % 2 == 0) {
//				if (self.hasWeapon && self.weapon.fireTimer <= 0 + DBL_EPSILON) {
//					hcOwn = HitChance( er, selfCenter, dirToEnemy, self.weapon, 9 );
//					if (self.weapon.params.explosion.damage) {
//						hcSelf = HitChance( r, selfCenter, dirToEnemy, self.weapon, 5 );
//						if (selfCenter.Dist( enemyCenter ) - r.MaxRadius() * 2 < self.weapon.params.explosion.radius) {
//							hcSelf = 1.0;
//						}
//					}
//				}
//				if (enemy.hasWeapon && enemy.weapon.fireTimer <= 0 + DBL_EPSILON) {
//					hcEnemy = HitChance( r, enemyCenter, dirToSelf, enemy.weapon, 11 );
//					if (isFloat) hcEnemy = min( hcEnemy*1.2, 1.0 );
//				}
//			}
//
//			double maxDist = 40.;
//			enemyDist = sqrt( enemyDist );
//
//			a.hcOwn += hcOwn;
//			a.hcEnemy += hcEnemy;
//			a.hcSelf += hcOwn > 0 ? hcSelf : 0;
//			//a.hcOwn = max(hcOwn,a.hcOwn);
//			//a.hcEnemy = max( hcEnemy, a.hcEnemy );
//			//a.hcSelf = (hcOwn>0)?max(hcSelf,a.hcSelf):0;
//
//			//a.hcOwn *= 1 - enemyDist / maxDist;
//			//a.hcEnemy *= 1 - enemyDist / maxDist;
//			//a.hcSelf *=  1 - enemyDist / maxDist;
//		}
//
//		DBG( DrawPath( path ) );
//		DBG( debug.draw( CustomData::PlacedText( str( a.action ), path.back(), TextAlignment::LEFT, 20, ColorFloat( 0, 1, 0, 1 ) ) ); )
//
//			int tick = sim.currentTick / 2;
//
//		a.health = self.health;
//		a.hcOwn /= tick;
//		a.hcSelf /= tick;
//		a.hcEnemy /= tick;
//
//		a.finalState = self;
//
//		Vec2 dir = GetActionDir( action );
//
//		a.targetDir = targetMoveDir.Dot( dir ) > 0.5;
//		a.targetMove = targetMoveDir == dir;
//
//		a.finalPos = self.position;
//		a.enemyPos = enemy.position;
//
//		Rect selfRect = GetUnitRect( self );
//		Rect enemyRect = GetUnitRect( enemy );
//		Vec2 center = GetCenter( self );
//		Vec2 enemyCenter = GetCenter( enemy );
//
//		a.enemyVisible = RaycastLevel( center, center.DirTo( enemyCenter ), enemyRect ).first;
//		a.enemyDist = center.Dist( enemyCenter );
//
//		//a.pred1 = HitPredict( enemy, enemyCenter.DirTo( center ), self, false, 7 );
//		//a.pred2 = HitPredict( self, center.DirTo( enemyCenter ), enemy, false, 7 );
//
//		//a.minTargetDist = sqrt( a.minTargetDist );
//
//		a.ticks = sim.currentTick;
//	}
//
//	for (actionScore & a : actions) {
//		a.score = 0;
//		a.score += a.health * 100;
//		int targetScore = 5;
//
//		double edgeDist = min( a.finalPos.x, 40 - a.finalPos.x );
//		double wallDist1 = RaycastLevel( a.finalPos + Vec2( 0, 0.9 ), Vec2( 1, 0 ) ).second;
//		double wallDist2 = RaycastLevel( a.finalPos + Vec2( 0, 0.9 ), Vec2( -1, 0 ) ).second;
//		double wallDist = min( wallDist1, wallDist2 );
//
//		if (targetType == TG_HEALTH) targetScore = 20;
//		else if (targetType == TG_HEALTH2) targetScore = 5;
//		else if (targetType == TG_ENEMY) targetScore = 1;
//		else if (targetType == TG_WEAPON) targetScore = 20;
//		else if (targetType == TG_WEAPON2) targetScore = 10;
//
//		if (targetType == TG_ENEMY && !enemyVisible && a.enemyDist > 9) targetScore = 20;
//
//		if (a.targetDir) a.score += targetScore;
//		if (a.targetMove) a.score += targetScore;
//
//		//a.score += a.hcOwn * (!isStuck?5:20);
//		//a.score -= a.hcEnemy * (hcEnemy.hitChance > 0?15:5);
//		//a.score -= a.hcSelf * (!isStuck ? 8 : 20);
//		a.score += a.hcOwn * 15;
//		//a.score -= a.hcEnemy * (hcEnemy.hitChance > 0?50:20);
//		a.score -= a.hcSelf * 10;
//
//		a.score += a.enemyDist;
//
//		if (a.minEnemyDist < 3) a.score -= 50;
//
//		if (a.finalPos.y < a.enemyPos.y) a.score -= 30;
//		if (edgeDist < 7) a.score -= 10;
//		//if (a.floatTicks) a.score -= 20;
//	}
//
//	actionScore best = stdh::best( actions, []( const actionScore & a, const actionScore & b ) { return a.score > b.score; } );
//	//actionScore best = stdh::best( actions, []( const actionScore & a, const actionScore & b ) { return a.hcEnemy < b.hcEnemy; } );
//	actionScore worst = stdh::best( actions, []( const actionScore & a, const actionScore & b ) { return a.score < b.score; } );
//	//actionScore worst = stdh::best( actions, []( const actionScore & a, const actionScore & b ) { return a.hcEnemy > b.hcEnemy; } );
//
//#ifdef DEBUG
//	for (actionScore & a : actions) {
//		Vec2 dbgDir = GetActionDir( GetAction( a.action ) );
//		Vec2 dbgEndPos = GetCenter( unit ) + dbgDir * 3;
//		double f = (a.score - worst.score) / (double)(best.score - worst.score);
//		if (a.score == best.score) {
//			debug.drawLine( GetCenter( unit ), dbgEndPos, 0.1, ColorFloat( 1, 0, 0, 1 ) );
//		}
//		else {
//			debug.drawLine( GetCenter( unit ), dbgEndPos, 0.1, ColorFloat( 1, 1, 1, f ) );
//		}
//		debug.draw( CustomData::PlacedText( str( f ), Vec2Float( dbgEndPos.x, dbgEndPos.y ), TextAlignment::CENTER, 15, ColorFloat( 1, 1, 1, 1 ) ) );
//		debug.draw( CustomData::PlacedText( str( a.action ) + ": " + VARDUMP( a.score ) + VARDUMP( a.health ) + VARDUMP( a.hcOwn ) + VARDUMP( a.hcEnemy ) + VARDUMP( a.hcSelf ), Vec2Float( 43, 27 - a.action * 1 ), TextAlignment::RIGHT, 14, ColorFloat( 1, 1, 1, 1 ) ) );
//		//debug.draw( CustomData::PlacedText( str(a.action) + ": " + VARDUMP( a.score ) + VARDUMP( a.health ) + VARDUMP( a.pred1.hitChance2 ) + VARDUMP( a.pred2.hitChance2 ), Vec2Float(43,27 - a.action * 1), TextAlignment::RIGHT, 14, ColorFloat( 1, 1, 1, 1 ) ) );
//		//DrawPath( PredictPath2( unit, GetAction( a.action ), 60, game.units ) );
//	}
//	debug.print( VARDUMP( hcEnemy.hitChance ) + " " + VARDUMP2( "FireTimer", en.weapon.fireTimer ) + VARDUMP( isStuck ) );
//	//debug.drawLine( center, center + GetActionDir( targetMove ) * 3, 0.1, ColorFloat( 0, 1, 1, 1 ) );
//	debug.print( "Smart: " + VARDUMP( best.score ) + VARDUMP( best.health ) + VARDUMP( best.hcOwn ) + VARDUMP( best.hcEnemy ) + VARDUMP( best.hcSelf ) + VARDUMP( best.floatTicks ) );
//	//EstimateReach( unit, target, true );
//#endif
//
//	return GetAction( best.action );
//}


//UnitAction ShootingHelper( const Unit &unit, const UnitAction & a, const Unit & enemy, const Game &game, Debug &debug ) {
//	if (!unit.weapon) return UnitAction();
//
//	UnitAction action;
//
//	Rect r = GetUnitRect( unit );
//	Rect er = GetUnitRect( enemy );
//
//	Vec2 lastAim = Vec2( 1, 0 ).Rotate( unit.weapon->lastAngle + M_PI );
//	Vec2 aim = unit.position.DirTo( er.Center() );
//
//	double hcEnemy = HitChance( GetUnitRect(enemy), r.Center(), aim, *unit.weapon );
//	double hcSelf = HitChance( r, r.Center(), aim, *unit.weapon );
//
//	action.shoot = true;
//	action.aim = aim;
//
//	if (random01() > 1 - hcSelf) action.shoot = false;
//	if (hcSelf > 0 && aim.Len() > 2) action.shoot = false;
//	if (random01() > hcEnemy) action.shoot = false;
//	if (hcEnemy < 0.001) action.shoot = false;
//
//
//
//	//Vec2 left = aim.Rotate( Rad( 30 ) );
//	//Vec2 right = aim.Rotate( Rad( -30 ) );
//
//	//int num = 6;
//
//	//vector<Vec2> dirs;
//	//dirs.reserve( 20 );
//
//	//for (int i = 0; i < num; i++) {
//	//	Vec2 d = left.Slerped( i / (double)num, right );
//	//	dirs.emplace_back( d );
//	//}
//	//dirs.emplace_back( aim );
//	//dirs.emplace_back( lastAim );
//
//	//double maxHcAbs = 0;
//	//Vec2 bestVec = aim;
//
//	//Sim sim;
//	//sim.useAim = true;
//	//sim.units.resize( 2 );
//
//	//Weapon w = *unit.weapon;
//
//	//UnitAction enA = Dodge( 30, enemy, game, debug ).second;
//
//	//for (Vec2 d : dirs) {
//	//	sim.units[0] = unit;
//	//	sim.units[1] = enemy;
//	//	sim.units[1].action = enA;
//	//	sim.units[0].action = a;
//	//	sim.units[0].action.aim = d;
//	//	*sim.units[0].weapon = w;
//	//	double maxHc = 0;
//
//	//	for (int i = 0; i < 30; i++) {
//	//		Vec2 pos = sim.units[0].position;
//	//		pos.y += sim.units[0].size.y*0.5;
//	//		double hcEnemyD = HitChance( GetUnitRect( sim.units[1] ), pos, d, *unit.weapon, 10 );
//	//		double hcSelfD = HitChance( GetUnitRect( sim.units[0] ), pos, d, *unit.weapon, 10 );
//	//		double hc = hcEnemyD - hcSelfD;
//	//		if (hc > maxHc) maxHc = hc;
//	//		sim.Tick();
//	//	}
//	//	if (maxHc > maxHcAbs) {
//	//		maxHcAbs = maxHc;
//	//		bestVec = d;
//	//	}
//	//}
//
//	//*sim.units[0].weapon = w;
//
//	//action.aim = bestVec;
//
//	return action;
//}

//const Unit * FindClosestUnit( Vec2 from, const vector<Unit> & items, bool enemy, bool raycast = false ) {
//	const Unit * closest = nullptr;
//	double mindist = DBL_MAX;
//	for (const Unit & it : items) {
//		if (enemy && it.playerId == selfPlayer) continue;
//		if (raycast && RaycastLevel( from, from.DirTo( it.position ) ).first == false) continue;
//		double dist = from.Dist2( it.position + Vec2(0,it.size.y*0.5) );
//		if (dist < mindist) {
//			mindist = dist;
//			closest = &it;
//		}
//	}
//	return closest;
//}
//
//const LootBox * FindClosestLootbox( Vec2 from, const vector<LootBox> & items, int type, bool raycast = false ) {
//	const LootBox * closest = nullptr;
//	double mindist = DBL_MAX;
//	for (const LootBox & it : items) {
//		if (!it.item || it.item->type != type) continue;
//		if (raycast && RaycastLevel( from, from.DirTo( it.position + Vec2( 0, it.size.y*0.5 ) ) ).first == false) continue;
//		double dist = from.Dist2( it.position );
//		if (dist < mindist) {
//			mindist = dist;
//			closest = &it;
//		}
//	}
//	return closest;
//}
//
//const Mine * FindClosestMine( Vec2 from, const vector<Mine> & items, int type, bool raycast = false ) {
//	const Mine * closest = nullptr;
//	double mindist = DBL_MAX;
//	for (const Mine & it : items) {
//		if (raycast && RaycastLevel( from, from.DirTo( it.position + Vec2( 0, it.size.y*0.5 ) ) ).first == false) continue;
//		double dist = from.Dist2( it.position );
//		if (dist < mindist) {
//			mindist = dist;
//			closest = &it;
//		}
//	}
//	return closest;
//}

//int GetScore( const Unit & u, const Sim & s ) {
//	int score = 0;
//
//	const Unit * nearestEnemy = nullptr;
//	double enemyDist = DBL_MAX;
//	for (const Unit & other : s.units) {
//		if (other.playerId != u.playerId) {
//			double dist = distanceSqr( u.position, other.position );
//			if (nearestEnemy == nullptr || dist < enemyDist) {
//				enemyDist = dist;
//				nearestEnemy = &other;
//			}
//		}
//	}
//
//	const LootBox *nearestHealth = nullptr;
//	const LootBox *nearestWeapon = nullptr;
//	double weaponDist = DBL_MAX;
//	for (const LootBox &lootBox : s.lootBoxes) {
//		if (lootBox.item->type == 1) {
//			Item::Weapon weap = *std::dynamic_pointer_cast<Item::Weapon>(lootBox.item);
//			bool betterWeapon = !u.weapon || (u.weapon && IsBetterWeapon( u.weapon->type, weap.weaponType ));
//			double dist = distanceSqr( u.position, lootBox.position );
//			if (betterWeapon && dist < weaponDist) {
//				weaponDist = dist;
//				nearestWeapon = &lootBox;
//			}
//		}
//		if (lootBox.item->type == 0) {
//			if (nearestHealth == nullptr || distanceSqr( u.position, lootBox.position ) < distanceSqr( u.position, nearestHealth->position )) {
//				nearestHealth = &lootBox;
//			}
//		}
//	}
//
//	score += u.health * 100;
//
//	//score += (40 - (nearestWeapon->position.x - u.position.x))* (u.weapon?1:10);
//	//score += (30 - (nearestWeapon->position.y - u.position.y))* (u.weapon?1:10);
//
//	//if (nearestHealth && u.health < 100) {
//	//	score += (40 - (nearestHealth->position.x - u.position.x))* 1;
//	//	score += (30 - (nearestHealth->position.y - u.position.y))* 1;
//	//}
//
//	Vec2 targetPos = u.position;
//	if (!u.weapon && nearestWeapon) {
//		targetPos = nearestWeapon->position;
//	}
//	else if (u.health < props.unitMaxHealth && nearestHealth) {
//		targetPos = nearestHealth->position;
//	}
//	else if (nearestWeapon) {
//		targetPos = nearestWeapon->position;
//	}
//	else {
//	}
//
//	return score;
//}
//
//struct simProbe {
//	int score = 0;
//	int min = 0;
//	int max = 0;
//	int avg = 0;
//	int action[3];
//};
//
//UnitAction Smart( const Unit &unit, const Game &game, Debug &debug ) {
//
//	vector<simProbe> probes;
//
//	for (int i = 0; i < 9; i++) {
//		for (int j = 0; j < 9; j++) {
//			simProbe p;
//			p.action[0] = i;
//			p.action[1] = j;
//			p.action[2] = 0;
//			probes.emplace_back( p );
//		}
//	}
//
//	Sim sim;
//	sim.lootBoxes = game.lootBoxes;
//	sim.mines = game.mines;
//	sim.units.emplace_back( unit );
//	for (const Unit & u : game.units) {
//		if (u.id != unit.id) {
//			sim.units.emplace_back( u );
//		}
//	}
//	for (simProbe & p : probes) {
//		for (int i = 0; i < 2; i++) {
//			sim.units[0].action = GetAction( p.action[i] );
//			for (int tick = 0; tick < 60; tick++) {
//				sim.Tick();
//			}
//		}
//	}
//
//	simProbe best = stdh::best( probes, []( const simProbe & a, const simProbe & b ) { return a.score > b.score;  } );
//
//	return GetAction(best.action[0]);
//}
