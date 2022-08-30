#pragma once

#include<vector>

#include<glm/glm.hpp>
#include<glm/gtc/matrix_transform.hpp>
#include<glm/gtc/type_ptr.hpp>

#include<Eigen/dense>

#include "shader.h"


using namespace glm;
using namespace std;

extern Camera camera;
extern const int SCR_WIDTH;
extern const int SCR_HEIGHT;

extern float deltaTime;
float elapsedTime;	// deltaTime이 지났을 때 다음 scene이 정해지는 데 그 scene을 정하기 위해서 중간 시점의 obj의 motion들을 정해주기 위한 그 시점. 


class Rigidbody;		// TODO is it valid?? in collision we need rigidbody* but rigidbody is not declared so. i write this sentence...


struct Collision {
	Rigidbody* with; // id of counterpart
	float occurence;		// time when collision occurs
};

class Rigidbody {
protected:
	vec3 position;
	unsigned int VBO,EBO,VAO;
	Shader* shader;
	unsigned int type; // 0 wall, 1 ball...
private:
public:
	unsigned int id;
	Rigidbody() {  };
	virtual ~Rigidbody() {};
	virtual void setupDraw()=0;
	virtual void draw()=0;
	virtual void update(float dt) = 0;
	virtual Collision collision(vector<Rigidbody*>)=0;
	virtual float whenCollide(Rigidbody*) = 0;		// return time when collision occur. 0<= t <= deltaTime. if not occur return -1.... it is part of collision detection.
	virtual void collisionResolve(Collision) =0;

	virtual vec3 getPos() = 0;
	virtual vec3 getVel() = 0;
	virtual vec3 getAcc() = 0;
	virtual unsigned int getType() = 0;
};


class Ball : public Rigidbody {
private:
	vec3 position;			// m
	vec3 velocity;			// m/s
	vec3 acceleration;		// m/s^2
	vec3 netForce;

	float density;			// kg/m^3
	float radius;			// m
	float mass;

	int nBreak;
	
	vec3 color;

	//unsigned int VBO,VAO;

public:
	Ball(vec3 pos, vec3 vel, vec3 acc,float density = 582,float radius = 0.5f, int nBreak = 5,vec3 color = vec3(1,1,1)) {
		this->radius = radius;
		this->nBreak = nBreak;

		this->position = pos;
		this->velocity = vel;
		this->acceleration = acc;
		this->density = density;
		this->mass = radius * radius * density * 3.14159f;
		this->netForce = mass * acceleration;

		this->type = 1;

		this->color = color;

		shader = new Shader("src/shader/ball_vs.txt", "src/shader/ball_fs.txt");
	}
	~Ball() {
		delete this->shader;
	}
	void setupDraw() {

		float r = radius;
		vector<vec3> posArr; // calculating vertex...
		for (int x = nBreak; x >= 0; x--) {
			for (int y = 0; y <= nBreak - x; y++) {
				int z = nBreak - y - x;
				vec3 vert = normalize(vec3(x, y, z)) * r;
				posArr.push_back(vert);
			}
		}
		// indexing algorithm

		vector<unsigned int> sphIndices;
		for (int line = 0; line < nBreak; line++) { // except with last line.
			for (int startIdx = ((line + 1) * line) / 2; startIdx < ((line + 2) * (line + 1)) / 2; startIdx++) {
				int idx2, idx3;		
				idx3 = startIdx + line + 1;
				idx2 = idx3 + 1;
				sphIndices.push_back(startIdx);
				sphIndices.push_back(idx2);
				sphIndices.push_back(idx3);
			}
		}
		for (int line = 2; line < nBreak + 1; line++) { //only with last line 
			for (int startIdx = ((line + 1) * line) / 2 + 1; startIdx < ((line + 2) * (line + 1)) / 2 - 1; startIdx++) {
				//각 line의 맨 끝 부분을 꼭지점으로 하는 역삼각형은 존재하지 않음.
				int idx2, idx3; // 아래쪽이 idx2.
				idx3 = startIdx - line;
				idx2 = idx3 - 1;
				sphIndices.push_back(startIdx);
				sphIndices.push_back(idx2);
				sphIndices.push_back(idx3);
			}
		}
		//we can do this process with octaheadron... but... i'm tired...
		// now we can draw 1/8 sphere... we draw 8 of 1/8 sphere...

		glGenBuffers(1,&VBO);
		glGenBuffers(1, &EBO);
		glGenVertexArrays(1, &VAO);

		glBindBuffer(GL_ARRAY_BUFFER, VBO);
		glBindVertexArray(VAO);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);

