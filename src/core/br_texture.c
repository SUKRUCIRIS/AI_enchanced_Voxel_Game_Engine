#include "../../third_party/stb/stb_image.h"
#include "br_texture.h"

br_texture_manager *create_br_texture_manager(void)
{
	br_texture_manager *x = malloc(sizeof(br_texture_manager));
	x->textures = create_DA(sizeof(br_texture *), 0);
	for (int i = 0; i < 32; i++)
	{
		x->indices[i] = 0;
	}
	x->programs = create_DA(sizeof(GLuint), 0);
	x->uniforms = create_DA(sizeof(GLint), 0);
	return x;
}

br_texture *create_br_texture(br_texture_manager *manager, const char *path, GLenum texType,
															GLint min_filter, GLint mag_filter, int index, int wraps, int wrapt)
{
	if (get_size_DA(manager->textures) >= 32)
	{
		return 0;
	}
	glBindTexture(texType, 0);
	br_texture *tex = calloc(1, sizeof(br_texture));
	int widthImg, heightImg, numColCh;
	stbi_set_flip_vertically_on_load(1);
	unsigned char *bytes = stbi_load(path, &widthImg, &heightImg, &numColCh, 0);
	if (!bytes)
	{
		return 0;
	}
	GLenum format = GL_RGB;
	if (numColCh == 1)
	{
		format = GL_RED;
	}
	else if (numColCh == 3)
	{
		format = GL_RGB;
	}
	else if (numColCh == 4)
	{
		format = GL_RGBA;
	}
	glGenTextures(1, &(tex->id));
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(texType, tex->id);
	glTexParameteri(texType, GL_TEXTURE_MIN_FILTER, min_filter);
	glTexParameteri(texType, GL_TEXTURE_MAG_FILTER, mag_filter);
	glTexParameteri(texType, GL_TEXTURE_WRAP_S, wraps);
	glTexParameteri(texType, GL_TEXTURE_WRAP_T, wrapt);
	glTexImage2D(texType, 0, GL_RGBA, widthImg, heightImg, 0, format, GL_UNSIGNED_BYTE, bytes);
	if (min_filter == GL_NEAREST_MIPMAP_NEAREST ||
			min_filter == GL_LINEAR_MIPMAP_NEAREST ||
			min_filter == GL_NEAREST_MIPMAP_LINEAR ||
			min_filter == GL_LINEAR_MIPMAP_LINEAR)
	{
		glGenerateMipmap(texType);
	}
	stbi_image_free(bytes);
	glBindTexture(texType, 0);
	tex->type = texType;
	tex->manager = manager;
	pushback_DA(manager->textures, &tex);
	manager->indices[get_index_DA(manager->textures, &tex)] = index;
	return tex;
}

br_texture *create_br_texture_memory(br_texture_manager *manager, unsigned char *data, int width, int height, GLenum texType,
																		 GLint min_filter, GLint mag_filter, int index, int wraps, int wrapt)
{
	if (get_size_DA(manager->textures) >= 32)
	{
		return 0;
	}
	glBindTexture(texType, 0);
	br_texture *tex = calloc(1, sizeof(br_texture));
	glGenTextures(1, &(tex->id));
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(texType, tex->id);
	glTexParameteri(texType, GL_TEXTURE_MIN_FILTER, min_filter);
	glTexParameteri(texType, GL_TEXTURE_MAG_FILTER, mag_filter);
	glTexParameteri(texType, GL_TEXTURE_WRAP_S, wraps);
	glTexParameteri(texType, GL_TEXTURE_WRAP_T, wrapt);
	glTexImage2D(texType, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
	glGenerateMipmap(texType);
	glBindTexture(texType, 0);
	tex->type = texType;
	tex->manager = manager;
	pushback_DA(manager->textures, &tex);
	manager->indices[get_index_DA(manager->textures, &tex)] = index;
	return tex;
}

void delete_br_texture(br_texture *texture)
{
	unsigned int index = get_index_DA(texture->manager->textures, &texture);
	for (int i = index; i < 31; i++)
	{
		texture->manager->indices[i] = texture->manager->indices[i + 1];
	}
	remove_DA(texture->manager->textures, index);
	glDeleteTextures(1, &(texture->id));
	free(texture);
}

void delete_br_texture_manager(br_texture_manager *manager)
{
	br_texture **textures = get_data_DA(manager->textures);
	while (get_size_DA(manager->textures) > 0)
	{
		textures = get_data_DA(manager->textures);
		delete_br_texture(textures[0]);
	}
	delete_DA(manager->textures);
	delete_DA(manager->uniforms);
	delete_DA(manager->programs);
	free(manager);
}

void use_br_texture_manager(br_texture_manager *manager, GLuint program)
{
	br_texture **textures = get_data_DA(manager->textures);
	for (unsigned int i = 0; i < get_size_DA(manager->textures); i++)
	{
		glActiveTexture(GL_TEXTURE0 + manager->indices[i]);
		glBindTexture(textures[i]->type, textures[i]->id);
	}
	if (get_index_DA(manager->programs, &program) == UINT_MAX)
	{
		pushback_DA(manager->programs, &program);
		GLint uniform = glGetUniformLocation(program, "textures");
		pushback_DA(manager->uniforms, &uniform);
	}
	GLint *uniforms = get_data_DA(manager->uniforms);
	unsigned int index = get_index_DA(manager->programs, &program);
	glUniform1iv(uniforms[index], 32, manager->indices);
}