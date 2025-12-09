#pragma once

#include "SystemManager.h"
#include "Scene.h"
#include "SpellComponent.h"
#include "ProjectileComponent.h"
#include "InputComponent.h"
#include "LocationComponent.h"
#include "CollisionComponent.h"
#include "SpriteComponent.h"
#include "SoundManager.h"
#include <unordered_map>
#include <array>
#include <random>
#include <iostream>

namespace ECSEngine
{

    template <typename... Components>
    class SpellSystem : public System<Components...>
    {
    public:
        bool Run(Scene<Components...> &scene) override
        {
            auto &entityManager = scene.GetEntityManager();
            auto &soundManager = scene.GetSoundManager();
            float deltaTime = 1.0f / 60.0f;

            for (auto it = entityManager.begin(); it != entityManager.end(); ++it)
            {
                if (!it->isActive())
                    continue;
                EntityID entityId = it->getID();

                // Only process entities with SpellComponent and InputComponent
                if (!entityManager.template HasComponent<SpellComponent>(entityId) ||
                    !entityManager.template HasComponent<InputComponent>(entityId))
                    continue;

                auto &spellComp = entityManager.template GetComponent<SpellComponent>(entityId);
                auto &inputComp = entityManager.template GetComponent<InputComponent>(entityId);

                // Update cooldowns
                UpdateCooldowns(spellComp, deltaTime);

                // Update facing direction based on movement input
                UpdateFacingDirection(spellComp, inputComp);

                // Process element selection (1, 2, 3, 4 keys)
                ProcessElementSelection(spellComp, inputComp, entityId, soundManager);

                // Process spell casting (E key or left mouse button equivalent)
                ProcessSpellCast(scene, entityId, spellComp, inputComp, soundManager);
            }

            return true;
        }

    private:
        // Track previous key states for edge detection
        std::unordered_map<EntityID, std::array<bool, 4>> mPrevSelectKeyStates;
        std::unordered_map<EntityID, bool> mPrevCastKeyState;

        // Update all cooldowns
        void UpdateCooldowns(SpellComponent &spellComp, float deltaTime)
        {
            if (spellComp.castCooldown > 0.0f)
                spellComp.castCooldown -= deltaTime;

            if (spellComp.switchCooldown > 0.0f)
                spellComp.switchCooldown -= deltaTime;
        }

        // Check if can cast selected spell
        bool CanCast(const SpellComponent &spellComp)
        {
            return spellComp.castCooldown <= 0.0f;
        }

        // Check if can switch elements
        bool CanSwitch(const SpellComponent &spellComp)
        {
            return spellComp.switchCooldown <= 0.0f;
        }

        // Start cast cooldown for current spell
        void StartCastCooldown(SpellComponent &spellComp)
        {
            size_t idx = static_cast<size_t>(spellComp.selectedSpell);
            spellComp.castCooldown = spellComp.spellProperties[idx].cooldown;
        }

        // Start switch cooldown
        void StartSwitchCooldown(SpellComponent &spellComp)
        {
            spellComp.switchCooldown = spellComp.switchCooldownDuration;
        }

        // Get spell properties for selected spell
        const SpellProperties &GetSelectedProperties(const SpellComponent &spellComp)
        {
            return spellComp.spellProperties[static_cast<size_t>(spellComp.selectedSpell)];
        }

        // Update facing direction based on input
        void UpdateFacingDirection(SpellComponent &spellComp, const InputComponent &inputComp)
        {
            sf::Keyboard::Scancode scancodeA = sf::Keyboard::delocalize(sf::Keyboard::Key::A);
            sf::Keyboard::Scancode scancodeD = sf::Keyboard::delocalize(sf::Keyboard::Key::D);

            bool movingLeft = (scancodeA != sf::Keyboard::Scan::Unknown &&
                               static_cast<size_t>(scancodeA) < sf::Keyboard::ScancodeCount)
                                  ? inputComp.keydown[static_cast<size_t>(scancodeA)]
                                  : false;

            bool movingRight = (scancodeD != sf::Keyboard::Scan::Unknown &&
                                static_cast<size_t>(scancodeD) < sf::Keyboard::ScancodeCount)
                                   ? inputComp.keydown[static_cast<size_t>(scancodeD)]
                                   : false;

            if (movingLeft && !movingRight)
                spellComp.facingDirection = -1;
            else if (movingRight && !movingLeft)
                spellComp.facingDirection = 1;
        }

