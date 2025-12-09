#pragma once

#include "SystemManager.h"
#include "Scene.h"
#include "CameraComponent.h"
#include "LocationComponent.h"
#include "WindowManager.h"
#include <cmath>
#include <random>
#include <unordered_map>

namespace ECSEngine
{

	template <typename... Components>
	class CameraSystem : public System<Components...>
	{
	private:
		// Per-camera look-ahead state
		std::unordered_map<EntityID, float> mLookAheadState;

	public:
		bool Run(Scene<Components...> &scene) override
		{
			auto &entityManager = scene.GetEntityManager();
			auto &windowManager = scene.GetWindowManager();
			float deltaTime = 1.0f / 60.0f;

			for (auto it = entityManager.begin(); it != entityManager.end(); ++it)
			{
				if (!it->isActive())
					continue;
				EntityID entityId = it->getID();

				if (entityManager.template HasComponent<CameraFollower>(entityId))
				{
					auto &cameraFollower = entityManager.template GetComponent<CameraFollower>(entityId);
					EntityID trackedEntityId = cameraFollower.entityId;

					// Find camera entity
					for (auto camIt = entityManager.begin(); camIt != entityManager.end(); ++camIt)
					{
						if (!camIt->isActive())
							continue;
						EntityID camEntityId = camIt->getID();

						if (entityManager.template HasComponent<CameraComponent>(camEntityId))
						{
							auto &cameraComp = entityManager.template GetComponent<CameraComponent>(camEntityId);

							if (entityManager.template HasComponent<LocationComponent>(trackedEntityId))
							{
								auto &trackedLocation = entityManager.template GetComponent<LocationComponent>(trackedEntityId);

								sf::RenderWindow *window = windowManager.GetWindow();
								sf::Vector2u windowSize = window->getSize();
								float windowWidth = static_cast<float>(windowSize.x);

								float playerWindowX = (trackedLocation.position.x - cameraComp.position.x) / cameraComp.scale + windowWidth * 0.5f;
								float playerXPercent = playerWindowX / windowWidth;

								// Get movement direction and speed
								bool isMoving = false;
								float moveDirection = 0.0f; // -1 for left, 1 for right, 0 for stopped
								if (entityManager.template HasComponent<MovementComponent>(trackedEntityId))
								{
									auto &movementComp = entityManager.template GetComponent<MovementComponent>(trackedEntityId);
									isMoving = std::abs(movementComp.velocity.x) > 0.1f;
									if (isMoving)
									{
										moveDirection = (movementComp.velocity.x > 0.0f) ? 1.0f : -1.0f;
									}
								}

								// Look-ahead parameters
								const float lookAheadDistance = 120.0f; // World units to look ahead (reduced for smoother feel)
								const float lookAheadSmoothing = 5.0f;	// Smoothing speed for look-ahead (slower for smoother transitions)

								// Calculate look-ahead offset based on movement direction (per-camera)
								// Only apply look-ahead when moving
								float &currentLookAhead = mLookAheadState[camEntityId];
								float targetLookAhead = isMoving ? (moveDirection * lookAheadDistance) : 0.0f;
								float lookAheadDelta = targetLookAhead - currentLookAhead;
								float lerpFactor = 1.0f - std::exp(-lookAheadSmoothing * deltaTime);
								currentLookAhead += lookAheadDelta * lerpFactor;

								float targetCameraX = cameraComp.position.x;
								float trackingSpeed = 0.0f;

								// Hard zones: outer 5% (smaller than before)
								if (playerXPercent < 0.05f)
								{
									targetCameraX = trackedLocation.position.x - (windowWidth * 0.05f - windowWidth * 0.5f) * cameraComp.scale;
									trackingSpeed = 0.0f;
								}
								else if (playerXPercent > 0.95f)
								{
									targetCameraX = trackedLocation.position.x - (windowWidth * 0.95f - windowWidth * 0.5f) * cameraComp.scale;
									trackingSpeed = 0.0f;
								}
								// Soft zones: 5-35% (left) and 65-95% (right)
								// Apply reduced look-ahead in soft zones for smoother transitions
								else if (playerXPercent < 0.35f)
								{
									targetCameraX = trackedLocation.position.x - (windowWidth * 0.35f - windowWidth * 0.5f) * cameraComp.scale;
									// Apply reduced look-ahead (50%) in soft zones for smooth transition
									if (isMoving)
									{
										targetCameraX += currentLookAhead * 0.5f;
									}
									trackingSpeed = 5.0f;
								}
								else if (playerXPercent > 0.65f)
								{
									targetCameraX = trackedLocation.position.x - (windowWidth * 0.65f - windowWidth * 0.5f) * cameraComp.scale;
									// Apply reduced look-ahead (50%) in soft zones for smooth transition
									if (isMoving)
									{
										targetCameraX += currentLookAhead * 0.5f;
									}
									trackingSpeed = 5.0f;
								}
								// Dead zone: center 30% (35-65%, smaller than before)
								// Apply look-ahead only in dead zone when moving
								else
								{
									if (!isMoving)
									{
										float centerWindowX = windowWidth * 0.5f;
										float distanceFromCenter = playerWindowX - centerWindowX;

										if (std::abs(distanceFromCenter) > windowWidth * 0.15f) // Dead zone threshold smaller (was 0.2f)
										{
											targetCameraX = trackedLocation.position.x;
											trackingSpeed = 3.0f;
										}
										else
										{
											targetCameraX = cameraComp.position.x;
										}
									}
									else
									{
										// In dead zone and moving - apply look-ahead smoothly
										targetCameraX = trackedLocation.position.x + currentLookAhead;
										trackingSpeed = 4.0f; // Smooth follow with look-ahead
									}
								}

								float cameraDeltaX = targetCameraX - cameraComp.position.x;
								if (std::abs(cameraDeltaX) > 0.01f)
								{
									if (trackingSpeed == 0.0f)
									{
										cameraComp.position.x = targetCameraX;
									}
									else
									{
										float lerpFactor = 1.0f - std::exp(-trackingSpeed * deltaTime);
										cameraComp.position.x += cameraDeltaX * lerpFactor;
									}
								}

								// Follow player on Y axis with smooth interpolation
								float targetCameraY = trackedLocation.position.y;
								float cameraDeltaY = targetCameraY - cameraComp.position.y;
								if (std::abs(cameraDeltaY) > 0.01f)
								{
									float yTrackingSpeed = 5.0f;
									float lerpFactorY = 1.0f - std::exp(-yTrackingSpeed * deltaTime);
									cameraComp.position.y += cameraDeltaY * lerpFactorY;
								}

								if (entityManager.template HasComponent<CameraShake>(camEntityId))
								{
									auto &shake = entityManager.template GetComponent<CameraShake>(camEntityId);

									static std::random_device rd;
									static std::mt19937 gen(rd());
									std::uniform_real_distribution<float> shakeDist(-1.0f, 1.0f);

									float shakeX = shakeDist(gen) * shake.magnitude.x;
									float shakeY = shakeDist(gen) * shake.magnitude.y;

									cameraComp.position.x += shakeX;
									cameraComp.position.y += shakeY;

									shake.framesRemaining--;
									if (shake.framesRemaining <= 0)
									{
										entityManager.template RemoveComponent<CameraShake>(camEntityId);
									}
								}

								windowManager.SetCamera(cameraComp.position);
								windowManager.SetWorldScale(cameraComp.scale);
							}
							break;
						}
					}
				}
			}

			return true;
		}
	};

} // namespace ECSEngine
