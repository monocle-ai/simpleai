#include "GameEntity.h"
#include "GameMap.h"

namespace ai {
namespace example {

// returns a random start position within the boundaries
inline ai::Vector3f GameEntity::getStartPosition() const {
	const int size = _map->getSize();
	const int x = ai::random(-size, size);
	const float y = 0.0f;
	const int z = ai::random(-size, size);
	return ai::Vector3f(static_cast<float>(x), y, static_cast<float>(z));
}

GameEntity::GameEntity(const ai::CharacterId& id, const ai::example::GameMap* map) :
		ai::ICharacter(id), _map(map), _hitpoints(100), _damage(5), _attackDelay(500) {
	// pick some random start position
	setPosition(getStartPosition());
	setOrientation(ai::randomf(M_2PI));
	setAttribute(ai::attributes::NAME, "Example " + std::to_string(id));
	setSpeed(50.0f + ai::randomf(10.0f));

	setAttribute(ai::attributes::ID, std::to_string(getId()));
	setAttribute("Damage", std::to_string(_damage));
	setAttribute("Reloadtime", std::to_string(_attackDelay));
}

void GameEntity::update(long /*deltaTime*/, bool debuggingActive) {
	// cap position to the map
	const float sizeF = static_cast<float>(_map->getSize());
	const Vector3f& currentPos = _position;
	Vector3f newPos(currentPos);
	if (currentPos.x < -sizeF) {
		newPos.x = sizeF;
	} else if (currentPos.x > sizeF) {
		newPos.x = -sizeF;
	}
	if (currentPos.z < -sizeF) {
		newPos.z = sizeF;
	} else if (currentPos.z > sizeF) {
		newPos.z = -sizeF;
	}
	setPosition(newPos);

	// update attributes for debugging
	if (debuggingActive) {
		setAttribute(ai::attributes::POSITION, std::to_string(getPosition()));
		setAttribute("Hitpoints", std::to_string(getHitpoints()));
		setAttribute("Reloadtime", std::to_string(getAttackDelay()));
		setAttribute("Damage", std::to_string(getDamage()));
		setAttribute(ai::attributes::SPEED, std::to_string(getSpeed()));
		setAttribute(ai::attributes::ORIENTATION, std::to_string(toDegrees(getOrientation())));
	}
}

}
}
