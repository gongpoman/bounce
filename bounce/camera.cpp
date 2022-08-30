
#include<glad/glad.h>
#include <GLFW/glfw3.h>

#include<iostream>

#include<glm/glm.hpp>
#include<glm/gtc/type_ptr.hpp>
#include<glm/gtc/matrix_transform.hpp>

#include"camera.h"

glm::mat4 customLookAt(glm::vec3 pos, glm::vec3 tar, glm::vec3 worldup) {
    glm::vec3 dir = glm::normalize(pos - tar);
    glm::vec3 right = glm::normalize(glm::cross(worldup, dir));
    glm::vec3 up = glm::normalize(glm::cross(dir, right));

    glm::mat4 rotation = glm::mat4(glm::vec4(right.x, right.y, right.z, 0.0f),
        glm::vec4(up.x, up.y, up.z, 0.0f),
        glm::vec4(dir.x, dir.y, dir.z, 0.0f),
        glm::vec4(0, 0, 0, 1));
    rotation = glm::transpose(rotation);

    glm::mat4 translation(1.0f);
    translation = glm::translate(translation, -pos);
    return rotation*translation;
  
}

glm::mat4 Camera::GetViewMatrix(){
    return glm::lookAt(Position, Position + Front, Up);
//    return customLookAt(Position, Position + Front, WorldUp);
}
void Camera::ProcessKeyboard(Camera_Movement direction, float deltaTime) {
    float velocity = MovementSpeed * deltaTime;
    if (direction == FORWARD)
        Position += velocity * Front;
    if (direction == BACKWARD)
        Position -= velocity * Front;
    if (direction == RIGHT)
        Position += velocity * Right;
    if (direction == LEFT)
        Position -= velocity * Right;
}
void Camera::ProcessMouseMovement(float xoffset, float yoffset, GLboolean constrainPitch) {
    xoffset *= MouseSensitivity;
    yoffset *= MouseSensitivity;
    
    Pitch += yoffset;
    if (constrainPitch) {
        if (Pitch > 89.0f)
            Pitch = 89.0f;
        if (Pitch < -89.0f)
            Pitch = -89.0f;
    }

    Yaw += xoffset;
    updateCameraVectors();
}
void Camera::ProcessMouseScroll(float yoffset) {
    Zoom -= (float)yoffset;
    if (Zoom < 1.0f)
        Zoom = 1.0f;
    if (Zoom > 45.0f)
        Zoom = 45.0f;
}
void Camera::updateCameraVectors() {
    glm::vec3 front;
    front.x = glm::cos(glm::radians(Yaw)) * glm::cos(glm::radians(Pitch));
    front.y = glm::sin(glm::radians(Pitch));
    front.z = glm::sin(glm::radians(Yaw)) * glm::cos(glm::radians(Pitch));
    Front = glm::normalize(front);                      //-u  
    Right = glm::normalize(glm::cross(Front, WorldUp)); //v
    Up = glm::normalize(glm::cross(Right, Front));      //v
    // u,v,n이 camera space의 basis지만 LookAt함수에서 받는 것은 다른 것들이라서 조금 헷갈 릴 수 있음
}
