#pragma once

#include "SystemManager.h"
#include "Scene.h"
#include "CameraComponent.h"
#include "LocationComponent.h"
#include "WindowManager.h"
#include <cmath>
#include <random>

namespace ECSEngine
{

	template <typename... Components>
	class CameraSystem : public System<Components...>
	{
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

								bool isMoving = false;
								if (entityManager.template HasComponent<MovementComponent>(trackedEntityId))
								{
									auto &movementComp = entityManager.template GetComponent<MovementComponent>(trackedEntityId);
									isMoving = std::abs(movementComp.velocity.x) > 0.1f;
								}

								float targetCameraX = cameraComp.position.x;
								float trackingSpeed = 0.0f;

								if (playerXPercent < 0.1f)
								{
									targetCameraX = trackedLocation.position.x - (windowWidth * 0.1f - windowWidth * 0.5f) * cameraComp.scale;
									trackingSpeed = 0.0f;
								}
								else if (playerXPercent > 0.9f)
								{
									targetCameraX = trackedLocation.position.x - (windowWidth * 0.9f - windowWidth * 0.5f) * cameraComp.scale;
									trackingSpeed = 0.0f;
								}
								else if (playerXPercent < 0.3f)
								{
									targetCameraX = trackedLocation.position.x - (windowWidth * 0.3f - windowWidth * 0.5f) * cameraComp.scale;
									trackingSpeed = 5.0f;
								}
								else if (playerXPercent > 0.7f)
								{
									targetCameraX = trackedLocation.position.x - (windowWidth * 0.7f - windowWidth * 0.5f) * cameraComp.scale;
									trackingSpeed = 5.0f;
								}
								else
								{
									if (!isMoving)
									{
										float centerWindowX = windowWidth * 0.5f;
										float distanceFromCenter = playerWindowX - centerWindowX;

										if (std::abs(distanceFromCenter) > windowWidth * 0.2f)
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
										targetCameraX = cameraComp.position.x;
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

								const float levelY = 512.0f;
								cameraComp.position.y = levelY;

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