        // Process element selection keys (1, 2, 3, 4)
        void ProcessElementSelection(SpellComponent &spellComp, const InputComponent &inputComp,
                                     EntityID entityId, SoundManager &soundManager)
        {
            sf::Keyboard::Scancode scancode1 = sf::Keyboard::delocalize(sf::Keyboard::Key::Num1);
            sf::Keyboard::Scancode scancode2 = sf::Keyboard::delocalize(sf::Keyboard::Key::Num2);
            sf::Keyboard::Scancode scancode3 = sf::Keyboard::delocalize(sf::Keyboard::Key::Num3);
            sf::Keyboard::Scancode scancode4 = sf::Keyboard::delocalize(sf::Keyboard::Key::Num4);

            auto &prevStates = mPrevSelectKeyStates[entityId];

            std::array<sf::Keyboard::Scancode, 4> selectKeys = {scancode1, scancode2, scancode3, scancode4};
            std::array<SpellType, 4> spellTypes = {SpellType::Fire, SpellType::Water, SpellType::Wind, SpellType::Earth};
            std::array<std::string, 4> spellNames = {"Fire", "Water", "Wind", "Rock"}; // Debug

            for (size_t i = 0; i < 4; ++i)
            {
                bool keyPressed = (selectKeys[i] != sf::Keyboard::Scan::Unknown &&
                                   static_cast<size_t>(selectKeys[i]) < sf::Keyboard::ScancodeCount)
                                      ? inputComp.keydown[static_cast<size_t>(selectKeys[i])]
                                      : false;

                // Switch element on key press (not hold), only if different element and can switch
                if (keyPressed && !prevStates[i])
                {
                    if (spellComp.selectedSpell != spellTypes[i] && CanSwitch(spellComp))
                    {
                        spellComp.selectedSpell = spellTypes[i];
                        StartSwitchCooldown(spellComp);
                        PlaySwitchSound(spellTypes[i], soundManager);
                        std::cout << "Element switched to: " << spellNames[i] << " (key " << (i + 1) << ")" << std::endl;
                    }
                }

                prevStates[i] = keyPressed;
            }
        }

        // Process spell casting (J key)
        void ProcessSpellCast(Scene<Components...> &scene, EntityID entityId,
                              SpellComponent &spellComp, const InputComponent &inputComp,
                              SoundManager &soundManager)
        {
            sf::Keyboard::Scancode scancodeE = sf::Keyboard::delocalize(sf::Keyboard::Key::J);

            bool castKeyPressed = (scancodeE != sf::Keyboard::Scan::Unknown &&
                                   static_cast<size_t>(scancodeE) < sf::Keyboard::ScancodeCount)
                                      ? inputComp.keydown[static_cast<size_t>(scancodeE)]
                                      : false;

            bool &prevCastState = mPrevCastKeyState[entityId];

            // Cast spell on key press (not hold)
            if (castKeyPressed && !prevCastState && CanCast(spellComp))
            {
                CastSpell(scene, entityId, spellComp, soundManager);
            }

            prevCastState = castKeyPressed;
        }

