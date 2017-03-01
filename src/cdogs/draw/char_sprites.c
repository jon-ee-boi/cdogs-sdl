/*
    C-Dogs SDL
    A port of the legendary (and fun) action/arcade cdogs.
    Copyright (c) 2017 Cong Xu
    All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions are met:

    Redistributions of source code must retain the above copyright notice, this
    list of conditions and the following disclaimer.
    Redistributions in binary form must reproduce the above copyright notice,
    this list of conditions and the following disclaimer in the documentation
    and/or other materials provided with the distribution.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
    AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
    ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
    LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
    CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
    SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
    INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
    CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
    ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
    POSSIBILITY OF SUCH DAMAGE.
*/
#include "char_sprites.h"

#include <tinydir/tinydir.h>

#include "log.h"
#include "sys_config.h"
#include "yajl_utils.h"


#define VERSION 1

CharSpriteClasses gCharSpriteClasses;


const CharSprites *StrCharSpriteClass(const char *s)
{
	CharSprites *c;
	int error = hashmap_get(gCharSpriteClasses.customClasses, s, (any_t *)&c);
	if (error == MAP_OK) return c;
	error = hashmap_get(gCharSpriteClasses.classes, s, (any_t *)&c);
	if (error == MAP_OK) return c;
	return NULL;
}

void CharSpriteClassesInit(CharSpriteClasses *c)
{
	memset(c, 0, sizeof *c);
	c->classes = hashmap_new();
	c->customClasses = hashmap_new();
	char buf[CDOGS_PATH_MAX];
	GetDataFilePath(buf, "");
	CharSpriteClassesLoadDir(c->classes, buf);
}

static CharSprites *CharSpritesLoadJSON(const char *path);
void CharSpriteClassesLoadDir(map_t classes, const char *path)
{
	char buf[CDOGS_PATH_MAX];
	sprintf(buf, "%s/graphics/chars/bodies", path);
	tinydir_dir dir;
	if (tinydir_open(&dir, buf) == -1)
	{
		goto bail;
	}

	for (; dir.has_next; tinydir_next(&dir))
	{
		tinydir_file file;
		if (tinydir_readfile(&dir, &file) == -1)
		{
			LOG(LM_MAIN, LL_ERROR, "Cannot read file '%s': %s",
				file.path, strerror(errno));
			goto bail;
		}
		if (!file.is_dir || file.name[0] == '.')
		{
			continue;
		}
		CharSprites *c = CharSpritesLoadJSON(file.path);
		if (c == NULL)
		{
			continue;
		}
		const int error = hashmap_put(classes, file.name, c);
		if (error != MAP_OK)
		{
			LOG(LM_MAIN, LL_ERROR, "failed to add char sprites %s: %d",
				file.name, error);
			continue;
		}
	}

bail:
	tinydir_close(&dir);
}
static CharSprites *CharSpritesLoadJSON(const char *path)
{
	CharSprites *c = NULL;
	// Try to find a data.json in this dir
	tinydir_file dataFile;
	yajl_val node = NULL;
	char buf[CDOGS_PATH_MAX];
	sprintf(buf, "%s/data.json", path);
	if (tinydir_file_open(&dataFile, buf) != 0)
	{
		goto bail;
	}
	node = YAJLReadFile(buf);
	if (node == NULL)
	{
		LOG(LM_MAIN, LL_ERROR, "Error parsing char sprite JSON '%s'", buf);
		goto bail;
	}

	CCALLOC(c, sizeof *c);
	YAJLInt(&c->NeckOffset, node, "NeckOffset");
	YAJLInt(&c->FootOffset, node, "FootOffset");
	YAJLInt(&c->LegsOffset, node, "LegsOffset");
	YAJLInt(&c->WristOffset, node, "WristOffset");

bail:
	yajl_tree_free(node);
	return c;
}

static void CharSpritesDestroy(any_t data);
void CharSpriteClassesClear(map_t classes)
{
	hashmap_destroy(classes, CharSpritesDestroy);
}
static void CharSpritesDestroy(any_t data)
{
	CharSprites *c = data;
	CFREE(c);
}
void CharSpriteClassesTerminate(CharSpriteClasses *c)
{
	CharSpriteClassesClear(c->classes);
	CharSpriteClassesClear(c->customClasses);
}