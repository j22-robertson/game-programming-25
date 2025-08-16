//
// Created by James Robertson on 16/08/2025.
//



#ifndef GP25_EXERCISES_CORECOMPONENTS_H
#define GP25_EXERCISES_CORECOMPONENTS_H

#include <glm/glm.hpp>

#include "glm/gtx/quaternion.hpp"
#include "glm/gtx/transform.hpp"

struct Transform {
    Transform() : position(0.0f),rotation(),scale(1.0){};
    glm::mat4 Get_TransformMatrix() const {
        auto transform = glm::mat4(1.0f);
        transform = glm::translate(transform,position);
        transform *= glm::toMat4(rotation);
        transform = glm::scale(transform, scale);
        return transform;
    }

    glm::vec3& Get_Position() {
        return position;
    }

    glm::vec3& Get_Scale() {
        return scale;
    }

    glm::quat& Get_RotationAxis() {
        return rotation;
    }


    void Rotate(const glm::vec3& axis, const float angle_degrees) {
        rotation = glm::rotate(rotation, glm::radians(angle_degrees), axis);
    }

    void Translate(const glm::vec3& delta) {
        position+=delta;
    }

    void Scale(const glm::vec3& scale_force) {
        scale += scale_force;
    }

private:
    glm::vec3 position = glm::vec3(0, 0, 0);
    glm::vec3 scale =   glm::vec3(0,0,0);
    glm::quat rotation = glm::vec3(0,0,0);
};

#endif //GP25_EXERCISES_CORECOMPONENTS_H