		glBufferData(GL_ARRAY_BUFFER, posArr.size() * sizeof(vec3), &posArr[0].x, GL_STATIC_DRAW);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sphIndices.size() * sizeof(unsigned int), &sphIndices[0], GL_STATIC_DRAW);

		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(vec3), (void*)0);

		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindVertexArray(0);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,0);

	}
	void draw() {
		shader->use();
		glBindVertexArray(VAO);

		int nIdx = 0;
		for (int line = 0; line < nBreak; line++)
			for (int startIdx = ((line + 1) * line) / 2; startIdx < ((line + 2) * (line + 1)) / 2; startIdx++)
				nIdx += 3;
			for (int line = 2; line < nBreak + 1; line++) for (int startIdx = ((line + 1) * line) / 2 + 1; startIdx < ((line + 2) * (line + 1)) / 2 - 1; startIdx++)
				nIdx += 3;

		for (int i = 0; i < 8; i++) {
			mat4 worldMat = mat4(1);
			worldMat = translate(worldMat, position);
			if(i>=4)
				worldMat = rotate(worldMat, radians(180.0f), vec3(1, 0, 0));
			worldMat = rotate(worldMat, radians(90.0f * i), vec3(0, 1, 0));
			shader->setVec3("ballColor", color);
			glUniformMatrix4fv(glGetUniformLocation(shader->ID, "worldMat"), 1, GL_FALSE, value_ptr(worldMat));
			glUniformMatrix4fv(glGetUniformLocation(shader->ID, "viewMat"), 1, GL_FALSE, &camera.GetViewMatrix()[0][0]);
			glUniformMatrix4fv(glGetUniformLocation(shader->ID, "projMat"), 1, GL_FALSE, &perspective(camera.Zoom, (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.01f, 100.0f)[0][0]);

			glDrawElements(GL_TRIANGLES, nIdx, GL_UNSIGNED_INT, NULL);
		}
		glBindVertexArray(0);
	}
	virtual vec3 getPos() {
		return position;
	}
	vec3 getVel() {
		return velocity;
	}
	vec3 getAcc() {
		return acceleration;
	}
	float getRadius() {
		return radius;
	}
	virtual unsigned int getType() {
		return type;
	}
	virtual void update(float dt) { // here collision checking should be processed

		position += velocity * dt + 0.5f*acceleration * dt* dt;
		velocity += acceleration * dt;
	}

	virtual void collisionResolve(Collision collEvent);
	Collision collision(vector<Rigidbody*> rgdBSet) { 	//TODO 당장은 wall 만
		Collision col{NULL,9999.0f};

		for (int i = 0; i < rgdBSet.size(); i++) {				// 가장 빨리 일어난 collision을 detection 해서 objManaget로 보내 주는 함수임.
			if (rgdBSet[i] == this) {}
			else {
				float colOccurTime = rgdBSet[i]->whenCollide(this);
				if (!(colOccurTime < -0.99f) && colOccurTime< col.occurence) // collision occur.
				{
					col.with = rgdBSet[i];
					col.occurence = colOccurTime;
				}
			}
		}

		return col;
	}


	float whenCollide(Rigidbody* obj) {//TODO 당장은 wall 만
		return -1.0f;
	}
};

class Wall : public Rigidbody {
private:
	vec3 color;

	float* vertices;				//TODO ball 처럼 setupDraw에서 internal하게 그릴 수 있도록 해라.
	vec3 position;
	vec3 normal;
	vec3 tangent;					// related with width, 
	vec3 bitangent;
	float width, height;			//TODO 이것도 나중에 받아. 지금 vertices를 이용해가지고 다 받아서 wall이 전체적으로 꼬였음.

