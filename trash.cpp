
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
