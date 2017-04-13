#pragma once
#include "entity.h"
#include "engine.h"
#include "HUtils.h"
#include "globals.h"
#include "entityManager.h"
#include "entityBouncyBall.h"
#include "input.h"
#include "cameraController.h"
#include "entityInterop.h"
#include "entityWorld.h"
#include "collider.h"
#include "entityPhysicsObject.h"
#include <iomanip>
struct entPlayer : entPhysicsObject
{
	entPlayer()
	{

	}
	entPlayer(HUtils::XYZ pos_)
	{
		pos = pos_;
	}
	void simulatePhysics()
	{
		float airFriction = 1.01 * globals->timeDelta;
		float horizontalFriction = 1.01 * globals->timeDelta;

		float gravity = -globals->gravity * globals->timeDelta;

		onGround = 0;	

		// ground slope special cases //

		// find collision with ground
		trace ground = collider->findCollision(
			pos,
			HUtils::XYZ(0, -1, 0),
			dynamic_cast<entWorld*>(entityInterop->getWorld())->mesh);

		float prevYVel = vel.y; //calculate the difference in y velocity before and after we hit the floor
		if (ground.didHit && !ground.traceFailed)
		{
			float groundAngle = abs(90 - (asin(ground.hitNormal.y) * 180.0f / 3.141592));
			//cout << "groundAngle: " << std::fixed <<  groundAngle << "\r\n";

			if (groundAngle > 2 && 
				groundAngle < 90 && 
				ground.distance <= playerHeight + 0.1) /*Standard slope we can walk up*/
			{
				vel.y = 0;
				pos.y = ground.hitPos.y + playerHeight;
				onGround = 1;				
			}
			//if (ground.distance <= playerHeight + 0.01 && groundAngle >= 45) /*slide off*/
			//{
			//	gravity /= 5.0f;
			//	vel.x += ground.hitNormal.x * gravity * -3.6;
			//	vel.z += ground.hitNormal.z * gravity * -3.6;
			//	pos.y = ground.hitPos.y + playerHeight;
			//}
			else if (ground.distance <= playerHeight)
			{
				vel.y = 0;
				pos.y = ground.hitPos.y + playerHeight;
				onGround = 1;
			}
		}

		// very weird when jumping on slopes
		//camera->pitchAdditive -= (prevYVel - vel.y) * 250; //apply a pitch additive based on how much our velocity changed to shake our crosshair

		//float potentialFovAdd = vel.magnitude() * 1000.6; /*The faster the player is going, the more zoomed out he is*/
		//if (potentialFovAdd > camera->fovAdditive)
		//	camera->fovAdditive = potentialFovAdd;


		// wall collision //

		// find collision with wall
		trace wall = collider->findCollision(
			pos + HUtils::XYZ(0,0.1,0), 
			vel.normalized(), 
			dynamic_cast<entWorld*>(entityInterop->getWorld())->mesh);

		if (wall.didHit && !wall.traceFailed && wall.distance < 0.5)
		{
			HUtils::XYZ hozVel = HUtils::XYZ(vel.x, 0, vel.z);
			vel.x += wall.hitNormal.x * hozVel.magnitude();
			vel.y += wall.hitNormal.y * hozVel.magnitude();
			vel.z += wall.hitNormal.z * hozVel.magnitude();
		}
		
		if (!onGround)
		{
			//horizontalFriction = airFriction;
		}
		else // jumping
		{
			if (input->keyboard->keyDown(VK_SPACE))
			{
				//pos.y += 0.1;
				//vel.y += 0.005;
				acc.y += 0.01;
			}
		}
		vel += HUtils::XYZ(0, gravity, 0);
		
		//float maxSpeed = 0.01;

		/*If we're moving don't apply the same amount of ground friction*/
		//if (movVel.magnitude() > 0.003)
		//{
		//	horizontalFriction /= 3;
		//}

		//if (vel.magnitude() > maxSpeed && movVel.magnitude() >= 0.00001)
		//{
		//	movVel = movVel.normalized() * maxSpeed * -movVel.magnitude();
		//}
		//

		vel += acc;
		acc *= 0;

		vel.x /= horizontalFriction;
		vel.z /= horizontalFriction;
		
		// new velocity cap //
		float maxHoriSpeed = 0.01;
		HUtils::XYZ horiVel(vel.x, 0, vel.z);
		float currentSpeed = horiVel.magnitude();
		if (currentSpeed > maxHoriSpeed)
		{
			horiVel = horiVel.normalized() * maxHoriSpeed;
			vel.x = horiVel.x;
			vel.z = horiVel.z;
		}

		//cout << vel.magnitude() << endl;

		pos += vel;
	}
	void tick()
	{
		float jumpYOffset = 0.1f;
		float gravity = -globals->gravity * globals->timeDelta;
		onGround = 0;

		// floor collision //
		trace ground = collider->findCollision(
			pos,
			HUtils::XYZ(0, -1, 0),
			dynamic_cast<entWorld*>(entityInterop->getWorld())->mesh);
		float prevYVel = vel.y; //calculate the difference in y velocity before and after we hit the floor
		if (ground.didHit && !ground.traceFailed)
		{
			if (ground.hitNormal.y < 0.95f && ground.distance < playerHeight + jumpYOffset)
			{
				vel.y = 0;
				pos.y = ground.hitPos.y + playerHeight;
				onGround = 1;
				if (ground.hitNormal.y < 0.8f)
				{
					vel.x += ground.hitNormal.x * gravity * -ground.hitNormal.y;
					vel.z += ground.hitNormal.z * gravity * -ground.hitNormal.y;
					pos.y = ground.hitPos.y + playerHeight;
				}
			}
			else if (ground.distance < playerHeight)
			{
				vel.y = 0;
				pos.y = ground.hitPos.y + playerHeight;
				onGround = 1;
			}
		}
		


		// player movement inputs ///
		float energyFactor = 0.0015;
		float appliedEnergy = globals->timeDelta * energyFactor;
		if (onGround)
		{
			if (input->keyboard->keyDown(VK_SPACE))
			{
				pos.y += jumpYOffset;
				acc.y += 0.008;
			}
			if (input->keyboard->keyDown('W'))
			{
				//pos = pos + (camera->normalizedLookDir * HUtils::XYZ(1,0,1)).normalized() * HUtils::XYZ(speedMultiplier, 0, speedMultiplier);
				acc += (camera->normalizedLookDir * HUtils::XYZ(1, 0, 1)).normalized() * HUtils::XYZ(appliedEnergy, 0, appliedEnergy);
			}
			if (input->keyboard->keyDown('S'))
			{
				acc += (camera->normalizedLookDir * HUtils::XYZ(1, 0, 1)).normalized() * HUtils::XYZ(-appliedEnergy, 0, -appliedEnergy);
			}
			if (input->keyboard->keyDown('A'))
			{
				//pos = pos + yRotate(camera->normalizedLookDir, 270) * HUtils::XYZ(speedMultiplier, speedMultiplier, speedMultiplier);
				acc += (yRotate(camera->normalizedLookDir, 270) * HUtils::XYZ(1, 0, 1)).normalized() * HUtils::XYZ(appliedEnergy, 0, appliedEnergy);
			}
			if (input->keyboard->keyDown('D'))
			{
				acc += (yRotate(camera->normalizedLookDir, 90) * HUtils::XYZ(1, 0, 1)).normalized() * HUtils::XYZ(appliedEnergy, 0, appliedEnergy);
			}
		}

		// other key inputs //
		// exit
		if (input->keyboard->keyDownEvent('E'))
		{
			exit(0);
		}
		// new bouncy ball
		if (input->keyboard->keyDownEvent('F'))
		{
			for(int i = 0; i < 1; i++)
				entityManager->addEntity(new entBounceyBall(HUtils::XYZ(0,0,0), HUtils::randVec()*0.1));
		}
		// print all entities
		if (input->keyboard->keyDownEvent('G'))
		{
			int s = entityInterop->getEntityCount();
			for (int i = 0; i < s; i++)
			{
				cout << i << ": " << entityInterop->findEntity(i)->getClass() << endl;
			}
			Sleep(1000);
		}
		// toggle mouse lock
		if (input->keyboard->keyDownEvent('Q'))
		{
			camera->lockMouse = !camera->lockMouse;
		}
		// lmao jet pack
		if (input->keyboard->keyDown(VK_LBUTTON))
		{
			vel += camera->normalizedLookDir.normalized() * -0.00004;	
		}
		// crouch
		if (input->keyboard->keyDown(VK_CONTROL)) //crouched
		{
			crouched = 1;
			targetPlayerHeight = 0.8;
		}
		else
		{
			crouched = 0;
			targetPlayerHeight = 1.4;
		}
		


		// wall collision //
		float wallDistOffset = 0.1f;
		trace wall = collider->findCollision(
			pos + HUtils::XYZ(0, 0.1f, 0),
			vel.normalized(),
			dynamic_cast<entWorld*>(entityInterop->getWorld())->mesh);
		if (wall.didHit && !wall.traceFailed && wall.distance < wallDistOffset)
		{
			HUtils::XYZ u(vel.x, 0, vel.y);
			HUtils::XYZ v(-wall.hitNormal.z, 0, wall.hitNormal.x);
			float vm = v.magnitude();
			float t = (u.x * v.x + u.z * v.z) / (vm*vm);
			vel = v * t;
			pos += wall.hitNormal * wallDistOffset;
		}


		// end applies //
		vel += HUtils::XYZ(0, gravity, 0);
		vel += acc;
		acc *= 0;

		float vertFriction = 1.0001;
		float horiFriction = 1.0001;
		float groundFriction = 1.02;
		
		if (onGround)
			horiFriction = groundFriction;
		vel.x /= horiFriction;
		vel.z /= horiFriction;
		vel.y /= vertFriction;


		// new velocity cap //
		float maxHoriSpeed = 0.014;
		HUtils::XYZ horiVel(vel.x, 0, vel.z);
		float currentSpeed = horiVel.magnitude();
		if (currentSpeed > maxHoriSpeed)
		{
			horiVel = horiVel.normalized() * maxHoriSpeed;
			vel.x = horiVel.x;
			vel.z = horiVel.z;
		}

		cout << vel.magnitude() << endl;

		pos += vel;

		playerHeight += ((targetPlayerHeight - playerHeight) / 10.0f) * globals->timeDelta;

		globals->cX = pos.x;
		globals->cY = pos.y;
		globals->cZ = pos.z;

		globals->cVX = vel.x;
		globals->cVY = vel.y;
		globals->cVZ = vel .z;

		//pos.print();
	}
	void render()
	{

	}
	virtual std::string getClass()
	{
		return "entPlayer";
	}
	float playerHeight = 1.4;
	float standingPlayerHeight = 1.4;
	float targetPlayerHeight = 1;
	bool onGround = 0;
	bool crouched = 0;
	HUtils::XYZ acc;
};