        // Create a projectile entity for the selected spell
        void CastSpell(Scene<Components...> &scene, EntityID casterId,
                       SpellComponent &spellComp, SoundManager &soundManager)
        {
            auto &entityManager = scene.GetEntityManager();

            // Get caster position
            if (!entityManager.template HasComponent<LocationComponent>(casterId))
                return;

            auto &casterLoc = entityManager.template GetComponent<LocationComponent>(casterId);
            const SpellProperties &props = GetSelectedProperties(spellComp);

            // Calculate spawn position (offset from caster based on facing direction)
            float spawnOffsetX = spellComp.facingDirection * 20.0f;
            Point2D spawnPos = {
                casterLoc.position.x + spawnOffsetX,
                casterLoc.position.y + 8.0f
            };

            // Create projectile entity
            std::string spellName = GetSpellName(spellComp.selectedSpell);
            EntityID projectileId = entityManager.CreateEntity(spellName);

            // Add LocationComponent
            entityManager.template AddComponent<LocationComponent>(projectileId, LocationComponent(spawnPos));

            // Add MovementComponent with velocity based on facing direction
            MovementComponent movement;
            movement.velocity = Point2D(spellComp.facingDirection * props.speed, 0.0f);
            entityManager.template AddComponent<MovementComponent>(projectileId, movement);

            // Add ProjectileComponent
            ProjectileComponent projectile;
            projectile.spellType = spellComp.selectedSpell;
            projectile.damage = props.damage;
            projectile.lifetime = props.lifetime;
            projectile.maxLifetime = props.lifetime;
            projectile.ownerEntityId = casterId;
            projectile.active = true;
            entityManager.template AddComponent<ProjectileComponent>(projectileId, projectile);

            // Add CollisionComponent 
            // Adjust collision box position based on facing direction
            // When facing right (not flipped), offset left to align with sprite
            float collisionOffsetX = (spellComp.facingDirection > 0) ? -props.size : 0.0f;
            Rect collisionBounds(collisionOffsetX, 0.0f, props.size, props.size);
            CollisionComponent collision(collisionBounds, false);
            entityManager.template AddComponent<CollisionComponent>(projectileId, collision);

            // Add SpriteComponent
            SpriteComponent sprite;
            sprite.spriteId = props.spriteId;
            sprite.bounds = Rect(0.0f, 0.0f, props.size, props.size);
            sprite.inWorldSpace = true;
            sprite.flipX = (spellComp.facingDirection < 0);
            entityManager.template AddComponent<SpriteComponent>(projectileId, sprite);

            // Start cast cooldown
            StartCastCooldown(spellComp);

            // Play cast sound
            PlayCastSound(spellComp.selectedSpell, soundManager);
        }

        // Get spell name string
        std::string GetSpellName(SpellType type)
        {
            switch (type)
            {
            case SpellType::Fire:
                return "fire_spell";
            case SpellType::Water:
                return "water_spell";
            case SpellType::Wind:
                return "wind_spell";
            case SpellType::Earth:
                return "earth_spell";
            default:
                return "spell";
            }
        }

        // Play switch sound when changing elements
        void PlaySwitchSound(SpellType type, SoundManager &soundManager)
        {
            switch (type)
            {
            case SpellType::Fire:
                soundManager.PlaySound("fire_select");
                break;
            case SpellType::Water:
                soundManager.PlaySound("water_select");
                break;
            case SpellType::Wind:
                soundManager.PlaySound("wind_select");
                break;
            case SpellType::Earth:
                soundManager.PlaySound("earth_select");
                break;
            default:
                break;
            }
        }

        // Play cast sound based on spell type
        void PlayCastSound(SpellType type, SoundManager &soundManager)
        {
            switch (type)
            {
            case SpellType::Fire:
                // Randomly pick between two fire whoosh sounds for variety
                {
                    static std::mt19937 rng(std::random_device{}());
                    std::uniform_int_distribution<int> dist(1, 2);
                    std::string soundName = "fire_cast_" + std::to_string(dist(rng));
                    soundManager.PlaySound(soundName);
                }
                break;
            case SpellType::Water:
                soundManager.PlaySound("water_cast");
                break;
            case SpellType::Wind:
                soundManager.PlaySound("wind_cast");
                break;
            case SpellType::Earth:
                soundManager.PlaySound("earth_cast");
                break;
            default:
                break;
            }
        }
    };

} // namespace ECSEngine
