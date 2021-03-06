#ifndef _RENDER_CUBES_H_
#define _RENDER_CUBES_H_

#include <iostream>

#include "shader.h"

struct sCubeRenderer {
    unsigned int   VAO = -1;
    unsigned int   VBO = -1;

    sShader shader;
};


inline void cube_renderer_init(sCubeRenderer *renderer) {
    const float clip_vertex[] = {
	      0.0f, 0.0f, 0.0f,
        1.0f, 0.0f, 0.0f,
        1.0f,  1.0f, 0.0f, 
        1.0f,  1.0f, 0.0f,
        0.0f,  1.0f, 0.0f, 
        0.0f, 0.0f, 0.0f,

        0.0f, 0.0f,  1.0f,
        1.0f, 0.0f,  1.0f,
        1.0f,  1.0f,  1.0f,
        1.0f,  1.0f,  1.0f,
        0.0f,  1.0f,  1.0f,
        0.0f, 0.0f,  1.0f,

        0.0f,  1.0f,  1.0f,
        0.0f,  1.0f, 0.0f,
        0.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 
        0.0f, 0.0f,  1.0f,
        0.0f,  1.0f,  1.0f,

        1.0f,  1.0f,  1.0f,
        1.0f,  1.0f, 0.0f,
        1.0f, 0.0f, 0.0f,
        1.0f, 0.0f, 0.0f,
        1.0f, 0.0f,  1.0f,
        1.0f,  1.0f,  1.0f,

        0.0f, 0.0f, 0.0f,
        1.0f, 0.0f, 0.0f,
        1.0f, 0.0f,  1.0f,
        1.0f, 0.0f,  1.0f,
        0.0f, 0.0f,  1.0f,
        0.0f, 0.0f, 0.0f,

        0.0f,  1.0f, 0.0f,
        1.0f,  1.0f, 0.0f,
        1.0f,  1.0f,  1.0f,
        1.0f,  1.0f,  1.0f,
        0.0f,  1.0f,  1.0f,
        0.0f,  1.0f, 0.0f
	  }; 

    const char* vertex_shader = " \
        #version 410 \n\
        layout(location = 0) in vec3 a_pos; \n\
        uniform mat4 u_model; \n\
        uniform mat4 u_proj; \n\
        void main() { \n\
            gl_Position = u_proj * u_model * vec4(a_pos, 1.0); \n\
        \n} \
    ";

    const char* fragment_shader = " \
        #version 410 \n\
        layout(location = 0) out vec4 FragColor; \n\
        uniform vec4 u_color; \n\
        void main() { \n\
            FragColor = u_color; \n\
        \n} \
    ";

    glGenVertexArrays(1, &renderer->VAO);
	  glGenBuffers(1, &renderer->VBO); 

	  glBindVertexArray(renderer->VAO);

	  glBindBuffer(GL_ARRAY_BUFFER, renderer->VBO);
	  glBufferData(GL_ARRAY_BUFFER, sizeof(clip_vertex), clip_vertex, GL_STATIC_DRAW);

	  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*) 0);

	  glEnableVertexAttribArray(0);
	  glBindBuffer(GL_ARRAY_BUFFER, 0);
	  glBindVertexArray(0);

    renderer->shader = sShader(vertex_shader, fragment_shader);
}


inline void cube_renderer_render(sCubeRenderer *renderer,
                                 const sMat44 *models,
                                 const sVector4 *colors,
                                 const int obj_count,
                                 const sMat44 *proj_mat) {
    glBindVertexArray(renderer->VAO);

    renderer->shader.activate();

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    for(int i = 0; i < obj_count; i++) {
        renderer->shader.set_uniform_matrix4("u_proj", proj_mat);
        renderer->shader.set_uniform_matrix4("u_model", &models[i]);
        renderer->shader.set_uniform_vector("u_color", colors[i]);

        glDrawArrays(GL_TRIANGLES, 0, 36);

        //std::cout << "Draw cube with color R:" << colors[i].x << " G:" << colors[i].y << " B:" << colors[i].z << std::endl;
    }
    renderer->shader.deactivate();

    glBindVertexArray(0);
}


#endif