	bool __isCollisionIn(float collTime, Rigidbody* obj);
public:


	Wall(vec3 position, vec3 normal, float* vertices, vec3 color) {
		this->position = position;
		this->normal = normalize(normal);

		if (position.x > 3)
			this->tangent = vec3(0, 0, 1);								// TODO temp tangent...
		else
			this->tangent = vec3(1, 0, 0);

		this->bitangent = normalize(cross(normal, tangent));

		this->vertices = vertices;
		this->color = color;
		this->shader = new Shader("src/shader/wall_vs.txt", "src/shader/wall_fs.txt");
		this->width = 10.0f;
		this->height = 10.0f;
		this->type = 0;


	}
	~Wall() {
		delete this->shader;
	}
	void setupDraw() { // simple plane. not use EBO.
		glGenBuffers(1, &VBO);
		glBindBuffer(GL_ARRAY_BUFFER, VBO);

		glGenVertexArrays(1, &VAO);
		glBindVertexArray(VAO);

		glBufferData(GL_ARRAY_BUFFER, 18 * sizeof(float), vertices, GL_STATIC_DRAW);

		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);

		glBindVertexArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
	}
	void draw() {
		shader->use();
		glUniformMatrix4fv(glGetUniformLocation(shader->ID, "viewMat"), 1, GL_FALSE, &camera.GetViewMatrix()[0][0]);
		glUniformMatrix4fv(glGetUniformLocation(shader->ID, "projMat"), 1, GL_FALSE, &perspective(camera.Zoom, (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.01f, 100.0f)[0][0]);
		shader->setVec3("color", color);

		glBindVertexArray(VAO);
		glDrawArrays(GL_TRIANGLES, 0, 18);
		glBindVertexArray(0);
	}
	virtual vec3 getPos() {
		return position;
	}
	virtual vec3 getVel() {
		return vec3(0);
	}
	virtual vec3 getAcc() {
		return vec3(0);
	}
	virtual unsigned int getType() {
		return type;
	}
	virtual void update(float dt) {

	}
	virtual Collision collision(vector<Rigidbody*> rgdBSet) {
		Collision col{ NULL,9999.0f };
		return col;
	}
	virtual float whenCollide(Rigidbody* obj) { //		TODO wall - each different obj type... collision...
		cout << obj->id << " obj : ";

		// solve equation...when wall - ball collision occur
		if (obj->getType() == 1) {
			float collTime = -1.0f;

			vec3 curPos = obj->getPos();

			vec3 normalToBall = normal;
			if (dot((position - curPos), normal) > 0)			//TODO. ball의 중심이 wall을 무한하게 확장한 평면 위에 존재할 때...?
				normalToBall *= -1.0f;


			float constant = dot(normal, obj->getPos()) - dot(normal, position) - ((Ball*)obj)->getRadius() * dot(normalToBall, normal);
			float linear = dot(normal, obj->getVel());
			float quadratic = 0.5f * dot(normal, obj->getAcc());				// t라	는 시간 후에 어떤 평면에 도달하게 될 때. t에대한 2차방정식이 나오는 데. 그 방정식의 계수...

			if (abs(quadratic) <= FLT_EPSILON) {							// x,z 축 방향으로는 가속도가 없기 때문에. normal에 y성분이 없다면 1차방정식이 됨. 그래도 일단 general 하게 짰음.
				collTime = -constant / linear;
				if (FLT_EPSILON < collTime && FLT_EPSILON < (deltaTime - elapsedTime) - collTime && __isCollisionIn(collTime, obj)) {}	// deltaTime은 이전 frame을 계산하는데 걸린 시간을 나타냄. 항상 같을 수 없으니 약간 오차를 고려해서 .03정도 고려함.
				else
					collTime = -1.0f;
			}
			else {															// 가속도가 있을 때.
				if (linear * linear - 4.0f * constant * quadratic >= 0)
				{
					float sol1 = (-linear - sqrt(linear * linear - 4.0f * constant * quadratic)) / quadratic / 2.0f;
					float sol2 = (-linear + sqrt(linear * linear - 4.0f * constant * quadratic)) / quadratic / 2.0f;

					if (quadratic > 0) {
						collTime = (sol1 > 0) ? sol1 : sol2;
						if (FLT_EPSILON < collTime && FLT_EPSILON < (deltaTime - elapsedTime) - collTime && __isCollisionIn(collTime, obj)) {}
						else
							collTime = -1.0f;
					}
					else if (quadratic < 0) {
						collTime = (sol2 > 0) ? sol2 : sol1;
						if (FLT_EPSILON < collTime && FLT_EPSILON < (deltaTime - elapsedTime)  - collTime && __isCollisionIn(collTime, obj)) {}
						else
							collTime = -1.0f;
					}
				}
			}
			if (collTime != -1.0f)
				cout << "at " << id << " wall collision occur in next step :  " << collTime << " sec  pos :" << obj->getPos().x << " : "<< obj->getPos().y<<" : "<< obj->getPos().z<<"  r : " << ((Ball*)obj)->getRadius() << " vel : " << obj->getVel().x << " : " << obj->getVel().y << " : " << obj->getVel().z << endl;
			else
				cout << "at " << id << " wall not collide" << endl;
			return collTime;
		}

		return -1.0f;
	}
	virtual void collisionResolve(Collision) {

	}
	vec3 getNormal() {
		return normal;
	}
};

class ObjManager {
private:
	vector<Rigidbody*> things;
	unsigned int assignId;

public : 

	ObjManager() {
		things = vector<Rigidbody*>();
		assignId = 0;
		elapsedTime = 0.0f;
	}

	~ObjManager() {
		vector<Rigidbody*>::iterator iter = things.begin();
		for (iter = things.begin(); iter != things.end(); iter++)
			delete *iter;
	}
	void addThing(Rigidbody* thing) {
		thing->id = assignId++;
		things.push_back(thing);
	}
	void draw() {
		vector<Rigidbody*>::iterator iter = things.begin();
		for (iter = things.begin(); iter != things.end(); iter++) {
			Rigidbody* a = *iter;
			a->draw();
		}
	}
	int getThingsSize() {
		return things.size();
	}
	virtual void collisionResolve(Collision collEvent) {

	}
	void update() {
//		collision 처리를 해 줘야 됨. 근데 여러가지 문제가 있지. 그 것을 한번에 통합해서 결과적으로 맞는 물리 현상을 기술해서 그 값들을 obj에 넘겨줘야 됨.

		vector<Rigidbody*>::iterator iter;
		
		for (iter = things.begin() ; iter != things.end();)  //높이 -50m이하로 떨어지면 ball 삭제.
		{	
			if ( (*iter)->getPos().y <= -50) {
				iter = things.erase(iter);
			}
			else
				iter++;
		}

		elapsedTime = 0.0f;

		cout << "update ㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡ" << endl;
		while (deltaTime-elapsedTime > 0) {

			Collision fastest{NULL,999.0f}; //fastest occurence

			vector<Rigidbody*>::iterator collObj;
			for (iter = things.begin(); iter != things.end(); iter++) {							// 각 obj입장에서 가장 빨리 일어난 collision들을 받아서. 그중에서 제일 빨리 일어난 collision을 선택 함. 그게 fastest.
				Collision c = (*iter)->collision(things);

				if (c.occurence < fastest.occurence) {
					fastest = c;
					collObj = iter;
				}
			}
																							//DeltaTime 안에서 여러번의 충돌이 있을 수 있음. 충돌이 여러번 일어나서 DeltaTime이 지나가 버리면 화면에 그 상태를 뿌려 줌.

			float elapsedDT = fastest.occurence;												// elapsedDT는 한 frame안에서 충돌과 다음 충돌이 발생하기 까지 걸린 시간.
 
			if (deltaTime-elapsedTime < elapsedDT) {											// elapsed DT가 deltaTime - elapsedTime 보다 클 때. 한 frame 안에 더이상 충돌 x. 그러니까. 남은 시간 deltaTime - elapsedTime만큼 motion 진행
				for (iter = things.begin(); iter != things.end(); iter++) {
					(*iter)->update(deltaTime - elapsedTime);
				}
			}
			else {																				// 모든 obj들 elapsedDt 만큼 motion 진행 및 충돌 발생한 obj collision resolve. 
				for (iter = things.begin(); iter != things.end(); iter++) {
//					(*iter)->update(deltaTime - elapsedTime);
					(*iter)->update(elapsedDT);										// 
				}
//				cout << "collision resolve!!!" << endl;
				(*collObj)->collisionResolve(fastest);

			}
			cout << deltaTime - elapsedTime << endl;
			elapsedTime += elapsedDT;
		}
		cout << "update end ㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡ" << endl;

	}
};


//we know time when plane equation and obj collide. but we dont know that collision occur really in wall.
bool Wall::__isCollisionIn(float collTime, Rigidbody* obj) { // TODO wall - each different obj type... collision...
	
	// wall ball collision
	if (obj->getType() == 1) {
		vec3 objAcc = obj->getAcc();		// present state
		vec3 objVel = obj->getVel();
		vec3 objPos = obj->getPos();

		vec3 nextPos = objPos + objVel * collTime + 0.5f * objAcc * collTime * collTime;

		vec3 normalToBall = normal;
		if (dot((position - objPos), normal) > 0)
			normalToBall *= -1.0f;

		vec3 collPoint = nextPos - ((Ball*)obj)->getRadius() * normalToBall;

		if (abs(dot(collPoint, normal) - dot(normal, position)) <= 5.0f * FLT_EPSILON) {		// 평면에 도달했음.

			Eigen::MatrixXf m1 = Eigen::MatrixXf(3, 2);
			m1 << tangent.x, bitangent.x,		tangent.y, bitangent.y,		tangent.z,bitangent.z;

			Eigen::Vector3f v1;
			vec3 c_p = collPoint - position;
			v1 << c_p.x, c_p.y, c_p.z;

			float w = m1.colPivHouseholderQr().solve(v1)[0];
			float h = m1.colPivHouseholderQr().solve(v1)[1];
			if (abs(w) - width / 2 <= FLT_EPSILON && abs(h) - height / 2 <= FLT_EPSILON)
				return true;
			else
				return false;
		}
		// TODO wall edge collision.
		return false;
	}
	return true;
}

void Ball::collisionResolve(Collision collEvent) {

	// ball collision with wall
	if (collEvent.with->getType() == 0) {	// TODO 당장은 Wall 만.
		//reflection...
		// TODO wall의 normal을 가져와서. velocity값을 reflection 시킨다. 그러면 이제 다음 frame의 whencollision함수에서 이론적으로 충돌하지 않는다라는 결과가 나오게 될거임 ㅇㅇ

		vec3 velB = this->velocity;
		float lenVel = length(velB);
		velB = normalize(velB);

		Wall* wall = ((Wall*)collEvent.with);

		vec3 normalToBall = normalize(wall->getNormal());
		if (dot(position - wall->getPos(), wall->getNormal()) < 0)				//TODO. ball의 중심이 wall을 무한하게 확장한 평면 위에 존재할 때...? 이 부분wall의 whenCollision에서도 문제 될 수 있음.
			normalToBall *= -1.0f;

		vec3 newVel = velB - 2 * dot(velB, normalToBall) * normalToBall;
		newVel *= lenVel;					//TODO 속도 감소??
		velocity = newVel;

		// 만약에 collision 된 애가.update 이후에 안쪽으로 들어오게 될 수도 있음. 아마 부동소수점 이슈겠지. 그러면 밖을 로 꺼내줘야 됨.
		// if distance btw wall and ball <= rad of ball.. then.. pos of ball is offseted to > r

		float distBtwWallBall = dot(normalToBall, position - wall->getPos());
		if (distBtwWallBall <= radius)
			position += normalToBall * (radius - distBtwWallBall + 0.1f); 
	}
}